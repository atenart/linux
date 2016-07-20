/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __AL_HW_EC_REG_H
#define __AL_HW_EC_REG_H

/* Selection between descriptor caching options (WORD selection) */
#define EC_GEN_EN_EXT_CACHE_WORD_SPLIT   BIT(20)

/* Drop indication for the selected protocol index */
#define EC_EPE_A_PROT_ACT_DROP           BIT(0)

/* Enable SIP/DIP swap if SIP<DIP */
#define EC_RFW_THASH_CFG_1_ENABLE_IP_SWAP BIT(16)
/* Enable PORT swap if SPORT<DPORT */
#define EC_RFW_THASH_CFG_1_ENABLE_PORT_SWAP BIT(17)

/* Selects how to calculate the L3 header length when L3 is IpPv */
#define EC_RFW_META_L3_LEN_CALC          BIT(4)

/* Number of MetaData at the end of the packet1 - One MetaData b */
#define EC_RFW_OUT_CFG_META_CNT_MASK     0x00000003
/* Enable packet drop */
#define EC_RFW_OUT_CFG_DROP_EN           BIT(2)

/* Select the header that will be used for the checksum when a t */
#define EC_RFW_CHECKSUM_HDR_SEL          BIT(1)

/* Default data selection 0 - Default value 1 - Table data out */
#define EC_RFW_CTRL_TABLE_DEF_SEL        BIT(20)

/* Drop indication */
#define EC_FWD_MAC_CTRL_RX_VAL_DROP		BIT(0)

/* UDMA selection */
#define EC_FWD_MAC_CTRL_RX_VAL_UDMA_MASK	0x000000078
#define EC_FWD_MAC_CTRL_RX_VAL_UDMA_SHIFT	3

/* queue number */
#define EC_FWD_MAC_CTRL_RX_VAL_QID_MASK		0x00000180
#define EC_FWD_MAC_CTRL_RX_VAL_QID_SHIFT	7

/* Entry is valid for Rx forwarding engine. */
#define EC_FWD_MAC_CTRL_RX_VALID         BIT(15)
/* Control value for Tx forwarding engine */
#define EC_FWD_MAC_CTRL_TX_VAL_MASK      0x001F0000
#define EC_FWD_MAC_CTRL_TX_VAL_SHIFT     16
/* Entry is valid for Tx forwarding engine. */
#define EC_FWD_MAC_CTRL_TX_VALID         BIT(31)

/* MSS selection option:0 - MSS value is selected using MSS_sel  */
#define EC_TSO_CFG_ADD_0_MSS_SEL         BIT(0)

/* Enable TSO with tunnelling */
#define EC_TSO_CFG_TUNNEL_EN_TUNNEL_TSO  BIT(0)
/* Enable outer UDP checksum update */
#define EC_TSO_CFG_TUNNEL_EN_UDP_CHKSUM  BIT(8)
/* Enable outer UDP length update */
#define EC_TSO_CFG_TUNNEL_EN_UDP_LEN     BIT(9)
/* Enable outer Ip6  length update */
#define EC_TSO_CFG_TUNNEL_EN_IPV6_PLEN   BIT(10)
/* Enable outer IPv4 checksum update */
#define EC_TSO_CFG_TUNNEL_EN_IPV4_CHKSUM BIT(11)
/* Enable outer IPv4 Identification update */
#define EC_TSO_CFG_TUNNEL_EN_IPV4_IDEN   BIT(12)
/* Enable outer IPv4 length update */
#define EC_TSO_CFG_TUNNEL_EN_IPV4_TLEN   BIT(13)

/* Swap output byte order */
#define EC_TMI_TX_CFG_SWAP_BYTES         BIT(0)
/* Enable forwarding to the Rx data path. */
#define EC_TMI_TX_CFG_EN_FWD_TO_RX       BIT(1)
/* Mask 2 for XOFF [7:0] Mask 2 for sampled Almost Full indicati */
#define EC_EFC_EC_XOFF_MASK_2_SHIFT      8

/* Mask 1 for generating XON pulse, masking XOFF [0] */
#define EC_EFC_XON_MASK_1                BIT(0)
/* Mask 2 for generating XON pulse, masking Almost Full indicati */
#define EC_EFC_XON_MASK_2                BIT(1)

/* Threshold high */
#define EC_EFC_RX_FIFO_HYST_TH_HIGH_SHIFT 16

/* Use Ethernet controller Tx FIFO empty status for EEE control */
#define EC_EEE_CFG_E_USE_EC_TX_FIFO      BIT(2)
/* Use Ethernet controller Rx FIFO empty status for EEE control */
#define EC_EEE_CFG_E_USE_EC_RX_FIFO      BIT(3)
/* Enable Low power signalling. */
#define EC_EEE_CFG_E_ENABLE              BIT(4)
/* Mask output to MAC.  */
#define EC_EEE_CFG_E_MASK_MAC_EEE        BIT(8)
/* Mask output to stop MAC interface. */
#define EC_EEE_CFG_E_MASK_EC_TMI_STOP    BIT(9)

#endif /* __AL_HW_EC_REG_H */
