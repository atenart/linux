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

/* dma_q flags */
#define AL_UDMA_Q_FLAGS_IGNORE_RING_ID	BIT(0)
#define AL_UDMA_Q_FLAGS_NO_COMP_UPDATE	BIT(1)
#define AL_UDMA_Q_FLAGS_EN_COMP_COAL	BIT(2)

#define AL_UDMA_INITIAL_RING_ID		1

#define AL_ADDR_LOW(x)	((u32)((dma_addr_t)(x)))
#define AL_ADDR_HIGH(x)	((u32)((((dma_addr_t)(x)) >> 16) >> 16))

/*
 * misc queue configurations
 *
 * @param udma_q udma queue data structure
 */
static void al_udma_q_config(struct al_udma_q *udma_q)
{
	u32 *reg_addr;
	u32 val;

	if (udma_q->udma->type == UDMA_TX) {
		reg_addr = udma_q->q_regs + UDMA_M2S_Q_MASK;

		/* enable DMB */
		val = readl(reg_addr);
		val &= ~UDMA_M2S_Q_RATE_LIMIT_MASK_INTERNAL_PAUSE_DMB;
		writel(val, reg_addr);
	}
}

/*
 * set the queue's completion configuration register
 *
 * @param udma_q udma queue data structure
 */
static void al_udma_q_config_compl(struct al_udma_q *udma_q)
{
	u32 *reg_addr;
	u32 val;

	if (udma_q->udma->type == UDMA_TX)
		reg_addr = udma_q->q_regs + UDMA_M2S_Q_COMP_CFG;
	else
		reg_addr = udma_q->q_regs + UDMA_S2M_Q_COMP_CFG;

	val = readl(reg_addr);

	if (udma_q->flags & AL_UDMA_Q_FLAGS_NO_COMP_UPDATE)
		val &= ~UDMA_M2S_Q_COMP_CFG_EN_COMP_RING_UPDATE;
	else
		val |= UDMA_M2S_Q_COMP_CFG_EN_COMP_RING_UPDATE;

	if (udma_q->flags & AL_UDMA_Q_FLAGS_EN_COMP_COAL)
		val &= ~UDMA_M2S_Q_COMP_CFG_DIS_COMP_COAL;
	else
		val |= UDMA_M2S_Q_COMP_CFG_DIS_COMP_COAL;

	writel(val, reg_addr);
}

/*
 * reset the queues pointers (Head, Tail, etc) and set the base addresses
 *
 * @param udma_q udma queue data structure
 */
static void al_udma_q_set_pointers(struct al_udma_q *udma_q)
{
	/*
	 * reset the descriptors ring pointers
	 * assert descriptor base address aligned.
	 */
	WARN_ON((AL_ADDR_LOW(udma_q->desc_phy_base) &
			~UDMA_M2S_Q_TDRBP_LOW_ADDR_MASK) != 0);
	writel(AL_ADDR_LOW(udma_q->desc_phy_base),
	       udma_q->q_regs + UDMA_RINGS_DRBP_LOW);
	writel(AL_ADDR_HIGH(udma_q->desc_phy_base),
	       udma_q->q_regs + UDMA_RINGS_DRBP_HIGH);

	writel(udma_q->size, udma_q->q_regs + UDMA_RINGS_DRL);

	/* if completion ring update disabled */
	if (udma_q->cdesc_base_ptr == NULL) {
		udma_q->flags |= AL_UDMA_Q_FLAGS_NO_COMP_UPDATE;
	} else {
		/*
		 * reset the completion descriptors ring pointers
		 * assert completion base address aligned.
		 */
		WARN_ON((AL_ADDR_LOW(udma_q->cdesc_phy_base) & ~UDMA_M2S_Q_TCRBP_LOW_ADDR_MASK) != 0);
		writel(AL_ADDR_LOW(udma_q->cdesc_phy_base),
		       udma_q->q_regs + UDMA_RINGS_CRBP_LOW);
		writel(AL_ADDR_HIGH(udma_q->cdesc_phy_base),
		       udma_q->q_regs + UDMA_RINGS_CRBP_HIGH);
	}
	al_udma_q_config_compl(udma_q);
}

/*
 * enable/disable udma queue
 *
 * @param udma_q udma queue data structure
 * @param enable none zero value enables the queue, zero means disable
 */
static void al_udma_q_enable(struct al_udma_q *udma_q, int enable)
{
	u32 reg = readl(udma_q->q_regs + UDMA_RINGS_CFG);

	if (enable) {
		reg |= (UDMA_M2S_Q_CFG_EN_PREF | UDMA_M2S_Q_CFG_EN_SCHEDULING);
		udma_q->status = AL_QUEUE_ENABLED;
	} else {
		reg &= ~(UDMA_M2S_Q_CFG_EN_PREF | UDMA_M2S_Q_CFG_EN_SCHEDULING);
		udma_q->status = AL_QUEUE_DISABLED;
	}

	writel(reg, udma_q->q_regs + UDMA_RINGS_CFG);
}

/* Initialize the udma queue data structure */
int al_udma_q_init(struct al_udma *udma, u32 qid,
		   struct al_udma_q_params *q_params)
{
	struct al_udma_q *udma_q;

	if (qid >= udma->num_of_queues) {
		dev_err(udma->dev, "udma: invalid queue id (%d)\n", qid);
		return -EINVAL;
	}

	if (udma->udma_q[qid].status == AL_QUEUE_ENABLED) {
		dev_err(udma->dev, "udma: queue (%d) already enabled!\n", qid);
		return -EIO;
	}

	if (q_params->size < AL_UDMA_MIN_Q_SIZE) {
		dev_err(udma->dev, "udma: queue (%d) size too small\n", qid);
		return -EINVAL;
	}

	if (q_params->size > AL_UDMA_MAX_Q_SIZE) {
		dev_err(udma->dev, "udma: queue (%d) size too large\n", qid);
		return -EINVAL;
	}

	if (q_params->size & (q_params->size - 1)) {
		dev_err(udma->dev,
			"udma: queue (%d) size (%d) must be power of 2\n",
			q_params->size, qid);
		return -EINVAL;
	}

	udma_q = &udma->udma_q[qid];
	/* set the queue's regs base address */
	if (udma->type == UDMA_TX)
		udma_q->q_regs = udma->udma_regs + UDMA_M2S_Q(qid);
	else
		udma_q->q_regs = udma->udma_regs + UDMA_S2M_Q(qid);

	udma_q->adapter_rev_id = q_params->adapter_rev_id;
	udma_q->size = q_params->size;
	udma_q->size_mask = q_params->size - 1;
	udma_q->desc_base_ptr = q_params->desc_base;
	udma_q->desc_phy_base = q_params->desc_phy_base;
	udma_q->cdesc_base_ptr = q_params->cdesc_base;
	udma_q->cdesc_phy_base = q_params->cdesc_phy_base;

	udma_q->next_desc_idx = 0;
	udma_q->next_cdesc_idx = 0;
	udma_q->end_cdesc_ptr = (u8 *) udma_q->cdesc_base_ptr +
	    (udma_q->size - 1) * udma->cdesc_size;
	udma_q->comp_head_idx = 0;
	udma_q->comp_head_ptr = (union al_udma_cdesc *)udma_q->cdesc_base_ptr;
	udma_q->desc_ring_id = AL_UDMA_INITIAL_RING_ID;
	udma_q->comp_ring_id = AL_UDMA_INITIAL_RING_ID;

	udma_q->pkt_crnt_descs = 0;
	udma_q->flags = 0;
	udma_q->status = AL_QUEUE_DISABLED;
	udma_q->udma = udma;
	udma_q->qid = qid;

	/* start hardware configuration: */
	al_udma_q_config(udma_q);
	/* reset the queue pointers */
	al_udma_q_set_pointers(udma_q);

	/* enable the q */
	al_udma_q_enable(udma_q, 1);

	dev_dbg(udma->dev,
		"udma [%s %d]: %s q init. size 0x%x\n  desc ring info: phys base 0x%llx virt base %p)",
		udma_q->udma->name, udma_q->qid,
		udma->type == UDMA_TX ? "Tx" : "Rx", q_params->size,
		(unsigned long long)q_params->desc_phy_base,
		q_params->desc_base);
	dev_dbg(udma->dev,
		"  cdesc ring info: phys base 0x%llx virt base %p",
		(unsigned long long)q_params->cdesc_phy_base,
		q_params->cdesc_base);

	return 0;
}

/* return (by reference) a pointer to a specific queue date structure. */
int al_udma_q_handle_get(struct al_udma *udma, u32 qid,
			 struct al_udma_q **q_handle)
{

	if (unlikely(qid >= udma->num_of_queues)) {
		dev_err(udma->dev, "udma [%s]: invalid queue id (%d)\n",
			udma->name, qid);
		return -EINVAL;
	}

	*q_handle = &udma->udma_q[qid];
	return 0;
}
