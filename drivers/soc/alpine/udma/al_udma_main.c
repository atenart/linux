/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/soc/alpine/al_udma_regs.h>
#include <linux/soc/alpine/al_hw_udma.h>
#include <linux/soc/alpine/al_hw_udma_config.h>

#define UDMA_STATE_IDLE		0x0
#define UDMA_STATE_NORMAL	0x1
#define UDMA_STATE_ABORT	0x2
#define UDMA_STATE_RESERVED	0x3

const char *const al_udma_states_name[] = {
	"Idle",
	"Normal",
	"Abort",
	"Reset"
};

#define AL_ADDR_LOW(x)	((u32)((dma_addr_t)(x)))
#define AL_ADDR_HIGH(x)	((u32)((((dma_addr_t)(x)) >> 16) >> 16))

static void al_udma_set_defaults(struct al_udma *udma)
{
	u8 rev_id = udma->rev_id;
	u32 tmp;

	if (udma->type == UDMA_TX) {
		void *tmp_unit_regs = udma->udma_regs;

		/*
		 * Setting the data fifo depth to 4K (256 strips of 16B)
		 * This allows the UDMA to have 16 outstanding writes
		 */
		if (rev_id >= AL_UDMA_REV_ID_2) {
			tmp = readl(tmp_unit_regs + UDMA_M2S_RD_DATA_CFG);
			tmp &= ~UDMA_M2S_RD_DATA_CFG_DATA_FIFO_DEPTH_MASK;
			tmp |= 256 << UDMA_M2S_RD_DATA_CFG_DATA_FIFO_DEPTH_SHIFT;
			writel(tmp, tmp_unit_regs + UDMA_M2S_RD_DATA_CFG);
		}

		/* set AXI timeout to 1M (~2.6 ms) */
		writel(1000000, tmp_unit_regs + UDMA_GEN_AXI_CFG_1);

		writel(0, tmp_unit_regs + UDMA_M2S_COMP_CFG_APP_ACK);
	}

	if (udma->type == UDMA_RX)
		writel(0, udma->udma_regs + UDMA_S2M_COMP_CFG_APP_ACK);
}

static void al_udma_config_compl(struct al_udma *udma)
{
	u32 val;

	if (udma->type != UDMA_RX)
		return;

	val = readl(udma->udma_regs + UDMA_S2M_COMP_CFG_1C);
	val &= ~UDMA_S2M_COMP_CFG_1C_DESC_SIZE_MASK;
	val |= (udma->cdesc_size >> 2) & UDMA_S2M_COMP_CFG_1C_DESC_SIZE_MASK;
	writel(val, udma->udma_regs + UDMA_S2M_COMP_CFG_1C);
}

/* Initialize the udma engine */
int al_udma_init(struct al_udma *udma, struct al_udma_params *udma_params)
{
	int i;

	udma->dev = udma_params->dev;

	if (udma_params->num_of_queues > DMA_MAX_Q) {
		dev_err(udma->dev, "udma: invalid num_of_queues parameter\n");
		return -EINVAL;
	}

	udma->type = udma_params->type;
	udma->num_of_queues = udma_params->num_of_queues;
	udma->cdesc_size = udma_params->cdesc_size;
	udma->gen_regs = udma_params->udma_regs_base;

	if (udma->type == UDMA_TX)
		udma->udma_regs = udma_params->udma_regs_base + UDMA_M2S_REGS;
	else
		udma->udma_regs = udma_params->udma_regs_base + UDMA_S2M_REGS;

	udma->rev_id = al_udma_get_revision(udma_params->udma_regs_base);

	if (udma_params->name == NULL)
		udma->name = "";
	else
		udma->name = udma_params->name;

	udma->state = UDMA_DISABLE;
	for (i = 0; i < DMA_MAX_Q; i++) {
		udma->udma_q[i].status = AL_QUEUE_NOT_INITIALIZED;
	}

	/*
	 * the register expects it to be in words
	 * initialize configuration registers to correct values
	 */
	al_udma_set_defaults(udma);

	al_udma_config_compl(udma);

	dev_dbg(udma->dev, "udma [%s] initialized. base %p\n", udma->name,
		udma->udma_regs);

	return 0;
}

/* Change the UDMA's state */
void al_udma_state_set(struct al_udma *udma, enum al_udma_state state)
{
	u32 reg = 0;

	if (state == udma->state)
		dev_dbg(udma->dev,
			"udma [%s]: requested state identical to current state (%d)\n",
			udma->name, state);
	else
		dev_dbg(udma->dev, "udma [%s]: change state from (%s) to (%s)\n",
			udma->name, al_udma_states_name[udma->state],
			 al_udma_states_name[state]);

	switch (state) {
	case UDMA_DISABLE:
		reg |= UDMA_M2S_CHANGE_STATE_DIS;
		break;
	case UDMA_NORMAL:
		reg |= UDMA_M2S_CHANGE_STATE_NORMAL;
		break;
	case UDMA_ABORT:
		reg |= UDMA_M2S_CHANGE_STATE_ABORT;
		break;
	default:
		dev_err(udma->dev, "udma: invalid state (%d)\n", state);
		return;
	}

	if (udma->type == UDMA_TX)
		writel(reg, udma->udma_regs + UDMA_M2S_CHANGE_STATE);
	else
		writel(reg, udma->udma_regs + UDMA_S2M_CHANGE_STATE);

	udma->state = state;
}

/* returns the current UDMA hardware state */
enum al_udma_state al_udma_state_get(struct al_udma *udma)
{
	u32 state_reg;
	u32 comp_ctrl;
	u32 stream_if;
	u32 data_rd;
	u32 desc_pref;

	if (udma->type == UDMA_TX)
		state_reg = readl(udma->udma_regs + UDMA_M2S_STATE);
	else
		state_reg = readl(udma->udma_regs + UDMA_S2M_STATE);

	comp_ctrl = (state_reg & UDMA_M2S_STATE_COMP_CTRL_MASK) >>
			UDMA_M2S_STATE_COMP_CTRL_SHIFT;
	stream_if = (state_reg & UDMA_M2S_STATE_STREAM_IF_MASK) >>
			UDMA_M2S_STATE_STREAM_IF_SHIFT;
	data_rd = (state_reg & UDMA_M2S_STATE_DATA_RD_CTRL_MASK) >>
			UDMA_M2S_STATE_DATA_RD_CTRL_SHIFT;
	desc_pref = (state_reg & UDMA_M2S_STATE_DESC_PREF_MASK) >>
			UDMA_M2S_STATE_DESC_PREF_SHIFT;

	/* if any of the states is abort then return abort */
	if ((comp_ctrl == UDMA_STATE_ABORT) || (stream_if == UDMA_STATE_ABORT)
			|| (data_rd == UDMA_STATE_ABORT)
			|| (desc_pref == UDMA_STATE_ABORT))
		return UDMA_ABORT;

	/* if any of the states is normal then return normal */
	if ((comp_ctrl == UDMA_STATE_NORMAL)
			|| (stream_if == UDMA_STATE_NORMAL)
			|| (data_rd == UDMA_STATE_NORMAL)
			|| (desc_pref == UDMA_STATE_NORMAL))
		return UDMA_NORMAL;

	return UDMA_IDLE;
}

/* get next completed packet from completion ring of the queue */
u32 al_udma_cdesc_packet_get(struct al_udma_q *udma_q,
			     volatile union al_udma_cdesc **cdesc)
{
	u32 count;
	volatile union al_udma_cdesc *curr;
	u32 comp_flags;

	/* comp_head points to the last comp desc that was processed */
	curr = udma_q->comp_head_ptr;
	comp_flags = le32_to_cpu(curr->al_desc_comp_tx.ctrl_meta);

	/* check if the completion descriptor is new */
	if (unlikely(!al_udma_new_cdesc(udma_q, comp_flags)))
		return 0;

	count = udma_q->pkt_crnt_descs + 1;

	/* if new desc found, increment the current packets descriptors */
	while (!cdesc_is_last(comp_flags)) {
		curr = al_cdesc_next_update(udma_q, curr);
		comp_flags = le32_to_cpu(curr->al_desc_comp_tx.ctrl_meta);

		if (unlikely(!al_udma_new_cdesc(udma_q, comp_flags))) {
			/*
			 * The current packet here doesn't have all
			 * descriptors completed. log the current desc
			 * location and number of completed descriptors so
			 * far. Then return.
			 */
			udma_q->pkt_crnt_descs = count;
			udma_q->comp_head_ptr = curr;

			return 0;
		}
		count++;
	}

	/* return back the first descriptor of the packet */
	*cdesc = al_udma_cdesc_idx_to_ptr(udma_q, udma_q->next_cdesc_idx);
	udma_q->pkt_crnt_descs = 0;
	udma_q->comp_head_ptr = al_cdesc_next_update(udma_q, curr);

	dev_dbg(udma_q->udma->dev,
		"udma [%s %d]: packet completed. first desc %p (ixd 0x%x) descs %d\n",
		udma_q->udma->name, udma_q->qid, *cdesc, udma_q->next_cdesc_idx,
		count);

	return count;
}
