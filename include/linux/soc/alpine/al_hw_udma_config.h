/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __AL_HW_UDMA_CONFIG_H__
#define __AL_HW_UDMA_CONFIG_H__

#include "al_hw_udma_regs.h"
#include "al_hw_udma.h"

/* M2S max packet size configuration */
struct al_udma_m2s_pkt_len_conf {
	u32 max_pkt_size;
	bool encode_64k_as_zero;
};

/* M2S DMA Rate Limitation mode */
struct al_udma_m2s_rlimit_mode {
	bool pkt_mode_en;
	u16 short_cycle_sz;
	u32 token_init_val;
};

enum al_udma_m2s_rlimit_action {
	AL_UDMA_STRM_RLIMIT_ENABLE,
	AL_UDMA_STRM_RLIMIT_PAUSE,
	AL_UDMA_STRM_RLIMIT_RESET
};

/* UDMA / UDMA Q rate limitation configuration */
struct al_udma_m2s_rlimit {
	struct al_udma_m2s_rlimit_mode rlimit_mode; /* rate limitation enablers */
};

/* Configure M2S packet len */
int al_udma_m2s_packet_size_cfg_set(struct al_udma *udma,
				    struct al_udma_m2s_pkt_len_conf *conf);

void al_udma_s2m_max_descs_set(struct al_udma *udma, u8 max_descs);
void al_udma_m2s_max_descs_set(struct al_udma *udma, u8 max_descs);

/* UDMA get revision */
static inline unsigned int al_udma_get_revision(
		void __iomem *unit_regs)
{
	return (readl(unit_regs + UDMA_GEN_DMA_MISC_REV)
			& UDMA_GEN_DMA_MISC_REVISION_PROGRAMMING_ID_MASK) >>
			UDMA_GEN_DMA_MISC_REVISION_PROGRAMMING_ID_SHIFT;
}

/*
 * S2M UDMA configure a queue's completion descriptors coalescing
 *
 * @param q_udma
 * @param enable set to true to enable completion coalescing
 * @param coal_timeout in South Bridge cycles.
 */
void al_udma_s2m_q_compl_coal_config(struct al_udma_q *udma_q, bool enable,
				     u32 coal_timeout);

/*
 * S2M UDMA configure completion descriptors write burst parameters
 *
 * @param udma
 * @param burst_size completion descriptors write burst size in bytes.
 *
 * @return 0 if no error found.
 */
int al_udma_s2m_compl_desc_burst_config(struct al_udma *udma, u16 burst_size);

#endif /* __AL_HW_UDMA_CONFIG_H__ */
