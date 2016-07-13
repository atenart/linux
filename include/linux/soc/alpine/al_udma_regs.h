/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef AL_UDMA_REGS_H
#define AL_UDMA_REGS_H

#define UDMA_AXI_S2M			0x0100

#define UDMA_AXI_S2M_DATA_WR_CFG_1	(UDMA_AXI_S2M + 0x00)

#define UDMA_M2S			0x0200

#define UDMA_S2M			0x0200

#define UDMA_M2S_STATE			(UDMA_M2S + 0x00)
#define UDMA_M2S_CHANGE_STATE		(UDMA_M2S + 0x04)
#define UDMA_M2S_CFG_LEN		(UDMA_M2S + 0x54)

#define UDMA_S2M_STATE			(UDMA_S2M + 0x00)
#define UDMA_S2M_CHANGE_STATE		(UDMA_S2M + 0x04)

#define UDMA_M2S_RD			0x0300

#define UDMA_S2M_RD			0x0300

#define UDMA_M2S_RD_DESC_PREF_CFG_2	(UDMA_M2S_RD + 0x04)
#define UDMA_M2S_RD_DESC_PREF_CFG_3	(UDMA_M2S_RD + 0x08)
#define UDMA_M2S_RD_DATA_CFG		(UDMA_M2S_RD + 0x10)

#define UDMA_S2M_RD_DESC_PREF_CFG_3	(UDMA_S2M_RD + 0x08)

#define UDMA_S2M_COMP			0x0380

#define UDMA_S2M_COMP_CFG_1C		(UDMA_S2M_COMP + 0x00)
#define UDMA_S2M_COMP_CFG_APP_ACK	(UDMA_S2M_COMP + 0x0c)

#define UDMA_M2S_COMP			0x0400

#define UDMA_M2S_COMP_CFG_APP_ACK	(UDMA_M2S_COMP + 0x08)

/* MAX 4 elems */
#define UDMA_M2S_Q(x)			(0x1000 + (x) * 0x1000)

/* MAX 4 elems */
#define UDMA_S2M_Q(x)			(0x1000 + (x) * 0x1000)

#define UDMA_M2S_Q_MASK			0x74
#define UDMA_M2S_Q_COMP_CFG		0xa0

#define UDMA_S2M_Q_COMP_CFG		0x54
#define UDMA_S2M_Q_COMP_CFG_2		0x58

#define UDMA_RINGS			0x0000

#define UDMA_RINGS_CFG			(UDMA_RINGS + 0x20)
#define UDMA_RINGS_DRBP_LOW		(UDMA_RINGS + 0x28)
#define UDMA_RINGS_DRBP_HIGH		(UDMA_RINGS + 0x2c)
#define UDMA_RINGS_DRL			(UDMA_RINGS + 0x30)
#define UDMA_RINGS_DRTP_INC		(UDMA_RINGS + 0x38)
#define UDMA_RINGS_CRBP_LOW		(UDMA_RINGS + 0x44)
#define UDMA_RINGS_CRBP_HIGH		(UDMA_RINGS + 0x48)
#define UDMA_RINGS_CRHP			(UDMA_RINGS + 0x4c)

#define UDMA_M2S_REGS			0x00000

#define UDMA_S2M_REGS			0x10000

#define UDMA_GEN_REGS			0x1c000

#define UDMA_GEN_IRQ_MAIN_IOFIC		(UDMA_GEN_REGS + 0x0000)

#define UDMA_GEN_IRQ_SECONDARY_IOFIC	(UDMA_GEN_REGS + 0x2000)

#define UDMA_GEN_DMA_MISC		(UDMA_GEN_REGS + 0x2080)

#define UDMA_GEN_DMA_MISC_REV		(UDMA_GEN_DMA_MISC + 0x04)

#define UDMA_GEN_AXI			(UDMA_GEN_REGS + 0x2280)

#define UDMA_GEN_AXI_CFG_1		(UDMA_GEN_AXI + 0x0000)

#endif
