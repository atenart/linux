/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/device.h>
#include <linux/soc/alpine/al_udma_regs.h>
#include <linux/soc/alpine/al_hw_udma_config.h>
#include <linux/soc/alpine/al_hw_udma_regs.h>

/* M2S packet len configuration */
int al_udma_m2s_packet_size_cfg_set(struct al_udma *udma,
				    struct al_udma_m2s_pkt_len_conf *conf)
{
	u32 reg = readl(&udma->udma_regs + UDMA_M2S_CFG_LEN);
	u32 max_supported_size = UDMA_M2S_CFG_LEN_MAX_PKT_SIZE_MASK;

	WARN_ON(udma->type != UDMA_TX);

	if (conf->encode_64k_as_zero)
		max_supported_size += 1;

	if (conf->max_pkt_size > max_supported_size) {
		dev_err(udma->dev,
			"udma [%s]: requested max_pkt_size (0x%x) exceeds the supported limit (0x%x)\n",
			udma->name, conf->max_pkt_size, max_supported_size);
		return -EINVAL;
	}

	reg &= ~UDMA_M2S_CFG_LEN_ENCODE_64K;
	if (conf->encode_64k_as_zero)
		reg |= UDMA_M2S_CFG_LEN_ENCODE_64K;
	else
		reg &= ~UDMA_M2S_CFG_LEN_ENCODE_64K;

	reg &= ~UDMA_M2S_CFG_LEN_MAX_PKT_SIZE_MASK;
	reg |= conf->max_pkt_size;

	writel(reg, &udma->udma_regs + UDMA_M2S_CFG_LEN);

	return 0;
}

/* set max descriptors */
void al_udma_m2s_max_descs_set(struct al_udma *udma, u8 max_descs)
{
	u32 pref_thr = max_descs, min_burst_above_thr = 4, tmp;

	/*
	 * increase min_burst_above_thr so larger burst can be used to fetch
	 * descriptors
	 */
	if (pref_thr >= 8)
		min_burst_above_thr = 8;
	/*
	 * don't set prefetch threshold too low so we can have the
	 * min_burst_above_thr >= 4
	 */
	else
		pref_thr = 4;

	tmp = readl(&udma->udma_regs + UDMA_M2S_RD_DESC_PREF_CFG_2);
	tmp &= ~UDMA_M2S_RD_DESC_PREF_CFG_2_MAX_DESC_PER_PKT_MASK;
	tmp |= max_descs << UDMA_M2S_RD_DESC_PREF_CFG_2_MAX_DESC_PER_PKT_SHIFT;
	writel(tmp, &udma->udma_regs + UDMA_M2S_RD_DESC_PREF_CFG_2);

	tmp = readl(&udma->udma_regs + UDMA_M2S_RD_DESC_PREF_CFG_3);
	tmp &= ~(UDMA_M2S_RD_DESC_PREF_CFG_3_PREF_THR_MASK |
		 UDMA_M2S_RD_DESC_PREF_CFG_3_MIN_BURST_ABOVE_THR_MASK);
	tmp |= pref_thr << UDMA_M2S_RD_DESC_PREF_CFG_3_PREF_THR_SHIFT;
	tmp |= min_burst_above_thr << UDMA_M2S_RD_DESC_PREF_CFG_3_MIN_BURST_ABOVE_THR_SHIFT;
	writel(tmp, &udma->udma_regs + UDMA_M2S_RD_DESC_PREF_CFG_3);
}

/* set s2m max descriptors */
void al_udma_s2m_max_descs_set(struct al_udma *udma, u8 max_descs)
{
	u32 pref_thr = max_descs, min_burst_above_thr = 4, tmp;

	/*
	 * increase min_burst_above_thr so larger burst can be used to fetch
	 * descriptors
	 */
	if (pref_thr >= 8)
		min_burst_above_thr = 8;
	/*
	 * don't set prefetch threshold too low so we can have the
	 * min_burst_above_thr >= 4
	 */
	else
		pref_thr = 4;

	tmp = readl(&udma->udma_regs + UDMA_S2M_RD_DESC_PREF_CFG_3);
	tmp &= ~(UDMA_S2M_RD_DESC_PREF_CFG_3_PREF_THR_MASK |
		 UDMA_S2M_RD_DESC_PREF_CFG_3_MIN_BURST_ABOVE_THR_MASK);
	tmp |= pref_thr << UDMA_S2M_RD_DESC_PREF_CFG_3_PREF_THR_SHIFT;
	tmp |= min_burst_above_thr << UDMA_S2M_RD_DESC_PREF_CFG_3_MIN_BURST_ABOVE_THR_SHIFT;
	writel(tmp, &udma->udma_regs + UDMA_S2M_RD_DESC_PREF_CFG_3);
}

/* S2M UDMA configure a queue's completion descriptors coalescing */
void al_udma_s2m_q_compl_coal_config(struct al_udma_q *udma_q, bool enable,
				     u32 coal_timeout)
{
	u32 reg = readl(udma_q->q_regs + UDMA_S2M_Q_COMP_CFG);

	if (enable)
		reg &= ~UDMA_S2M_Q_COMP_CFG_DIS_COMP_COAL;
	else
		reg |= UDMA_S2M_Q_COMP_CFG_DIS_COMP_COAL;

	writel(reg, udma_q->q_regs + UDMA_S2M_Q_COMP_CFG);
	writel(coal_timeout, udma_q->q_regs + UDMA_S2M_Q_COMP_CFG_2);
}

/* S2M UDMA configure completion descriptors write burst parameters */
int al_udma_s2m_compl_desc_burst_config(struct al_udma *udma, u16 burst_size)
{
	u32 tmp;

	if ((burst_size != 64) && (burst_size != 128) && (burst_size != 256)) {
		dev_err(udma->dev, "invalid burst_size value (%d)\n",
			burst_size);
		return -EINVAL;
	}

	/* convert burst size from bytes to beats (16 byte) */
	burst_size = burst_size / 16;

	tmp = readl(&udma->udma_regs + UDMA_AXI_S2M_DATA_WR_CFG_1);
	tmp &= ~(UDMA_AXI_S2M_DESC_WR_CFG_1_MIN_AXI_BEATS_MASK |
		 UDMA_AXI_S2M_DESC_WR_CFG_1_MAX_AXI_BEATS_MASK);
	tmp |= burst_size << UDMA_AXI_S2M_DESC_WR_CFG_1_MIN_AXI_BEATS_SHIFT;
	tmp |= burst_size << UDMA_AXI_S2M_DESC_WR_CFG_1_MAX_AXI_BEATS_SHIFT;
	writel(tmp, &udma->udma_regs + UDMA_AXI_S2M_DATA_WR_CFG_1);

	return 0;
}
