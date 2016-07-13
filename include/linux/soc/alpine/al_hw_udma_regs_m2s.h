/*
 * Copyright (C) 2015, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __AL_HW_UDMA_M2S_REG_H
#define __AL_HW_UDMA_M2S_REG_H

#include <linux/types.h>

/* Completion control */
#define UDMA_M2S_STATE_COMP_CTRL_MASK 0x00000003
#define UDMA_M2S_STATE_COMP_CTRL_SHIFT 0
/* Stream interface */
#define UDMA_M2S_STATE_STREAM_IF_MASK 0x00000030
#define UDMA_M2S_STATE_STREAM_IF_SHIFT 4
/* Data read control */
#define UDMA_M2S_STATE_DATA_RD_CTRL_MASK 0x00000300
#define UDMA_M2S_STATE_DATA_RD_CTRL_SHIFT 8
/* Descriptor prefetch */
#define UDMA_M2S_STATE_DESC_PREF_MASK 0x00003000
#define UDMA_M2S_STATE_DESC_PREF_SHIFT 12

/* Start normal operation */
#define UDMA_M2S_CHANGE_STATE_NORMAL BIT(0)
/* Stop normal operation */
#define UDMA_M2S_CHANGE_STATE_DIS    BIT(1)
/*
 * Stop all machines.
 * (Prefetch, scheduling, completion and stream interface)
 */
#define UDMA_M2S_CHANGE_STATE_ABORT  BIT(2)

/* Maximum packet size for the M2S */
#define UDMA_M2S_CFG_LEN_MAX_PKT_SIZE_MASK 0x000FFFFF
/*
 * Length encoding for 64K.
 * 0 - length 0x0000 = 0
 * 1 - length 0x0000 = 64k
 */
#define UDMA_M2S_CFG_LEN_ENCODE_64K  BIT(24)

/* Maximum number of descriptors per packet */
#define UDMA_M2S_RD_DESC_PREF_CFG_2_MAX_DESC_PER_PKT_MASK 0x0000001F
#define UDMA_M2S_RD_DESC_PREF_CFG_2_MAX_DESC_PER_PKT_SHIFT 0
/*
 * Minimum descriptor burst size when prefetch FIFO level is above the
 * descriptor prefetch threshold
 */
#define UDMA_M2S_RD_DESC_PREF_CFG_3_MIN_BURST_ABOVE_THR_MASK 0x000000F0
#define UDMA_M2S_RD_DESC_PREF_CFG_3_MIN_BURST_ABOVE_THR_SHIFT 4
/*
 * Descriptor fetch threshold.
 * Used as a threshold to determine the allowed minimum descriptor burst size.
 * (Must be at least max_desc_per_pkt)
 */
#define UDMA_M2S_RD_DESC_PREF_CFG_3_PREF_THR_MASK 0x0000FF00
#define UDMA_M2S_RD_DESC_PREF_CFG_3_PREF_THR_SHIFT 8

/*
 * Maximum number of data beats in the data read FIFO.
 * Defined based on data FIFO size
 * (default FIFO size 2KB → 128 beats)
 */
#define UDMA_M2S_RD_DATA_CFG_DATA_FIFO_DEPTH_MASK 0x000003FF
#define UDMA_M2S_RD_DATA_CFG_DATA_FIFO_DEPTH_SHIFT 0

/*
 * Enable operation of this queue.
 * Start prefetch.
 */
#define UDMA_M2S_Q_CFG_EN_PREF       BIT(16)
/*
 * Enable operation of this queue.
 * Start scheduling.
 */
#define UDMA_M2S_Q_CFG_EN_SCHEDULING BIT(17)

/*
 * M2S Descriptor Ring Base address [31:4].
 * Value of the base address of the M2S descriptor ring
 * [3:0] - 0 - 16B alignment is enforced
 * ([11:4] should be 0 for 4KB alignment)
 */
#define UDMA_M2S_Q_TDRBP_LOW_ADDR_MASK 0xFFFFFFF0

/*
 * M2S Descriptor Ring Base address [31:4].
 * Value of the base address of the M2S descriptor ring
 * [3:0] - 0 - 16B alignment is enforced
 * ([11:4] should be 0 for 4KB alignment)
 * NOTE:
 * Length of the descriptor ring (in descriptors) associated with the ring base
 * address. Ends at maximum burst size alignment.
 */
#define UDMA_M2S_Q_TCRBP_LOW_ADDR_MASK 0xFFFFFFF0

/*
 * Mask the internal pause mechanism for DMB.
 * (Data Memory Barrier).
 */
#define UDMA_M2S_Q_RATE_LIMIT_MASK_INTERNAL_PAUSE_DMB BIT(2)

/* Enable writing to the completion ring */
#define UDMA_M2S_Q_COMP_CFG_EN_COMP_RING_UPDATE BIT(0)
/* Disable the completion coalescing function. */
#define UDMA_M2S_Q_COMP_CFG_DIS_COMP_COAL BIT(1)

#endif /* __AL_HW_UDMA_M2S_REG_H */
