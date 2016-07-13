/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __AL_HW_UDMA_S2M_REG_H
#define __AL_HW_UDMA_S2M_REG_H

/*
 * Defines the maximum number of AXI beats for a single AXI burst. This value is
 * used for the burst split decision.
 */
#define UDMA_AXI_S2M_DESC_WR_CFG_1_MAX_AXI_BEATS_MASK 0x000000FF
#define UDMA_AXI_S2M_DESC_WR_CFG_1_MAX_AXI_BEATS_SHIFT 0
/*
 * Minimum burst for writing completion descriptors.
 * (AXI beats).
 * Value must be aligned to cache lines (64 bytes).
 * Default value is 2 cache lines, 8 beats.
 */
#define UDMA_AXI_S2M_DESC_WR_CFG_1_MIN_AXI_BEATS_MASK 0x00FF0000
#define UDMA_AXI_S2M_DESC_WR_CFG_1_MIN_AXI_BEATS_SHIFT 16

/*
 * Minimum descriptor burst size when prefetch FIFO level is above the
 * descriptor prefetch threshold
 */
#define UDMA_S2M_RD_DESC_PREF_CFG_3_MIN_BURST_ABOVE_THR_MASK 0x000000F0
#define UDMA_S2M_RD_DESC_PREF_CFG_3_MIN_BURST_ABOVE_THR_SHIFT 4
/*
 * Descriptor fetch threshold.
 * Used as a threshold to determine the allowed minimum descriptor burst size.
 * (Must be at least "max_desc_per_pkt")
 */
#define UDMA_S2M_RD_DESC_PREF_CFG_3_PREF_THR_MASK 0x0000FF00
#define UDMA_S2M_RD_DESC_PREF_CFG_3_PREF_THR_SHIFT 8

/*
 * Completion descriptor size.
 * (words)
 */
#define UDMA_S2M_COMP_CFG_1C_DESC_SIZE_MASK 0x0000000F

/* Disables the completion coalescing function. */
#define UDMA_S2M_Q_COMP_CFG_DIS_COMP_COAL BIT(1)

#endif /* __AL_HW_UDMA_S2M_REG_H */
