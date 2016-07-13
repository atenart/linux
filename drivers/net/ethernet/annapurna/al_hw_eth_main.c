/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/soc/alpine/iofic.h>
#include <linux/soc/alpine/al_hw_udma_iofic.h>
#include <linux/soc/alpine/al_hw_udma_config.h>

#include "al_eth_regs.h"
#include "al_hw_eth.h"
#include "al_hw_eth_ec_regs.h"
#include "al_hw_eth_mac_regs.h"
#include "al_hw_unit_adapter_regs.h"

#define AL_ADDR_LOW(x)	((u32)((dma_addr_t)(x)))
#define AL_ADDR_HIGH(x)	((u32)((((dma_addr_t)(x)) >> 16) >> 16))

#define AL_ETH_TX_PKT_UDMA_FLAGS	(AL_ETH_TX_FLAGS_NO_SNOOP | \
					 AL_ETH_TX_FLAGS_INT)

#define AL_ETH_TX_PKT_META_FLAGS	(AL_ETH_TX_FLAGS_IPV4_L3_CSUM | \
					 AL_ETH_TX_FLAGS_L4_CSUM |	\
					 AL_ETH_TX_FLAGS_L4_PARTIAL_CSUM |	\
					 AL_ETH_TX_FLAGS_L2_MACSEC_PKT | \
					 AL_ETH_TX_FLAGS_L2_DIS_FCS |\
					 AL_ETH_TX_FLAGS_TSO |\
					 AL_ETH_TX_FLAGS_TS)

#define AL_ETH_TX_SRC_VLAN_CNT_SHIFT		5
#define AL_ETH_TX_L4_PROTO_IDX_SHIFT		8
#define AL_ETH_TX_TUNNEL_MODE_SHIFT		18
#define AL_ETH_TX_OUTER_L3_PROTO_SHIFT		20
#define AL_ETH_TX_VLAN_MOD_ADD_SHIFT		22
#define AL_ETH_TX_VLAN_MOD_DEL_SHIFT		24
#define AL_ETH_TX_VLAN_MOD_E_SEL_SHIFT		26
#define AL_ETH_TX_VLAN_MOD_VID_SEL_SHIFT	28
#define AL_ETH_TX_VLAN_MOD_PBIT_SEL_SHIFT	30

/* tx Meta Descriptor defines */
#define AL_ETH_TX_META_STORE			BIT(21)
#define AL_ETH_TX_META_L3_LEN_MASK		0xff
#define AL_ETH_TX_META_L3_OFF_MASK		0xff
#define AL_ETH_TX_META_L3_OFF_SHIFT		8
#define AL_ETH_TX_META_MSS_LSB_VAL_SHIFT	22
#define AL_ETH_TX_META_MSS_MSB_TS_VAL_SHIFT	16
#define AL_ETH_TX_META_OUTER_L3_LEN_MASK	0x1f
#define AL_ETH_TX_META_OUTER_L3_LEN_SHIFT	24
#define AL_ETH_TX_META_OUTER_L3_OFF_HIGH_MASK	0x18
#define AL_ETH_TX_META_OUTER_L3_OFF_HIGH_SHIFT	10
#define AL_ETH_TX_META_OUTER_L3_OFF_LOW_MASK	0x07
#define AL_ETH_TX_META_OUTER_L3_OFF_LOW_SHIFT	29

/* Rx Descriptor defines */
#define AL_ETH_RX_L3_PROTO_IDX_MASK	0x1F
#define AL_ETH_RX_L4_PROTO_IDX_MASK	0x1F
#define AL_ETH_RX_L4_PROTO_IDX_SHIFT	8

#define AL_ETH_RX_L3_OFFSET_SHIFT	9
#define AL_ETH_RX_L3_OFFSET_MASK	(0x7f << AL_ETH_RX_L3_OFFSET_SHIFT)
#define AL_ETH_RX_HASH_SHIFT		16
#define AL_ETH_RX_HASH_MASK		(0xffff		<< AL_ETH_RX_HASH_SHIFT)

#define AL_ETH_MDIO_DELAY_PERIOD	1 /* micro seconds to wait when polling mdio status */
#define AL_ETH_MDIO_DELAY_COUNT		150 /* number of times to poll */
#define AL_ETH_S2M_UDMA_COMP_COAL_TIMEOUT	200 /* Rx descriptors coalescing timeout in SB clocks */

#define AL_ETH_EPE_ENTRIES_NUM 26
static struct al_eth_epe_p_reg_entry al_eth_epe_p_regs[AL_ETH_EPE_ENTRIES_NUM] = {
	{ 0x0, 0x0, 0x0 },
	{ 0x0, 0x0, 0x1 },
	{ 0x0, 0x0, 0x2 },
	{ 0x0, 0x0, 0x3 },
	{ 0x18100, 0xFFFFF, 0x80000004 },
	{ 0x188A8, 0xFFFFF, 0x80000005 },
	{ 0x99100, 0xFFFFF, 0x80000006 },
	{ 0x98100, 0xFFFFF, 0x80000007 },
	{ 0x10800, 0x7FFFF, 0x80000008 },
	{ 0x20000, 0x73FFF, 0x80000009 },
	{ 0x20000, 0x70000, 0x8000000A },
	{ 0x186DD, 0x7FFFF, 0x8000000B },
	{ 0x30600, 0x7FF00, 0x8000000C },
	{ 0x31100, 0x7FF00, 0x8000000D },
	{ 0x32F00, 0x7FF00, 0x8000000E },
	{ 0x32900, 0x7FF00, 0x8000000F },
	{ 0x105DC, 0x7FFFF, 0x80010010 },
	{ 0x188E5, 0x7FFFF, 0x80000011 },
	{ 0x72000, 0x72000, 0x80000012 },
	{ 0x70000, 0x72000, 0x80000013 },
	{ 0x46558, 0x7FFFF, 0x80000001 },
	{ 0x18906, 0x7FFFF, 0x80000015 },
	{ 0x18915, 0x7FFFF, 0x80000016 },
	{ 0x31B00, 0x7FF00, 0x80000017 },
	{ 0x30400, 0x7FF00, 0x80000018 },
	{ 0x0, 0x0, 0x8000001F }
};

static struct al_eth_epe_control_entry al_eth_epe_control_table[AL_ETH_EPE_ENTRIES_NUM] = {
	{ { 0x2800000, 0x0, 0x0, 0x0, 0x1, 0x400000 } },
	{ { 0x280004C, 0x746000, 0xA46030, 0xE00000, 0x2, 0x400000 } },
	{ { 0x2800054, 0x746000, 0xA46030, 0x1600000, 0x2, 0x400000 } },
	{ { 0x280005C, 0x746000, 0xA46030, 0x1E00000, 0x2, 0x400000 } },
	{ { 0x2800042, 0xD42000, 0x0, 0x400000, 0x1010412, 0x400000 } },
	{ { 0x2800042, 0xD42000, 0x0, 0x400000, 0x1010412, 0x400000 } },
	{ { 0x2800042, 0xE42000, 0x0, 0x400000, 0x2020002, 0x400000 } },
	{ { 0x2800042, 0xE42000, 0x0, 0x400000, 0x2020002, 0x400000 } },
	{ { 0x280B046, 0x0, 0x6C1008, 0x0, 0x4, 0x406800 } },
	{ { 0x2800049, 0xF44060, 0x1744080, 0x14404, 0x6, 0x400011 } },
	{ { 0x2015049, 0xF44060, 0x1744080, 0x14404, 0x8080007, 0x400011 } },
	{ { 0x280B046, 0xF60040, 0x6C1004, 0x2800000, 0x6, 0x406811 } },
	{ { 0x2815042, 0x1F42000, 0x2042010, 0x1414460, 0x10100009, 0x40B800 } },
	{ { 0x2815042, 0x1F42000, 0x2042010, 0x800000, 0x10100009, 0x40B800 } },
	{ { 0x280B042, 0x0, 0x0, 0x430400, 0x4040009, 0x0 } },
	{ { 0x2815580, 0x0, 0x0, 0x0, 0x4040005, 0x0 } },
	{ { 0x280B000, 0x0, 0x0, 0x0, 0x1, 0x400000 } },
	{ { 0x2800040, 0x174E000, 0x0, 0x0, 0xE, 0x406800 } },
	{ { 0x280B000, 0x0, 0x0, 0x600000, 0x1, 0x406800 } },
	{ { 0x280B000, 0x0, 0x0, 0xE00000, 0x1, 0x406800 } },
	{ { 0x2800000, 0x0, 0x0, 0x0, 0x1, 0x400000 } },
	{ { 0x280B046, 0x0, 0x0, 0x2800000, 0x7, 0x400000 } },
	{ { 0x280B046, 0xF60040, 0x6C1004, 0x2800000, 0x6, 0x406811 } },
	{ { 0x2815042, 0x1F43028, 0x2000000, 0xC00000, 0x10100009, 0x40B800 } },
	{ { 0x2815400, 0x0, 0x0, 0x0, 0x4040005, 0x0 } },
	{ { 0x2800000, 0x0, 0x0, 0x0, 0x1, 0x400000 } }
};

#define AL_ETH_IS_1G_MAC(mac_mode) (((mac_mode) == AL_ETH_MAC_MODE_RGMII) || ((mac_mode) == AL_ETH_MAC_MODE_SGMII))
#define AL_ETH_IS_10G_MAC(mac_mode)	(((mac_mode) == AL_ETH_MAC_MODE_10GbE_Serial) ||	\
					((mac_mode) == AL_ETH_MAC_MODE_10G_SGMII) ||		\
					((mac_mode) == AL_ETH_MAC_MODE_SGMII_2_5G))
#define AL_ETH_IS_25G_MAC(mac_mode) ((mac_mode) == AL_ETH_MAC_MODE_KR_LL_25G)

static const char *al_eth_mac_mode_str(enum al_eth_mac_mode mode)
{
	switch (mode) {
	case AL_ETH_MAC_MODE_RGMII:
		return "RGMII";
	case AL_ETH_MAC_MODE_SGMII:
		return "SGMII";
	case AL_ETH_MAC_MODE_SGMII_2_5G:
		return "SGMII_2_5G";
	case AL_ETH_MAC_MODE_10GbE_Serial:
		return "KR";
	case AL_ETH_MAC_MODE_KR_LL_25G:
		return "KR_LL_25G";
	case AL_ETH_MAC_MODE_10G_SGMII:
		return "10G_SGMII";
	case AL_ETH_MAC_MODE_XLG_LL_40G:
		return "40G_LL";
	case AL_ETH_MAC_MODE_XLG_LL_50G:
		return "50G_LL";
	case AL_ETH_MAC_MODE_XLG_LL_25G:
		return "25G_LL";
	default:
		return "N/A";
	}
}

/*
 * change and wait udma state
 *
 * @param dma the udma to change its state
 * @param new_state
 *
 * @return 0 on success. otherwise on failure.
 */
static int al_udma_state_set_wait(struct al_hw_eth_adapter *adapter,
				  struct al_udma *dma,
				  enum al_udma_state new_state)
{
	enum al_udma_state state;
	enum al_udma_state expected_state = new_state;
	int count = 1000;

	al_udma_state_set(dma, new_state);

	if ((new_state == UDMA_NORMAL) || (new_state == UDMA_DISABLE))
		expected_state = UDMA_IDLE;

	do {
		state = al_udma_state_get(dma);
		if (state == expected_state)
			break;
		udelay(1);
		if (count-- == 0) {
			netdev_warn(adapter->netdev,
				    "[%s] warn: dma state didn't change to %s\n",
				    dma->name, al_udma_states_name[new_state]);
			return -ETIMEDOUT;
		}
	} while (1);
	return 0;
}

static void al_eth_epe_entry_set(struct al_hw_eth_adapter *adapter, u32 idx,
				 struct al_eth_epe_p_reg_entry *reg_entry,
				 struct al_eth_epe_control_entry *control_entry)
{
	writel(reg_entry->data, adapter->ec_regs_base + AL_EC_EPE_P(idx) +
	       AL_EC_EPE_P_COMP_DATA);
	writel(reg_entry->mask, adapter->ec_regs_base + AL_EC_EPE_P(idx) +
	       AL_EC_EPE_P_COMP_MASK);
	writel(reg_entry->ctrl, adapter->ec_regs_base + AL_EC_EPE_P(idx) +
	       AL_EC_EPE_P_COMP_CTRL);

	writel(reg_entry->data, adapter->ec_regs_base + AL_EC_MSP_C(idx) +
	       AL_EC_MSP_C_P_COMP_DATA);
	writel(reg_entry->mask, adapter->ec_regs_base + AL_EC_MSP_C(idx) +
	       AL_EC_MSP_C_P_COMP_MASK);
	writel(reg_entry->ctrl, adapter->ec_regs_base + AL_EC_MSP_C(idx) +
	       AL_EC_MSP_C_P_COMP_CTRL);

	/*control table  0*/
	writel(idx, adapter->ec_regs_base + AL_EC_EPE(0) +
	       AL_EC_EPE_ACT_TABLE_ADDR);
	writel(control_entry->data[5], adapter->ec_regs_base + AL_EC_EPE(0) +
	       AL_EC_EPE_ACT_TABLE_DATA_6);
	writel(control_entry->data[1], adapter->ec_regs_base + AL_EC_EPE(0) +
	       AL_EC_EPE_ACT_TABLE_DATA_2);
	writel(control_entry->data[2], adapter->ec_regs_base + AL_EC_EPE(0) +
	       AL_EC_EPE_ACT_TABLE_DATA_3);
	writel(control_entry->data[3], adapter->ec_regs_base + AL_EC_EPE(0) +
	       AL_EC_EPE_ACT_TABLE_DATA_4);
	writel(control_entry->data[4], adapter->ec_regs_base + AL_EC_EPE(0) +
	       AL_EC_EPE_ACT_TABLE_DATA_5);
	writel(control_entry->data[0], adapter->ec_regs_base + AL_EC_EPE(0) +
	       AL_EC_EPE_ACT_TABLE_DATA_1);

	/*control table 1*/
	writel(idx, adapter->ec_regs_base + AL_EC_EPE(1) +
	       AL_EC_EPE_ACT_TABLE_ADDR);
	writel(control_entry->data[5], adapter->ec_regs_base + AL_EC_EPE(1) +
	       AL_EC_EPE_ACT_TABLE_DATA_6);
	writel(control_entry->data[1], adapter->ec_regs_base + AL_EC_EPE(1) +
	       AL_EC_EPE_ACT_TABLE_DATA_2);
	writel(control_entry->data[2], adapter->ec_regs_base + AL_EC_EPE(1) +
	       AL_EC_EPE_ACT_TABLE_DATA_3);
	writel(control_entry->data[3], adapter->ec_regs_base + AL_EC_EPE(1) +
	       AL_EC_EPE_ACT_TABLE_DATA_4);
	writel(control_entry->data[4], adapter->ec_regs_base + AL_EC_EPE(1) +
	       AL_EC_EPE_ACT_TABLE_DATA_5);
	writel(control_entry->data[0], adapter->ec_regs_base + AL_EC_EPE(1) +
	       AL_EC_EPE_ACT_TABLE_DATA_1);
}

static void al_eth_epe_init(struct al_hw_eth_adapter *adapter)
{
	int idx;

	if (adapter->enable_rx_parser == 0) {
		netdev_dbg(adapter->netdev, "eth [%s]: disable rx parser\n",
			   adapter->name);

		writel(0x08000000, adapter->ec_regs_base + AL_EC_EPE(0) +
		       AL_EC_EPE_RES_DEF);
		writel(0x7, adapter->ec_regs_base + AL_EC_EPE(0) +
		       AL_EC_EPE_RES_IN);

		writel(0x08000000, adapter->ec_regs_base + AL_EC_EPE(1) +
		       AL_EC_EPE_RES_DEF);
		writel(0x7, adapter->ec_regs_base + AL_EC_EPE(1) +
		       AL_EC_EPE_RES_IN);

		return;
	}

	for (idx = 0; idx < AL_ETH_EPE_ENTRIES_NUM; idx++)
		al_eth_epe_entry_set(adapter, idx, &al_eth_epe_p_regs[idx],
				     &al_eth_epe_control_table[idx]);

	writel(0x08000080, adapter->ec_regs_base + AL_EC_EPE(0) +
	       AL_EC_EPE_RES_DEF);
	writel(0x7, adapter->ec_regs_base + AL_EC_EPE(0) + AL_EC_EPE_RES_IN);

	writel(0x08000080, adapter->ec_regs_base + AL_EC_EPE(1) +
	       AL_EC_EPE_RES_DEF);
	writel(0, adapter->ec_regs_base + AL_EC_EPE(1) + AL_EC_EPE_RES_IN);

	/*
	 * header length as function of 4 bits value, for GRE, when C bit
	 * is set, the header len should be increase by 4
	 */
	writel((4 << 16) | 4, adapter->ec_regs_base + AL_EC_EPE_H_HDR_LEN(8));

	/*
	 * select the outer information when writing the rx descriptor
	 * (l3 protocol index etc)
	 */
	writel(EC_RFW_META_L3_LEN_CALC, adapter->ec_regs_base + AL_EC_RFW_META);

	writel(EC_RFW_CHECKSUM_HDR_SEL, adapter->ec_regs_base +
	       AL_EC_RFW_CHECKSUM);
}

/*
 * read 40G MAC registers (indirect access)
 *
 * @param adapter pointer to the private structure
 * @param reg_addr address in the an registers
 *
 * @return the register value
 */
static u32 al_eth_40g_mac_reg_read(struct al_hw_eth_adapter *adapter,
				   u32 reg_addr)
{
	u32 val;

	/* indirect access */
	writel(reg_addr, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_ADDR);
	val = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_DATA);

	return val;
}

/*
 * write 40G MAC registers (indirect access)
 *
 * @param adapter pointer to the private structure
 * @param reg_addr address in the an registers
 * @param reg_data value to write to the register
 *
 */
static void al_eth_40g_mac_reg_write(struct al_hw_eth_adapter *adapter,
				     u32 reg_addr, u32 reg_data)
{
	/* indirect access */
	writel(reg_addr, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_ADDR);
	writel(reg_data, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_DATA);
}

/*
 * write 40G PCS registers (indirect access)
 *
 * @param adapter pointer to the private structure
 * @param reg_addr address in the an registers
 * @param reg_data value to write to the register
 *
 */
static void al_eth_40g_pcs_reg_write(struct al_hw_eth_adapter *adapter,
				     u32 reg_addr, u32 reg_data)
{
	/* indirect access */
	writel(reg_addr, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_40G_LL_ADDR);
	writel(reg_data, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_40G_LL_DATA);
}

/*
 * initialize the ethernet adapter's DMA
 */
int al_eth_adapter_init(struct al_hw_eth_adapter *adapter,
			struct al_eth_adapter_params *params)
{
	struct al_udma_params udma_params;
	struct al_udma_m2s_pkt_len_conf conf;
	int i;
	u32 reg;
	int rc;

	netdev_dbg(adapter->netdev,
		   "eth [%s]: initialize controller's UDMA. id = %d\n",
		   params->name, params->udma_id);
	netdev_dbg(adapter->netdev, "eth [%s]: enable_rx_parser: %x\n",
		   params->name, params->enable_rx_parser);

	adapter->name = params->name;
	adapter->rev_id = params->rev_id;
	adapter->netdev = params->netdev;
	adapter->udma_id = params->udma_id;
	adapter->udma_regs_base = params->udma_regs_base;
	adapter->ec_regs_base = (void __iomem *)params->ec_regs_base;
	adapter->mac_regs_base =
		(void __iomem *)params->mac_regs_base;
	adapter->unit_regs = params->udma_regs_base;
	adapter->enable_rx_parser = params->enable_rx_parser;
	adapter->ec_ints_base = (u8 __iomem *)adapter->ec_regs_base + 0x1c00;
	adapter->mac_ints_base = (struct interrupt_controller_ctrl __iomem *)
				 ((u8 __iomem *)adapter->mac_regs_base + 0x800);

	/* initialize Tx udma */
	udma_params.dev = adapter->netdev->dev.parent;
	udma_params.udma_regs_base = adapter->unit_regs;
	udma_params.type = UDMA_TX;
	udma_params.cdesc_size = AL_ETH_UDMA_TX_CDESC_SZ;
	udma_params.num_of_queues = AL_ETH_UDMA_TX_QUEUES;
	udma_params.name = "eth tx";
	rc = al_udma_init(&adapter->tx_udma, &udma_params);

	if (rc != 0) {
		netdev_err(adapter->netdev,
			   "failed to initialize %s, error %d\n",
			   udma_params.name, rc);
		return rc;
	}
	rc = al_udma_state_set_wait(adapter, &adapter->tx_udma, UDMA_NORMAL);
	if (rc != 0) {
		netdev_err(adapter->netdev,
			   "[%s]: failed to change state, error %d\n",
			   udma_params.name, rc);
		return rc;
	}
	/* initialize Rx udma */
	udma_params.dev = adapter->netdev->dev.parent;
	udma_params.udma_regs_base = adapter->unit_regs;
	udma_params.type = UDMA_RX;
	udma_params.cdesc_size = AL_ETH_UDMA_RX_CDESC_SZ;
	udma_params.num_of_queues = AL_ETH_UDMA_RX_QUEUES;
	udma_params.name = "eth rx";
	rc = al_udma_init(&adapter->rx_udma, &udma_params);

	if (rc != 0) {
		netdev_err(adapter->netdev,
			   "failed to initialize %s, error %d\n",
			   udma_params.name, rc);
		return rc;
	}

	rc = al_udma_state_set_wait(adapter, &adapter->rx_udma, UDMA_NORMAL);
	if (rc != 0) {
		netdev_err(adapter->netdev,
			   "[%s]: failed to change state, error %d\n",
			   udma_params.name, rc);
		return rc;
	}

	netdev_dbg(adapter->netdev,
		   "eth [%s]: controller's UDMA successfully initialized\n",
		   params->name);

	/* set max packet size to 1M (for TSO) */
	conf.encode_64k_as_zero = true;
	conf.max_pkt_size = 0xfffff;
	al_udma_m2s_packet_size_cfg_set(&adapter->tx_udma, &conf);

	/*
	 * Set m2s (tx) max descriptors to max data buffers number and one for
	 * meta descriptor
	 */
	al_udma_m2s_max_descs_set(&adapter->tx_udma, AL_ETH_PKT_MAX_BUFS + 1);

	/* set s2m (rx) max descriptors to max data buffers */
	al_udma_s2m_max_descs_set(&adapter->rx_udma, AL_ETH_PKT_MAX_BUFS);

	/*
	 * set s2m burst length when writing completion descriptors to
	 * 64 bytes
	 */
	al_udma_s2m_compl_desc_burst_config(&adapter->rx_udma, 64);

	/* if pointer to ec regs provided, then init the tx meta cache of this udma*/
	if (adapter->ec_regs_base) {
		/* INIT TX CACHE TABLE: */
		for (i = 0; i < 4; i++) {
			writel(i + (adapter->udma_id * 4),
			       adapter->ec_regs_base +
			       AL_EC_TSO_CACHE_TABLE_ADDR);
			writel(0x00000000, adapter->ec_regs_base +
			       AL_EC_TSO_CACHE_TABLE_DATA_1);
			writel(0x00000000, adapter->ec_regs_base +
			       AL_EC_TSO_CACHE_TABLE_DATA_2);
			writel(0x00000000, adapter->ec_regs_base +
			       AL_EC_TSO_CACHE_TABLE_DATA_3);
			writel(0x00000000, adapter->ec_regs_base +
			       AL_EC_TSO_CACHE_TABLE_DATA_4);
		}
	}
	/* only udma 0 allowed to init ec */
	if (adapter->udma_id != 0)
		return 0;

	/* enable internal machines*/
	writel(0xffffffff, adapter->ec_regs_base + AL_EC_GEN_EN);
	writel(0xffffffff, adapter->ec_regs_base + AL_EC_GEN_FIFO_EN);

	/* enable A0 descriptor structure */
	writel(readl(adapter->ec_regs_base + AL_EC_GEN_EN_EXTENDED_OPS) |
	       EC_GEN_EN_EXT_CACHE_WORD_SPLIT,
	       adapter->ec_regs_base + AL_EC_GEN_EN_EXTENDED_OPS);

	/* use mss value in the descriptor */
	writel(EC_TSO_CFG_ADD_0_MSS_SEL, adapter->ec_regs_base +
	       AL_EC_TSO_CFG_ADD_0);

	/* enable tunnel TSO */
	reg = EC_TSO_CFG_TUNNEL_EN_TUNNEL_TSO | EC_TSO_CFG_TUNNEL_EN_UDP_CHKSUM |
		EC_TSO_CFG_TUNNEL_EN_UDP_LEN | EC_TSO_CFG_TUNNEL_EN_IPV6_PLEN |
		EC_TSO_CFG_TUNNEL_EN_IPV4_CHKSUM | EC_TSO_CFG_TUNNEL_EN_IPV4_IDEN |
		EC_TSO_CFG_TUNNEL_EN_IPV4_TLEN;
	writel(reg, adapter->ec_regs_base + AL_EC_TSO_CFG_TUNNEL);

	/* swap input byts from MAC RX */
	writel(0x1, adapter->ec_regs_base + AL_EC_MAC_GEN);
	/* swap output bytes to MAC TX*/
	writel(EC_TMI_TX_CFG_EN_FWD_TO_RX | EC_TMI_TX_CFG_SWAP_BYTES,
	       adapter->ec_regs_base + AL_EC_TMI_TX_CFG);

	writel(0x3fb, adapter->ec_regs_base + AL_EC_TFW_UDMA(0) +
	       AL_EC_TFW_UDMA_FWD_DEC);

	/* RFW configuration: default 0 */
	writel(0x1, adapter->ec_regs_base + AL_EC_RFW_DEFAULT_OPT_1(0));

	/* VLAN table address */
	writel(0x0, adapter->ec_regs_base + AL_EC_RFW_VID_TABLE_ADDR);
	/* VLAN table data */
	writel(0x0, adapter->ec_regs_base + AL_EC_RFW_VID_TABLE_DATA);
	/*
	 * HASH config (select toeplitz and bits 7:0 of the thash result, enable
	 * symmetric hash)
	 */
	reg = EC_RFW_THASH_CFG_1_ENABLE_IP_SWAP | EC_RFW_THASH_CFG_1_ENABLE_PORT_SWAP;
	writel(reg, adapter->ec_regs_base + AL_EC_RFW_THASH_CFG_1);

	al_eth_epe_init(adapter);

	/* disable TSO padding and use mac padding instead */
	reg = readl(adapter->ec_regs_base + AL_EC_TSO_IN_CFG);
	reg &= ~0x7F00; /*clear bits 14:8 */
	writel(reg, adapter->ec_regs_base + AL_EC_TSO_IN_CFG);

	return 0;
}

/*
 * stop the DMA of the ethernet adapter
 */
int al_eth_adapter_stop(struct al_hw_eth_adapter *adapter)
{
	int rc;

	netdev_dbg(adapter->netdev, "eth [%s]: stop controller's UDMA\n",
		   adapter->name);

	/* disable Tx dma*/
	rc = al_udma_state_set_wait(adapter, &adapter->tx_udma, UDMA_DISABLE);
	if (rc != 0) {
		netdev_warn(adapter->netdev,
			    "[%s] warn: failed to change state, error %d\n",
			    adapter->tx_udma.name, rc);
		return rc;
	}

	netdev_dbg(adapter->netdev, "eth [%s]: controller's TX UDMA stopped\n",
		   adapter->name);

	/* disable Rx dma*/
	rc = al_udma_state_set_wait(adapter, &adapter->rx_udma, UDMA_DISABLE);
	if (rc != 0) {
		netdev_warn(adapter->netdev,
			    "[%s] warn: failed to change state, error %d\n",
			    adapter->rx_udma.name, rc);
		return rc;
	}

	netdev_dbg(adapter->netdev, "eth [%s]: controller's RX UDMA stopped\n",
		   adapter->name);
	return 0;
}

/* Q management */
/*
 * Configure and enable a queue ring
 */
int al_eth_queue_config(struct al_hw_eth_adapter *adapter,
			enum al_udma_type type, u32 qid,
			struct al_udma_q_params *q_params)
{
	struct al_udma *udma;
	int rc;

	netdev_dbg(adapter->netdev, "eth [%s]: config UDMA %s queue %d\n",
		   adapter->name, type == UDMA_TX ? "Tx" : "Rx", qid);

	udma = (type == UDMA_TX) ? &adapter->tx_udma : &adapter->rx_udma;
	q_params->adapter_rev_id = adapter->rev_id;

	rc = al_udma_q_init(udma, qid, q_params);
	if (rc)
		return rc;

	if (type == UDMA_RX)
		al_udma_s2m_q_compl_coal_config(&udma->udma_q[qid], true,
						AL_ETH_S2M_UDMA_COMP_COAL_TIMEOUT);

	return rc;
}

/* MAC layer */
int al_eth_rx_pkt_limit_config(struct al_hw_eth_adapter *adapter,
			       u32 min_rx_len, u32 max_rx_len)
{
	WARN_ON(AL_ETH_MAX_FRAME_LEN < max_rx_len);

	/* EC minimum packet length [bytes] in RX */
	writel(min_rx_len, adapter->ec_regs_base + AL_EC_MAC_MIN_PKT);
	/* EC maximum packet length [bytes] in RX */
	writel(max_rx_len, adapter->ec_regs_base + AL_EC_MAC_MAX_PKT);

	if (adapter->rev_id > AL_ETH_REV_ID_2) {
		writel(min_rx_len,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_1);
		writel(max_rx_len,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_2);
	}

	/*
	 * configure the MAC's max rx length, add 16 bytes so the packet get
	 * trimmed by the EC/Async_fifo rather by the MAC
	 */
	if (AL_ETH_IS_1G_MAC(adapter->mac_mode))
		writel(max_rx_len + 16,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_FRM_LEN);
	else if (AL_ETH_IS_10G_MAC(adapter->mac_mode) ||
		 AL_ETH_IS_25G_MAC(adapter->mac_mode))
		/* 10G MAC control register  */
		writel((max_rx_len + 16),
		       adapter->mac_regs_base + AL_ETH_MAC_10G_FRM_LEN);
	else
		al_eth_40g_mac_reg_write(adapter,
					 ETH_MAC_GEN_V3_MAC_40G_FRM_LENGTH_ADDR,
					 (max_rx_len + 16));

	return 0;
}

/* configure the mac media type. */
int al_eth_mac_config(struct al_hw_eth_adapter *adapter, enum al_eth_mac_mode mode)
{
	u32 tmp;

	switch (mode) {
	case AL_ETH_MAC_MODE_RGMII:
		writel(0x40003210, adapter->mac_regs_base + AL_ETH_MAC_GEN_CLK_CFG);

		/*
		 * 1G MAC control register
		 *
		 * bit[0]  - TX_ENA - zeroed by default. Should be asserted by al_eth_mac_start
		 * bit[1]  - RX_ENA - zeroed by default. Should be asserted by al_eth_mac_start
		 * bit[3]  - ETH_SPEED - zeroed to enable 10/100 Mbps Ethernet
		 * bit[4]  - PROMIS_EN - asserted to enable MAC promiscuous mode
		 * bit[23] - CNTL_FRM-ENA - asserted to enable control frames
		 * bit[24] - NO_LGTH_CHECK - asserted to disable length checks, which is done in the controller
		 */
		writel(0x01800010, adapter->mac_regs_base + AL_ETH_MAC_1G_CMD_CFG);

		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_RX_EMPTY);
		writel(0x0000000c,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_RX_FULL);
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_RX_ALMOST_EMPTY);
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_RX_ALMOST_FULL);

		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_TX_EMPTY);
		writel(0x0000000c,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_TX_FULL);
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_TX_ALMOST_EMPTY);
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_TX_ALMOST_FULL);

		writel(0x00000000, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);

		/*
		 * 1G MACSET 1G
		 * taking sel_1000/sel_10 inputs from rgmii PHY, and not from register.
		 * disabling magic_packets detection in mac
		 */
		writel(0x00000002, adapter->mac_regs_base + AL_ETH_MAC_GEN_MAC_1G_CFG);
		/* RGMII set 1G */
		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);
		tmp &= ETH_MAC_GEN_MUX_SEL_KR_IN_MASK;
		tmp |= 0x63910;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);
		writel(0xf, adapter->mac_regs_base + AL_ETH_MAC_GEN_RGMII_SEL);
		break;
	case AL_ETH_MAC_MODE_SGMII:
		if (adapter->rev_id > AL_ETH_REV_ID_2) {
			/*
			 * Configure and enable the ASYNC FIFO between the MACs
			 * and the EC
			 */
			/* TX min packet size */
			writel(0x00000010,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_1);
			/* TX max packet size */
			writel(0x00002800,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_2);
			/* TX input bus configuration */
			writel(0x00000080,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_3);
			/* TX output bus configuration */
			writel(0x00030020,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_4);
			/* TX Valid/ready configuration */
			writel(0x00000121,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_5);
			/* RX input bus configuration */
			writel(0x00030020,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_3);
			/* RX output bus configuration */
			writel(0x00000080,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_4);
			/* RX Valid/ready configuration */
			writel(0x00000212,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_5);
			/* V3 additional MAC selection */
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_SEL);
			writel(0x00000001,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CFG);
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CTRL);
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_10G_LL_CFG);
			/* ASYNC FIFO ENABLE */
			writel(0x00003333,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_AFIFO_CTRL);
			/* Timestamp_configuration */
			writel(ETH_MAC_GEN_V3_SPARE_CHICKEN_DISABLE_TIMESTAMP_STRETCH,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_SPARE);
		}

		writel(0x40053210, adapter->mac_regs_base + AL_ETH_MAC_GEN_CLK_CFG);

		/*
		 * 1G MAC control register
		 *
		 * bit[0]  - TX_ENA - zeroed by default. Should be asserted by al_eth_mac_start
		 * bit[1]  - RX_ENA - zeroed by default. Should be asserted by al_eth_mac_start
		 * bit[3]  - ETH_SPEED - zeroed to enable 10/100 Mbps Ethernet
		 * bit[4]  - PROMIS_EN - asserted to enable MAC promiscuous mode
		 * bit[23] - CNTL_FRM-ENA - asserted to enable control frames
		 * bit[24] - NO_LGTH_CHECK - asserted to disable length checks, which is done in the controller
		 */
		writel(0x01800010, adapter->mac_regs_base + AL_ETH_MAC_1G_CMD_CFG);

		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_RX_EMPTY);
		writel(0x0000000c,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_RX_FULL); /* must be larger than almost empty */
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_RX_ALMOST_EMPTY);
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_RX_ALMOST_FULL);

		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_TX_EMPTY); /* 8 ? */
		writel(0x0000000c,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_TX_FULL);
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_TX_ALMOST_EMPTY);
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_1G_TX_ALMOST_FULL);

		/* XAUI MAC control register */
		writel(0x000000c0, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);

		/*
		 * 1G MACSET 1G
		 * taking sel_1000/sel_10 inputs from rgmii_converter, and not from register.
		 * disabling magic_packets detection in mac
		 */
		writel(0x00000002, adapter->mac_regs_base + AL_ETH_MAC_GEN_MAC_1G_CFG);

		/* Setting PCS i/f mode to SGMII (instead of default 1000Base-X) */
		writel(0x00000014, adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_ADDR);
		writel(0x0000000b, adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_DATA);
		/* setting dev_ability to have speed of 1000Mb, [11:10] = 2'b10 */
		writel(0x00000004, adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_ADDR);
		writel(0x000009A0, adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_DATA);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		tmp &= ~ETH_MAC_GEN_LED_CFG_SEL_MASK;
		tmp |= ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		break;

	case AL_ETH_MAC_MODE_SGMII_2_5G:
		if (adapter->rev_id > AL_ETH_REV_ID_2) {
			/* configure and enable the ASYNC FIFO between the MACs and the EC */
			/* TX min packet size */
			writel(0x00000010,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_1);
			/* TX max packet size */
			writel(0x00002800,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_2);
			/* TX input bus configuration */
			writel(0x00000080,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_3);
			/* TX output bus configuration */
			writel(0x00030020,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_4);
			/* TX Valid/ready configuration */
			writel(0x00000023,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_5);
			/* RX input bus configuration */
			writel(0x00030020,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_3);
			/* RX output bus configuration */
			writel(0x00000080,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_4);
			/* RX Valid/ready configuration */
			writel(0x00000012,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_5);
			/* V3 additional MAC selection */
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_SEL);
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CFG);
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CTRL);
			writel(0x00000050,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_10G_LL_CFG);
			/* ASYNC FIFO ENABLE */
			writel(0x00003333,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_AFIFO_CTRL);
		}

		/* MAC register file */
		writel(0x01022830, adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
		/* XAUI MAC control register */
		writel(0x00000001, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);
		writel(0x00000028, adapter->mac_regs_base + AL_ETH_MAC_10G_IF_MODE);
		writel(0x00001140, adapter->mac_regs_base + AL_ETH_MAC_10G_CONTROL);
		/* RXAUI MAC control register */
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_32_64);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_64_32);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);
		tmp &= ETH_MAC_GEN_MUX_SEL_KR_IN_MASK;
		tmp |= 0x00063910;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);

		writel(0x40003210, adapter->mac_regs_base + AL_ETH_MAC_GEN_CLK_CFG);
		writel(0x000004f0, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);
		writel(0x00000401, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		tmp &= ~ETH_MAC_GEN_LED_CFG_SEL_MASK;
		tmp |= ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		break;

	case AL_ETH_MAC_MODE_10GbE_Serial:
		if (adapter->rev_id > AL_ETH_REV_ID_2) {
			/* configure and enable the ASYNC FIFO between the MACs and the EC */
			/* TX min packet size */
			writel(0x00000010,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_1);
			/* TX max packet size */
			writel(0x00002800,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_2);
			/* TX input bus configuration */
			writel(0x00000080,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_3);
			/* TX output bus configuration */
			writel(0x00030020,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_4);
			/* TX Valid/ready configuration */
			writel(0x00000023,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_5);
			/* RX input bus configuration */
			writel(0x00030020,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_3);
			/* RX output bus configuration */
			writel(0x00000080,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_4);
			/* RX Valid/ready configuration */
			writel(0x00000012,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_5);
			/* V3 additional MAC selection */
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_SEL);
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CFG);
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CTRL);
			writel(0x00000050,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_10G_LL_CFG);
			/* ASYNC FIFO ENABLE */
			writel(0x00003333,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_AFIFO_CTRL);
		}

		/* MAC register file */
		writel(0x01022810, adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
		/* XAUI MAC control register */
		writel(0x00000005, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);
		/* RXAUI MAC control register */
		writel(0x00000007, adapter->mac_regs_base + AL_ETH_MAC_GEN_RXAUI_CFG);
		writel(0x000001F1, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_CFG);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_32_64);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_64_32);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);
		tmp &= ETH_MAC_GEN_MUX_SEL_KR_IN_MASK;
		tmp |= 0x73910;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);

		writel(0x10003210, adapter->mac_regs_base + AL_ETH_MAC_GEN_CLK_CFG);
		writel(0x000004f0, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);
		writel(0x00000401, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		tmp &= ~ETH_MAC_GEN_LED_CFG_SEL_MASK;
		tmp |= ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		break;

	case AL_ETH_MAC_MODE_KR_LL_25G:
		if (adapter->rev_id > AL_ETH_REV_ID_2) {
			/* configure and enable the ASYNC FIFO between the MACs and the EC */
			/* TX min packet size */
			writel(0x00000010,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_1);
			/* TX max packet size */
			writel(0x00002800,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_2);
			/* TX input bus configuration */
			writel(0x00000080,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_3);
			/* TX output bus configuration */
			writel(0x00030020,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_4);
			/* TX Valid/ready configuration */
			writel(0x00000023,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_5);
			/* RX input bus configuration */
			writel(0x00030020,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_3);
			/* RX output bus configuration */
			writel(0x00000080,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_4);
			/* RX Valid/ready configuration */
			writel(0x00000012,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_5);
			/* V3 additional MAC selection */
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_SEL);
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CFG);
			writel(0x00000000,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CTRL);
			writel(0x000000a0,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_10G_LL_CFG);
			/* ASYNC FIFO ENABLE */
			writel(0x00003333,
			       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_AFIFO_CTRL);
		}

		/* MAC register file */
		writel(0x01022810, adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
		/* XAUI MAC control register */
		writel(0x00000005, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);
		/* RXAUI MAC control register */
		writel(0x00000007, adapter->mac_regs_base + AL_ETH_MAC_GEN_RXAUI_CFG);
		writel(0x000001F1, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_CFG);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_32_64);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_64_32);

		writel(0x000004f0, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);
		writel(0x00000401, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		tmp &= ETH_MAC_GEN_LED_CFG_SEL_MASK;
		tmp |= ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);

		break;

	case AL_ETH_MAC_MODE_10G_SGMII:
		/* MAC register file */
		writel(0x01022810, adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);

		/* XAUI MAC control register */
		writel(0x00000001, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);

		writel(0x0000002b, adapter->mac_regs_base + AL_ETH_MAC_10G_IF_MODE);
		writel(0x00009140, adapter->mac_regs_base + AL_ETH_MAC_10G_CONTROL);

		/* RXAUI MAC control register */
		writel(0x00000007, adapter->mac_regs_base + AL_ETH_MAC_GEN_RXAUI_CFG);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_32_64);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_64_32);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);
		tmp &= ETH_MAC_GEN_MUX_SEL_KR_IN_MASK;
		tmp |= 0x00063910;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);

		writel(0x40003210, adapter->mac_regs_base + AL_ETH_MAC_GEN_CLK_CFG);
		writel(0x00000401, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		tmp &= ~ETH_MAC_GEN_LED_CFG_SEL_MASK;
		tmp |= ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		break;

	case AL_ETH_MAC_MODE_XLG_LL_40G:
		/* configure and enable the ASYNC FIFO between the MACs and the EC */
		/* TX min packet size */
		writel(0x00000010,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_1);
		/* TX max packet size */
		writel(0x00002800,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_2);
		/* TX input bus configuration */
		writel(0x00000080,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_3);
		/* TX output bus configuration */
		writel(0x00010040,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_4);
		/* TX Valid/ready configuration */
		writel(0x00000023,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_5);
		/* RX input bus configuration */
		writel(0x00010040,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_3);
		/* RX output bus configuration */
		writel(0x00000080,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_4);
		/* RX Valid/ready configuration */
		writel(0x00000112,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_5);
		/* V3 additional MAC selection */
		writel(0x00000010, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_SEL);
		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CFG);
		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CTRL);
		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_10G_LL_CFG);
		/* ASYNC FIFO ENABLE */
		writel(0x00003333, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_AFIFO_CTRL);

		/* cmd_cfg */
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_ADDR);
		writel(0x01022810,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_DATA);

		/* XAUI MAC control register */
		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);
		tmp &= ETH_MAC_GEN_MUX_SEL_KR_IN_MASK;
		tmp |= 0x06883910;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);
		writel(0x0000040f, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);

		/* XAUI MAC control register */
		writel(0x00000005, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);
		/* RXAUI MAC control register */
		writel(0x00000007, adapter->mac_regs_base + AL_ETH_MAC_GEN_RXAUI_CFG);
		writel(0x000001F1, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_CFG);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_32_64);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_64_32);
		writel(0x10003210, adapter->mac_regs_base + AL_ETH_MAC_GEN_CLK_CFG);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		tmp &= ~ETH_MAC_GEN_LED_CFG_SEL_MASK;
		tmp |= ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		break;

	case AL_ETH_MAC_MODE_XLG_LL_25G:
		/* xgmii_mode: 0=xlgmii, 1=xgmii */
		writel(0x0080,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_ADDR);
		writel(0x00000001,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_DATA);

		/* configure and enable the ASYNC FIFO between the MACs and the EC */
		/* TX min packet size */
		writel(0x00000010,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_1);
		/* TX max packet size */
		writel(0x00002800,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_2);
		/* TX input bus configuration */
		writel(0x00000080,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_3);
		/* TX output bus configuration */
		writel(0x00010040,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_4);
		/* TX Valid/ready configuration */
		writel(0x00000023,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_5);
		/* RX input bus configuration */
		writel(0x00010040,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_3);
		/* RX output bus configuration */
		writel(0x00000080,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_4);
		/* RX Valid/ready configuration */
		writel(0x00000112,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_5);
		/* V3 additional MAC selection */
		writel(0x00000010, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_SEL);
		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CFG);
		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CTRL);
		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_10G_LL_CFG);
		/* ASYNC FIFO ENABLE */
		writel(0x00003333, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_AFIFO_CTRL);

		/* cmd_cfg */
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_ADDR);
		writel(0x01022810,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_DATA);
		/* use VL 0-2 for RXLAUI lane 0, use VL 1-3 for RXLAUI lane 1 */
		al_eth_40g_pcs_reg_write(adapter, 0x00010008, 0x0d80);
		/* configure the PCS to work 32 bit interface */
		writel(0x00440000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_40G_LL_CFG);

		/* disable MLD and move to clause 49 PCS: */
		writel(0xE, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_40G_LL_ADDR);
		writel(0, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_40G_LL_DATA);

		/* XAUI MAC control register */
		writel(0x0000040f, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);

		/* XAUI MAC control register */
		writel(0x00000005, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);
		/* RXAUI MAC control register */
		writel(0x00000007, adapter->mac_regs_base + AL_ETH_MAC_GEN_RXAUI_CFG);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_32_64);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_64_32);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		tmp &= ~ETH_MAC_GEN_LED_CFG_SEL_MASK;
		tmp |= ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);

		break;

	case AL_ETH_MAC_MODE_XLG_LL_50G:

		/* configure and enable the ASYNC FIFO between the MACs and the EC */
		/* TX min packet size */
		writel(0x00000010,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_1);
		/* TX max packet size */
		writel(0x00002800,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_2);
		/* TX input bus configuration */
		writel(0x00000080,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_3);
		/* TX output bus configuration */
		writel(0x00010040,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_4);
		/* TX Valid/ready configuration */
		writel(0x00000023,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_TX_AFIFO_CFG_5);
		/* RX input bus configuration */
		writel(0x00010040,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_3);
		/* RX output bus configuration */
		writel(0x00000080,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_4);
		/* RX Valid/ready configuration */
		writel(0x00000112,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_RX_AFIFO_CFG_5);
		/* V3 additional MAC selection */
		writel(0x00000010, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_SEL);
		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CFG);
		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_10G_LL_CTRL);
		writel(0x00000000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_10G_LL_CFG);
		/* ASYNC FIFO ENABLE */
		writel(0x00003333, adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_AFIFO_CTRL);

		/* cmd_cfg */
		writel(0x00000008,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_ADDR);
		writel(0x01022810,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_MAC_40G_LL_DATA);

		/* configure which two of the 4 PCS Lanes (VL) are combined to one RXLAUI lane */
		/* use VL 0-2 for RXLAUI lane 0, use VL 1-3 for RXLAUI lane 1 */
		al_eth_40g_pcs_reg_write(adapter, 0x00010008, 0x0d81);
		/* configure the PCS to work 32 bit interface */
		writel(0x00440000,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_V3_PCS_40G_LL_CFG);

		/* XAUI MAC control register */
		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);
		tmp &= ETH_MAC_GEN_MUX_SEL_KR_IN_MASK;
		tmp |= 0x06883910;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);

		writel(0x0000040f, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);

		/* XAUI MAC control register */
		writel(0x00000005, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);
		/* RXAUI MAC control register */
		writel(0x00000007, adapter->mac_regs_base + AL_ETH_MAC_GEN_RXAUI_CFG);
		writel(0x000001F1, adapter->mac_regs_base + AL_ETH_MAC_GEN_SD_CFG);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_32_64);
		writel(0x00000401,
		       adapter->mac_regs_base + AL_ETH_MAC_GEN_XGMII_DFIFO_64_32);
		writel(0x10003210, adapter->mac_regs_base + AL_ETH_MAC_GEN_CLK_CFG);

		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		tmp &= ~ETH_MAC_GEN_LED_CFG_SEL_MASK;
		tmp |= ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_GEN_LED_CFG);
		break;

	default:
		netdev_err(adapter->netdev, "Eth: unsupported MAC mode %d",
			   mode);
		return -EPERM;
	}
	adapter->mac_mode = mode;
	netdev_info(adapter->netdev, "configured MAC to %s mode:\n",
		    al_eth_mac_mode_str(mode));

	return 0;
}

/* start the mac */
int al_eth_mac_start(struct al_hw_eth_adapter *adapter)
{
	u32 tmp;

	if (AL_ETH_IS_1G_MAC(adapter->mac_mode)) {
		/* 1G MAC control register */
		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_1G_CMD_CFG);
		tmp |= ETH_1G_MAC_CMD_CFG_TX_ENA | ETH_1G_MAC_CMD_CFG_RX_ENA;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_1G_CMD_CFG);
	} else if (AL_ETH_IS_10G_MAC(adapter->mac_mode) || AL_ETH_IS_25G_MAC(adapter->mac_mode)) {
		/* 10G MAC control register  */
		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
		tmp |= ETH_10G_MAC_CMD_CFG_TX_ENA | ETH_10G_MAC_CMD_CFG_RX_ENA;
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
	} else {
		u32 cmd_cfg;

		cmd_cfg = al_eth_40g_mac_reg_read(adapter,
						  ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_ADDR);

		cmd_cfg |= (ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_TX_ENA |
			    ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_RX_ENA);

		al_eth_40g_mac_reg_write(adapter,
					 ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_ADDR,
					 cmd_cfg);
	}

	return 0;
}

/* stop the mac */
int al_eth_mac_stop(struct al_hw_eth_adapter *adapter)
{
	u32 tmp;

	if (AL_ETH_IS_1G_MAC(adapter->mac_mode)) {
		/* 1G MAC control register */
		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_1G_CMD_CFG);
		tmp &= ~(ETH_1G_MAC_CMD_CFG_TX_ENA | ETH_1G_MAC_CMD_CFG_RX_ENA);
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_1G_CMD_CFG);
	} else if (AL_ETH_IS_10G_MAC(adapter->mac_mode) ||
		 AL_ETH_IS_25G_MAC(adapter->mac_mode)) {
		/* 10G MAC control register  */
		tmp = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
		tmp &= ~(ETH_10G_MAC_CMD_CFG_TX_ENA | ETH_10G_MAC_CMD_CFG_RX_ENA);
		writel(tmp, adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
	} else {
		u32 cmd_cfg;

		cmd_cfg = al_eth_40g_mac_reg_read(adapter,
						  ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_ADDR);

		cmd_cfg &= ~(ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_TX_ENA |
			    ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_RX_ENA);

		al_eth_40g_mac_reg_write(adapter,
					 ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_ADDR,
					 cmd_cfg);
	}

	return 0;
}

static void al_eth_mac_link_config_1g_mac(struct al_hw_eth_adapter *adapter,
					  bool force_1000_base_x,
					  bool an_enable, u32 speed,
					  bool full_duplex)
{
	u32 mac_ctrl;
	u32 sgmii_ctrl = 0;
	u32 sgmii_if_mode = 0;
	u32 rgmii_ctrl = 0;

	mac_ctrl = readl(adapter->mac_regs_base + AL_ETH_MAC_1G_CMD_CFG);

	if (adapter->mac_mode == AL_ETH_MAC_MODE_SGMII) {
		writel(ETH_MAC_SGMII_REG_ADDR_CTRL_REG,
		       adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_ADDR);
		sgmii_ctrl = readl(adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_DATA);
		/*
		 * in case bit 0 is off in sgmii_if_mode register all the other
		 * bits are ignored.
		 */
		if (!force_1000_base_x)
			sgmii_if_mode = ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_EN;

		if (an_enable) {
			sgmii_if_mode |= ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_AN;
			sgmii_ctrl |= ETH_MAC_SGMII_REG_DATA_CTRL_AN_ENABLE;
		} else {
			sgmii_ctrl &= ~(ETH_MAC_SGMII_REG_DATA_CTRL_AN_ENABLE);
		}
	}

	if (adapter->mac_mode == AL_ETH_MAC_MODE_RGMII) {
		/*
		 * Use the speed provided by the MAC instead of the PHY
		 */
		rgmii_ctrl = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_RGMII_CFG);

		rgmii_ctrl &= ~ETH_MAC_GEN_RGMII_CFG_ENA_AUTO;
		rgmii_ctrl &= ~ETH_MAC_GEN_RGMII_CFG_SET_1000_SEL;
		rgmii_ctrl &= ~ETH_MAC_GEN_RGMII_CFG_SET_10_SEL;

		writel(rgmii_ctrl, adapter->mac_regs_base + AL_ETH_MAC_GEN_RGMII_CFG);
	}

	if (full_duplex) {
		mac_ctrl &= ~ETH_1G_MAC_CMD_CFG_HD_EN;
	} else {
		mac_ctrl |= ETH_1G_MAC_CMD_CFG_HD_EN;
		sgmii_if_mode |= ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_DUPLEX;
	}

	if (speed == 1000) {
		mac_ctrl |= ETH_1G_MAC_CMD_CFG_1G_SPD;
		sgmii_if_mode |= ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_1000;
	} else {
		mac_ctrl &= ~ETH_1G_MAC_CMD_CFG_1G_SPD;
		if (speed == 10) {
			mac_ctrl |= ETH_1G_MAC_CMD_CFG_10M_SPD;
		} else {
			sgmii_if_mode |= ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_100;
			mac_ctrl &= ~ETH_1G_MAC_CMD_CFG_10M_SPD;
		}
	}

	if (adapter->mac_mode == AL_ETH_MAC_MODE_SGMII) {
		writel(ETH_MAC_SGMII_REG_ADDR_IF_MODE_REG,
		       adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_ADDR);
		writel(sgmii_if_mode, adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_DATA);

		writel(ETH_MAC_SGMII_REG_ADDR_CTRL_REG,
		       adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_ADDR);
		writel(sgmii_ctrl, adapter->mac_regs_base + AL_ETH_MAC_SGMII_REG_DATA);
	}

	writel(mac_ctrl, adapter->mac_regs_base + AL_ETH_MAC_1G_CMD_CFG);
}

static void al_eth_mac_link_config_10g_mac(struct al_hw_eth_adapter *adapter,
					   bool force_1000_base_x,
					   bool an_enable, u32 speed,
					   bool full_duplex)
{
	u32 if_mode;
	u32 val;

	if_mode = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_IF_MODE);

	if (force_1000_base_x) {
		u32 control;

		if_mode &= ~ETH_10G_MAC_IF_MODE_SGMII_EN_MASK;

		control = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_CONTROL);

		if (an_enable)
			control |= ETH_10G_MAC_CONTROL_AN_EN_MASK;
		else
			control &= ~ETH_10G_MAC_CONTROL_AN_EN_MASK;

		writel(control, adapter->mac_regs_base + AL_ETH_MAC_10G_CONTROL);

	} else {
		if_mode |= ETH_10G_MAC_IF_MODE_SGMII_EN_MASK;
		if (an_enable) {
			if_mode |= ETH_10G_MAC_IF_MODE_SGMII_AN_MASK;
		} else {
			if_mode &= ~ETH_10G_MAC_IF_MODE_SGMII_AN_MASK;

			if (speed == 1000)
				val = ETH_10G_MAC_IF_MODE_SGMII_SPEED_1G;
			else if (speed == 100)
				val = ETH_10G_MAC_IF_MODE_SGMII_SPEED_100M;
			else
				val = ETH_10G_MAC_IF_MODE_SGMII_SPEED_10M;

			if_mode &= ~ETH_10G_MAC_IF_MODE_SGMII_SPEED_MASK;
			if_mode |= (val << ETH_10G_MAC_IF_MODE_SGMII_SPEED_SHIFT) &
				ETH_10G_MAC_IF_MODE_SGMII_SPEED_MASK;

			if_mode &= ~ETH_10G_MAC_IF_MODE_SGMII_DUPLEX_MASK;
			if_mode |= (((full_duplex) ? ETH_10G_MAC_IF_MODE_SGMII_DUPLEX_FULL :
				     ETH_10G_MAC_IF_MODE_SGMII_DUPLEX_HALF) << ETH_10G_MAC_IF_MODE_SGMII_DUPLEX_SHIFT) &
				     ETH_10G_MAC_IF_MODE_SGMII_DUPLEX_MASK;
		}
	}

	writel(if_mode, adapter->mac_regs_base + AL_ETH_MAC_10G_IF_MODE);
}

/* update link speed and duplex mode */
int al_eth_mac_link_config(struct al_hw_eth_adapter *adapter,
			   bool force_1000_base_x, bool an_enable, u32 speed,
			   bool full_duplex)
{
	if ((!AL_ETH_IS_1G_MAC(adapter->mac_mode)) &&
	    (adapter->mac_mode != AL_ETH_MAC_MODE_SGMII_2_5G)) {
		netdev_err(adapter->netdev,
			   "eth [%s]: this function not supported in this mac mode.\n",
			   adapter->name);
		return -EINVAL;
	}

	if ((adapter->mac_mode != AL_ETH_MAC_MODE_RGMII) && (an_enable)) {
		/*
		 * an_enable is not relevant to RGMII mode.
		 * in AN mode speed and duplex aren't relevant.
		 */
		netdev_info(adapter->netdev,
			    "eth [%s]: set auto negotiation to enable\n",
			    adapter->name);
	} else {
		netdev_info(adapter->netdev,
			    "eth [%s]: set link speed to %dMbps. %s duplex.\n",
			    adapter->name, speed,
			    full_duplex ? "full" : "half");

		if ((speed != 10) && (speed != 100) && (speed != 1000)) {
			netdev_err(adapter->netdev,
				   "eth [%s]: bad speed parameter (%d).\n",
				   adapter->name, speed);
			return -EINVAL;
		}
		if ((speed == 1000) && (full_duplex == false)) {
			netdev_err(adapter->netdev,
				   "eth [%s]: half duplex in 1Gbps is not supported.\n",
				   adapter->name);
			return -EINVAL;
		}
	}

	if (AL_ETH_IS_1G_MAC(adapter->mac_mode))
		al_eth_mac_link_config_1g_mac(adapter, force_1000_base_x,
					      an_enable, speed, full_duplex);
	else
		al_eth_mac_link_config_10g_mac(adapter, force_1000_base_x,
					       an_enable, speed, full_duplex);

	return 0;
}

/* MDIO */
int al_eth_mdio_config(struct al_hw_eth_adapter *adapter,
		       enum al_eth_mdio_type mdio_type, bool shared_mdio_if,
		       enum al_eth_ref_clk_freq ref_clk_freq,
		       unsigned int mdio_clk_freq_khz)
{
	enum al_eth_mdio_if mdio_if = AL_ETH_MDIO_IF_10G_MAC;
	const char *if_name = (mdio_if == AL_ETH_MDIO_IF_1G_MAC) ? "10/100/1G MAC" : "10G MAC";
	const char *type_name = (mdio_type == AL_ETH_MDIO_TYPE_CLAUSE_22) ? "Clause 22" : "Clause 45";
	const char *shared_name = shared_mdio_if ? "Yes" : "No";
	unsigned int ref_clk_freq_khz;
	u32 val;

	netdev_dbg(adapter->netdev,
		   "eth [%s]: mdio config: interface %s. type %s. shared: %s\n",
		   adapter->name, if_name, type_name, shared_name);
	adapter->shared_mdio_if = shared_mdio_if;

	val = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);
	netdev_dbg(adapter->netdev, "eth [%s]: mdio config: 10G mac \n",
		   adapter->name);

	switch (mdio_if) {
	case AL_ETH_MDIO_IF_1G_MAC:
		val &= ~BIT(10);
		break;
	case AL_ETH_MDIO_IF_10G_MAC:
		val |= BIT(10);
		break;
	}

	writel(val, adapter->mac_regs_base + AL_ETH_MAC_GEN_CFG);
	adapter->mdio_if = mdio_if;

	if (mdio_if == AL_ETH_MDIO_IF_10G_MAC) {
		val = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_CFG_STATUS);
		switch (mdio_type) {
		case AL_ETH_MDIO_TYPE_CLAUSE_22:
			val &= ~BIT(6);
			break;
		case AL_ETH_MDIO_TYPE_CLAUSE_45:
			val |= BIT(6);
			break;
		}

		/* set clock div to get 'mdio_clk_freq_khz' */
		switch (ref_clk_freq) {
		default:
			netdev_err(adapter->netdev,
				   "%s: invalid reference clock frequency (%d)\n",
				   adapter->name, ref_clk_freq);
		case AL_ETH_REF_FREQ_375_MHZ:
			ref_clk_freq_khz = 375000;
			break;
		case AL_ETH_REF_FREQ_187_5_MHZ:
			ref_clk_freq_khz = 187500;
			break;
		case AL_ETH_REF_FREQ_250_MHZ:
			ref_clk_freq_khz = 250000;
			break;
		case AL_ETH_REF_FREQ_500_MHZ:
			ref_clk_freq_khz = 500000;
			break;
		case AL_ETH_REF_FREQ_428_MHZ:
			ref_clk_freq_khz = 428000;
			break;
		}

		val &= ~(0x1FF << 7);
		val |= (ref_clk_freq_khz / (2 * mdio_clk_freq_khz)) << 7;
		val &= ~ETH_10G_MAC_MDIO_CFG_HOLD_TIME_MASK;
		val |= (ETH_10G_MAC_MDIO_CFG_HOLD_TIME_7_CLK << ETH_10G_MAC_MDIO_CFG_HOLD_TIME_SHIFT) &
			ETH_10G_MAC_MDIO_CFG_HOLD_TIME_MASK;
		writel(val, adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_CFG_STATUS);
	} else {
		if (mdio_type != AL_ETH_MDIO_TYPE_CLAUSE_22) {
			netdev_err(adapter->netdev,
				   "eth [%s] mdio type not supported for this interface\n",
				   adapter->name);
			return -EINVAL;
		}
	}

	adapter->mdio_type = mdio_type;
	return 0;
}

static void al_eth_mdio_1g_mac_read(struct al_hw_eth_adapter *adapter,
				    u32 phy_addr, u32 reg, u16 *val)
{
	*val = readl(adapter->mac_regs_base + AL_ETH_MAC_1G_PHY + reg);
}

static void al_eth_mdio_1g_mac_write(struct al_hw_eth_adapter *adapter,
				     u32 phy_addr, u32 reg, u16 val)
{
	writel(val, adapter->mac_regs_base + AL_ETH_MAC_1G_PHY + reg);
}

static int al_eth_mdio_10g_mac_wait_busy(struct al_hw_eth_adapter *adapter)
{
	int count = 0;
	u32 mdio_cfg_status;

	do {
		mdio_cfg_status = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_CFG_STATUS);
		if (mdio_cfg_status & BIT(0)) {
			if (count > 0)
				netdev_dbg(adapter->netdev,
					   "eth [%s] mdio: still busy!\n",
					   adapter->name);
		} else {
			return 0;
		}
		udelay(AL_ETH_MDIO_DELAY_PERIOD);
	} while (count++ < AL_ETH_MDIO_DELAY_COUNT);

	return -ETIMEDOUT;
}

static int al_eth_mdio_10g_mac_type22(struct al_hw_eth_adapter *adapter,
				      int read, u32 phy_addr, u32 reg, u16 *val)
{
	int rc;
	const char *op = (read == 1) ? "read" : "write";
	u32 mdio_cfg_status;
	u16 mdio_cmd;

	/* wait if the HW is busy */
	rc = al_eth_mdio_10g_mac_wait_busy(adapter);
	if (rc) {
		netdev_err(adapter->netdev,
			   " eth [%s] mdio %s failed. HW is busy\n",
			   adapter->name, op);
		return rc;
	}

	mdio_cmd = (u16)(0x1F & reg);
	mdio_cmd |= (0x1F & phy_addr) << 5;

	if (read)
		mdio_cmd |= BIT(15); /* READ command */

	writew(mdio_cmd, adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_CMD);
	if (!read)
		writew(*val, adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_DATA);

	/* wait for the busy to clear */
	rc = al_eth_mdio_10g_mac_wait_busy(adapter);
	if (rc != 0) {
		netdev_err(adapter->netdev, " %s mdio %s failed on timeout\n",
			   adapter->name, op);
		return -ETIMEDOUT;
	}

	mdio_cfg_status = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_CFG_STATUS);

	if (mdio_cfg_status & BIT(1)) {
		netdev_err(adapter->netdev,
			   " %s mdio %s failed on error. phy_addr 0x%x reg 0x%x\n",
			   adapter->name, op, phy_addr, reg);
		return -EIO;
	}
	if (read)
		*val = readw((u16 *)adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_DATA);
	return 0;
}

static int al_eth_mdio_10g_mac_type45(struct al_hw_eth_adapter *adapter,
				      int read, u32 port_addr, u32 device,
				      u32 reg, u16 *val)
{
	int rc;
	const char *op = (read == 1) ? "read" : "write";
	u32 mdio_cfg_status;
	u16 mdio_cmd;

	/* wait if the HW is busy */
	rc = al_eth_mdio_10g_mac_wait_busy(adapter);
	if (rc) {
		netdev_err(adapter->netdev, " %s mdio %s failed. HW is busy\n",
			   adapter->name, op);
		return rc;
	}

	/* set command register */
	mdio_cmd = (u16)(0x1F & device);
	mdio_cmd |= (0x1F & port_addr) << 5;
	writew(mdio_cmd, adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_CMD);

	/* send address frame */
	writew(reg, adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_REGADDR);

	/* wait for the busy to clear */
	rc = al_eth_mdio_10g_mac_wait_busy(adapter);
	if (rc) {
		netdev_err(adapter->netdev,
			   " %s mdio %s (address frame) failed on timeout\n",
			   adapter->name, op);
		return rc;
	}

	/* if read, write again to the command register with READ bit set */
	if (read) {
		mdio_cmd |= BIT(15); /* READ command */
		writew(mdio_cmd, (u16 *)adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_CMD);
	} else {
		writew(*val, (u16 *)adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_DATA);
	}

	/* wait for the busy to clear */
	rc = al_eth_mdio_10g_mac_wait_busy(adapter);
	if (rc) {
		netdev_err(adapter->netdev, " %s mdio %s failed on timeout\n",
			   adapter->name, op);
		return rc;
	}

	mdio_cfg_status = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_CFG_STATUS);

	if (mdio_cfg_status & BIT(1)) {
		netdev_err(adapter->netdev,
			   " %s mdio %s failed on error. port 0x%x, device 0x%x reg 0x%x\n",
			   adapter->name, op, port_addr, device, reg);
		return -EIO;
	}

	if (read)
		*val = readw((u16 *)adapter->mac_regs_base + AL_ETH_MAC_10G_MDIO_DATA);

	return 0;
}

/*
 * acquire mdio interface ownership
 * when mdio interface shared between multiple eth controllers, this function waits until the ownership granted for this controller.
 * this function does nothing when the mdio interface is used only by this controller.
 *
 * @param adapter
 * @return 0 on success, -ETIMEDOUT  on timeout.
 */
static int al_eth_mdio_lock(struct al_hw_eth_adapter *adapter)
{
	int count = 0;
	u32 mdio_ctrl_1;

	if (!adapter->shared_mdio_if)
		return 0; /* nothing to do when interface is not shared */

	do {
		mdio_ctrl_1 = readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_MDIO_CTRL_1);
		if (mdio_ctrl_1 & BIT(0)) {
			if (count > 0)
				netdev_dbg(adapter->netdev,
					   "eth %s mdio interface still busy!\n",
					   adapter->name);
		} else {
			return 0;
		}
		udelay(AL_ETH_MDIO_DELAY_PERIOD);
	} while (count++ < (AL_ETH_MDIO_DELAY_COUNT * 4));

	netdev_err(adapter->netdev,
		   " %s mdio failed to take ownership. MDIO info reg: 0x%08x\n",
		   adapter->name, readl(adapter->mac_regs_base + AL_ETH_MAC_GEN_MDIO_1));

	return -ETIMEDOUT;
}

/*
 * free mdio interface ownership
 * when mdio interface shared between multiple eth controllers, this function releases the ownership granted for this controller.
 * this function does nothing when the mdio interface is used only by this controller.
 *
 * @param adapter
 * @return 0.
 */
static int al_eth_mdio_free(struct al_hw_eth_adapter *adapter)
{
	if (!adapter->shared_mdio_if)
		return 0; /* nothing to do when interface is not shared */

	writel(0, adapter->mac_regs_base + AL_ETH_MAC_GEN_MDIO_CTRL_1);

	/*
	 * Addressing RMN: 2917
	 *
	 * RMN description:
	 * The HW spin-lock is stateless and doesn't maintain any scheduling
	 * policy.
	 *
	 * Software flow:
	 * After getting the lock wait 2 times the delay period in order to give
	 * the other port chance to take the lock and prevent starvation.
	 * This is not scalable to more than two ports.
	 */
	udelay(2 * AL_ETH_MDIO_DELAY_PERIOD);

	return 0;
}

int al_eth_mdio_read(struct al_hw_eth_adapter *adapter, u32 phy_addr,
		     u32 device, u32 reg, u16 *val)
{
	int rc = al_eth_mdio_lock(adapter);

	if (rc)
		return rc;

	if (adapter->mdio_if == AL_ETH_MDIO_IF_1G_MAC)
		al_eth_mdio_1g_mac_read(adapter, phy_addr, reg, val);
	else
		if (adapter->mdio_type == AL_ETH_MDIO_TYPE_CLAUSE_22)
			rc = al_eth_mdio_10g_mac_type22(adapter, 1, phy_addr,
							reg, val);
		else
			rc = al_eth_mdio_10g_mac_type45(adapter, 1, phy_addr,
							device, reg, val);

	al_eth_mdio_free(adapter);

	netdev_dbg(adapter->netdev,
		   "eth mdio read: phy_addr %x, device %x, reg %x val %x\n",
		   phy_addr, device, reg, *val);
	return rc;
}

int al_eth_mdio_write(struct al_hw_eth_adapter *adapter, u32 phy_addr,
		      u32 device, u32 reg, u16 val)
{
	int rc;

	netdev_dbg(adapter->netdev,
		   "eth mdio write: phy_addr %x, device %x, reg %x, val %x\n",
		   phy_addr, device, reg, val);

	rc = al_eth_mdio_lock(adapter);
	/* interface ownership taken */
	if (rc)
		return rc;

	if (adapter->mdio_if == AL_ETH_MDIO_IF_1G_MAC) {
		al_eth_mdio_1g_mac_write(adapter, phy_addr, reg, val);
	} else {
		if (adapter->mdio_type == AL_ETH_MDIO_TYPE_CLAUSE_22)
			rc = al_eth_mdio_10g_mac_type22(adapter, 0, phy_addr,
							reg, &val);
		else
			rc = al_eth_mdio_10g_mac_type45(adapter, 0, phy_addr,
							device, reg, &val);
	}

	al_eth_mdio_free(adapter);
	return rc;
}

static void al_dump_tx_desc(struct al_hw_eth_adapter *adapter,
			    union al_udma_desc *tx_desc)
{
	u32 *ptr = (u32 *)tx_desc;

	netdev_dbg(adapter->netdev,
		   "eth tx desc:\n0x%08x\n0x%08x\n0x%08x\n0x%08x\n",
		   ptr[0], ptr[1], ptr[2], ptr[3]);
}

static void al_dump_tx_pkt(struct al_hw_eth_adapter *adapter,
			   struct al_udma_q *tx_dma_q, struct al_eth_pkt *pkt)
{
	const char *tso = (pkt->flags & AL_ETH_TX_FLAGS_TSO) ? "TSO" : "";
	const char *l3_csum = (pkt->flags & AL_ETH_TX_FLAGS_IPV4_L3_CSUM) ? "L3 CSUM" : "";
	const char *l4_csum = (pkt->flags & AL_ETH_TX_FLAGS_L4_CSUM) ?
	  ((pkt->flags & AL_ETH_TX_FLAGS_L4_PARTIAL_CSUM) ? "L4 PARTIAL CSUM" : "L4 FULL CSUM") : "";
	const char *fcs = (pkt->flags & AL_ETH_TX_FLAGS_L2_DIS_FCS) ? "Disable FCS" : "";
	const char *ptp = (pkt->flags & AL_ETH_TX_FLAGS_TS) ? "TX_PTP" : "";
	const char *l3_proto_name = "unknown";
	const char *l4_proto_name = "unknown";
	const char *outer_l3_proto_name = "N/A";
	const char *tunnel_mode = ((pkt->tunnel_mode) &
				(AL_ETH_TUNNEL_WITH_UDP == AL_ETH_TUNNEL_WITH_UDP)) ?
				"TUNNEL_WITH_UDP" :
				((pkt->tunnel_mode) &
				(AL_ETH_TUNNEL_NO_UDP == AL_ETH_TUNNEL_NO_UDP)) ?
				"TUNNEL_NO_UDP" : "";
	u32 total_len = 0;
	int i;

	netdev_dbg(adapter->netdev, "[%s %d]: flags: %s %s %s %s %s %s\n",
		   tx_dma_q->udma->name, tx_dma_q->qid, tso, l3_csum, l4_csum,
		   fcs, ptp, tunnel_mode);

	switch (pkt->l3_proto_idx) {
	case AL_ETH_PROTO_ID_IPv4:
		l3_proto_name = "IPv4";
		break;
	case AL_ETH_PROTO_ID_IPv6:
		l3_proto_name = "IPv6";
		break;
	default:
		l3_proto_name = "unknown";
		break;
	}

	switch (pkt->l4_proto_idx) {
	case AL_ETH_PROTO_ID_TCP:
		l4_proto_name = "TCP";
		break;
	case AL_ETH_PROTO_ID_UDP:
		l4_proto_name = "UDP";
		break;
	default:
		l4_proto_name = "unknown";
		break;
	}

	switch (pkt->outer_l3_proto_idx) {
	case AL_ETH_PROTO_ID_IPv4:
		outer_l3_proto_name = "IPv4";
		break;
	case AL_ETH_PROTO_ID_IPv6:
		outer_l3_proto_name = "IPv6";
		break;
	default:
		outer_l3_proto_name = "N/A";
		break;
	}

	netdev_dbg(adapter->netdev,
		   "[%s %d]: L3 proto: %d (%s). L4 proto: %d (%s). "
		   "Outer_L3 proto: %d (%s). vlan source count %d. mod add %d. mod del %d\n",
		   tx_dma_q->udma->name, tx_dma_q->qid, pkt->l3_proto_idx,
		   l3_proto_name, pkt->l4_proto_idx, l4_proto_name,
		   pkt->outer_l3_proto_idx, outer_l3_proto_name,
		   pkt->source_vlan_count, pkt->vlan_mod_add_count,
		   pkt->vlan_mod_del_count);

	if (pkt->meta) {
		const char *store = pkt->meta->store ? "Yes" : "No";
		const char *ptp_val = (pkt->flags & AL_ETH_TX_FLAGS_TS) ? "Yes" : "No";

		netdev_dbg(adapter->netdev,
			   "[%s %d]: tx pkt with meta data. words valid %x\n",
			   tx_dma_q->udma->name, tx_dma_q->qid,
			   pkt->meta->words_valid);
		netdev_dbg(adapter->netdev,
			   "[%s %d]: meta: store to cache %s. l3 hdr len %d. l3 hdr offset %d. "
			   "l4 hdr len %d. mss val %d ts_index %d ts_val:%s\n",
			   tx_dma_q->udma->name, tx_dma_q->qid, store,
			   pkt->meta->l3_header_len,
			   pkt->meta->l3_header_offset,
			   pkt->meta->l4_header_len, pkt->meta->mss_val,
			   pkt->meta->ts_index, ptp_val);
		netdev_dbg(adapter->netdev,
			   "outer_l3_hdr_offset %d. outer_l3_len %d.\n",
			   pkt->meta->outer_l3_offset, pkt->meta->outer_l3_len);
	}

	netdev_dbg(adapter->netdev, "[%s %d]: num of bufs: %d\n",
		   tx_dma_q->udma->name, tx_dma_q->qid, pkt->num_of_bufs);
	for (i = 0; i < pkt->num_of_bufs; i++) {
		netdev_dbg(adapter->netdev,
			   "eth [%s %d]: buf[%d]: len 0x%08x. address 0x%016llx\n",
			   tx_dma_q->udma->name, tx_dma_q->qid,
			   i, pkt->bufs[i].len,
			   (unsigned long long)pkt->bufs[i].addr);
		total_len += pkt->bufs[i].len;
	}

	netdev_dbg(adapter->netdev, "[%s %d]: total len: 0x%08x\n",
		   tx_dma_q->udma->name, tx_dma_q->qid, total_len);

}

/* add packet to transmission queue */
int al_eth_tx_pkt_prepare(struct al_hw_eth_adapter *adapter,
			  struct al_udma_q *tx_dma_q, struct al_eth_pkt *pkt)
{
	union al_udma_desc *tx_desc;
	u32 tx_descs;
	u32 flags = AL_M2S_DESC_FIRST | AL_M2S_DESC_CONCAT |
		    (pkt->flags & AL_ETH_TX_FLAGS_INT);
	u64 tgtid = ((u64)pkt->tgtid) << AL_UDMA_DESC_TGTID_SHIFT;
	u32 meta_ctrl;
	u32 ring_id;
	int buf_idx;

	netdev_dbg(adapter->netdev, "[%s %d]: new tx pkt\n",
		   tx_dma_q->udma->name, tx_dma_q->qid);

	al_dump_tx_pkt(adapter, tx_dma_q, pkt);

	tx_descs = pkt->num_of_bufs;
	if (pkt->meta)
		tx_descs += 1;

	if (unlikely(al_udma_available_get(tx_dma_q) < tx_descs)) {
		netdev_dbg(adapter->netdev,
			   "[%s %d]: failed to allocate (%d) descriptors",
			   tx_dma_q->udma->name, tx_dma_q->qid, tx_descs);
		return 0;
	}

	if (pkt->meta) {
		u32 meta_word_0 = 0;
		u32 meta_word_1 = 0;
		u32 meta_word_2 = 0;
		u32 meta_word_3 = 0;

		meta_word_0 |= flags | AL_M2S_DESC_META_DATA;
		meta_word_0 &=  ~AL_M2S_DESC_CONCAT;
		flags &= ~(AL_M2S_DESC_FIRST | AL_ETH_TX_FLAGS_INT);

		tx_desc = al_udma_desc_get(tx_dma_q);
		/* get ring id, and clear FIRST and Int flags */
		ring_id = al_udma_ring_id_get(tx_dma_q) <<
			AL_M2S_DESC_RING_ID_SHIFT;

		meta_word_0 |= ring_id;
		meta_word_0 |= pkt->meta->words_valid << 12;

		if (pkt->meta->store)
			meta_word_0 |= AL_ETH_TX_META_STORE;

		if (pkt->meta->words_valid & 1) {
			meta_word_0 |= pkt->meta->vlan1_cfi_sel;
			meta_word_0 |= pkt->meta->vlan2_vid_sel << 2;
			meta_word_0 |= pkt->meta->vlan2_cfi_sel << 4;
			meta_word_0 |= pkt->meta->vlan2_pbits_sel << 6;
			meta_word_0 |= pkt->meta->vlan2_ether_sel << 8;
		}

		if (pkt->meta->words_valid & 2) {
			meta_word_1 = pkt->meta->vlan1_new_vid;
			meta_word_1 |= pkt->meta->vlan1_new_cfi << 12;
			meta_word_1 |= pkt->meta->vlan1_new_pbits << 13;
			meta_word_1 |= pkt->meta->vlan2_new_vid << 16;
			meta_word_1 |= pkt->meta->vlan2_new_cfi << 28;
			meta_word_1 |= pkt->meta->vlan2_new_pbits << 29;
		}

		if (pkt->meta->words_valid & 4) {
			u32 l3_offset;

			meta_word_2 = pkt->meta->l3_header_len & AL_ETH_TX_META_L3_LEN_MASK;
			meta_word_2 |= (pkt->meta->l3_header_offset & AL_ETH_TX_META_L3_OFF_MASK) <<
				AL_ETH_TX_META_L3_OFF_SHIFT;
			meta_word_2 |= (pkt->meta->l4_header_len & 0x3f) << 16;

			if (unlikely(pkt->flags & AL_ETH_TX_FLAGS_TS))
				meta_word_0 |= pkt->meta->ts_index <<
					AL_ETH_TX_META_MSS_MSB_TS_VAL_SHIFT;
			else
				meta_word_0 |= (((pkt->meta->mss_val & 0x3c00) >> 10)
						<< AL_ETH_TX_META_MSS_MSB_TS_VAL_SHIFT);
			meta_word_2 |= ((pkt->meta->mss_val & 0x03ff)
					<< AL_ETH_TX_META_MSS_LSB_VAL_SHIFT);

			/*
			 * move from bytes to multiplication of 2 as the HW
			 * expect to get it
			 */
			l3_offset = (pkt->meta->outer_l3_offset >> 1);

			meta_word_0 |=
				(((l3_offset &
				   AL_ETH_TX_META_OUTER_L3_OFF_HIGH_MASK) >> 3)
				   << AL_ETH_TX_META_OUTER_L3_OFF_HIGH_SHIFT);

			meta_word_3 |=
				((l3_offset &
				   AL_ETH_TX_META_OUTER_L3_OFF_LOW_MASK)
				   << AL_ETH_TX_META_OUTER_L3_OFF_LOW_SHIFT);

			/*
			 * shift right 2 bits to work in multiplication of 4
			 * as the HW expect to get it
			 */
			meta_word_3 |=
				(((pkt->meta->outer_l3_len >> 2) &
				   AL_ETH_TX_META_OUTER_L3_LEN_MASK)
				   << AL_ETH_TX_META_OUTER_L3_LEN_SHIFT);
		}

		tx_desc->tx_meta.len_ctrl = cpu_to_le32(meta_word_0);
		tx_desc->tx_meta.meta_ctrl = cpu_to_le32(meta_word_1);
		tx_desc->tx_meta.meta1 = cpu_to_le32(meta_word_2);
		tx_desc->tx_meta.meta2 = cpu_to_le32(meta_word_3);
		al_dump_tx_desc(adapter, tx_desc);
	}

	meta_ctrl = pkt->flags & AL_ETH_TX_PKT_META_FLAGS;

	meta_ctrl |= pkt->l3_proto_idx;
	meta_ctrl |= pkt->l4_proto_idx << AL_ETH_TX_L4_PROTO_IDX_SHIFT;
	meta_ctrl |= pkt->source_vlan_count << AL_ETH_TX_SRC_VLAN_CNT_SHIFT;
	meta_ctrl |= pkt->vlan_mod_add_count << AL_ETH_TX_VLAN_MOD_ADD_SHIFT;
	meta_ctrl |= pkt->vlan_mod_del_count << AL_ETH_TX_VLAN_MOD_DEL_SHIFT;
	meta_ctrl |= pkt->vlan_mod_v1_ether_sel << AL_ETH_TX_VLAN_MOD_E_SEL_SHIFT;
	meta_ctrl |= pkt->vlan_mod_v1_vid_sel << AL_ETH_TX_VLAN_MOD_VID_SEL_SHIFT;
	meta_ctrl |= pkt->vlan_mod_v1_pbits_sel << AL_ETH_TX_VLAN_MOD_PBIT_SEL_SHIFT;

	meta_ctrl |= pkt->tunnel_mode << AL_ETH_TX_TUNNEL_MODE_SHIFT;
	if (pkt->outer_l3_proto_idx == AL_ETH_PROTO_ID_IPv4)
		meta_ctrl |= BIT(AL_ETH_TX_OUTER_L3_PROTO_SHIFT);

	flags |= pkt->flags & AL_ETH_TX_PKT_UDMA_FLAGS;
	for (buf_idx = 0; buf_idx < pkt->num_of_bufs; buf_idx++) {
		u32 flags_len = flags;

		tx_desc = al_udma_desc_get(tx_dma_q);
		/* get ring id, and clear FIRST and Int flags */
		ring_id = al_udma_ring_id_get(tx_dma_q) <<
			AL_M2S_DESC_RING_ID_SHIFT;

		flags_len |= ring_id;

		if (buf_idx == (pkt->num_of_bufs - 1))
			flags_len |= AL_M2S_DESC_LAST;

		/* clear First and Int flags */
		flags &= AL_ETH_TX_FLAGS_NO_SNOOP;
		flags |= AL_M2S_DESC_CONCAT;

		flags_len |= pkt->bufs[buf_idx].len & AL_M2S_DESC_LEN_MASK;
		tx_desc->tx.len_ctrl = cpu_to_le32(flags_len);
		if (buf_idx == 0)
			tx_desc->tx.meta_ctrl = cpu_to_le32(meta_ctrl);
		tx_desc->tx.buf_ptr = cpu_to_le64(
			pkt->bufs[buf_idx].addr | tgtid);
		al_dump_tx_desc(adapter, tx_desc);
	}

	netdev_dbg(adapter->netdev,
		   "[%s %d]: pkt descriptors written into the tx queue. descs num (%d)\n",
		   tx_dma_q->udma->name, tx_dma_q->qid, tx_descs);

	return tx_descs;
}

void al_eth_tx_dma_action(struct al_udma_q *tx_dma_q, u32 tx_descs)
{
	/* add tx descriptors */
	al_udma_desc_action_add(tx_dma_q, tx_descs);
}

/* get number of completed tx descriptors, upper layer should derive from */
int al_eth_comp_tx_get(struct al_hw_eth_adapter *adapter,
		       struct al_udma_q *tx_dma_q)
{
	int rc;

	rc = al_udma_cdesc_get_all(tx_dma_q, NULL);
	if (rc != 0) {
		al_udma_cdesc_ack(tx_dma_q, rc);
		netdev_dbg(adapter->netdev,
			   "[%s %d]: tx completion: descs (%d)\n",
			   tx_dma_q->udma->name, tx_dma_q->qid, rc);
	}

	return rc;
}

/* add buffer to receive queue */
int al_eth_rx_buffer_add(struct al_hw_eth_adapter *adapter,
			 struct al_udma_q *rx_dma_q,
			 struct al_buf *buf, u32 flags,
			 struct al_buf *header_buf)
{
	u64 tgtid = ((u64)flags & AL_ETH_RX_FLAGS_TGTID_MASK) <<
		AL_UDMA_DESC_TGTID_SHIFT;
	u32 flags_len = flags & ~AL_ETH_RX_FLAGS_TGTID_MASK;
	union al_udma_desc *rx_desc;

	netdev_dbg(adapter->netdev, "[%s %d]: add rx buffer.\n",
		   rx_dma_q->udma->name, rx_dma_q->qid);

	if (unlikely(al_udma_available_get(rx_dma_q) < 1)) {
		netdev_dbg(adapter->netdev,
			   "[%s]: rx q (%d) has no enough free descriptor",
			   rx_dma_q->udma->name, rx_dma_q->qid);
		return -ENOSPC;
	}

	rx_desc = al_udma_desc_get(rx_dma_q);

	flags_len |= al_udma_ring_id_get(rx_dma_q) << AL_S2M_DESC_RING_ID_SHIFT;
	flags_len |= buf->len & AL_S2M_DESC_LEN_MASK;

	if (flags & AL_S2M_DESC_DUAL_BUF) {
		WARN_ON(!header_buf); /*header valid in dual buf */
		WARN_ON((rx_dma_q->udma->rev_id < AL_UDMA_REV_ID_2) &&
		       (AL_ADDR_HIGH(buf->addr) != AL_ADDR_HIGH(header_buf->addr)));

		flags_len |= ((header_buf->len >> AL_S2M_DESC_LEN2_GRANULARITY_SHIFT)
			<< AL_S2M_DESC_LEN2_SHIFT) & AL_S2M_DESC_LEN2_MASK;
		rx_desc->rx.buf2_ptr_lo = cpu_to_le32(AL_ADDR_LOW(header_buf->addr));
	}
	rx_desc->rx.len_ctrl = cpu_to_le32(flags_len);
	rx_desc->rx.buf1_ptr = cpu_to_le64(buf->addr | tgtid);

	return 0;
}

/* notify the hw engine about rx descriptors that were added to the receive queue */
void al_eth_rx_buffer_action(struct al_hw_eth_adapter *adapter,
			     struct al_udma_q *rx_dma_q, u32 descs_num)
{
	netdev_dbg(adapter->netdev,
		   "[%s]: update the rx engine tail pointer: queue %d. descs %d\n",
		   rx_dma_q->udma->name, rx_dma_q->qid, descs_num);

	/* add rx descriptor */
	al_udma_desc_action_add(rx_dma_q, descs_num);
}

/* get packet from RX completion ring */
u32 al_eth_pkt_rx(struct al_hw_eth_adapter *adapter, struct al_udma_q *rx_dma_q,
		  struct al_eth_pkt *pkt)
{
	volatile union al_udma_cdesc *cdesc;
	volatile struct al_eth_rx_cdesc *rx_desc;
	u32 i, rc = al_udma_cdesc_packet_get(rx_dma_q, &cdesc);

	if (rc == 0)
		return 0;

	WARN_ON(rc > AL_ETH_PKT_MAX_BUFS);

	netdev_dbg(adapter->netdev, "[%s]: fetch rx packet: queue %d.\n",
		   rx_dma_q->udma->name, rx_dma_q->qid);

	pkt->rx_header_len = 0;
	for (i = 0; i < rc; i++) {
		u32 buf1_len, buf2_len;

		/* get next descriptor */
		rx_desc = (volatile struct al_eth_rx_cdesc *)al_cdesc_next(rx_dma_q,
									   cdesc,
									   i);

		buf1_len = le32_to_cpu(rx_desc->len);

		if ((i == 0) && (le32_to_cpu(rx_desc->word2) &
			AL_UDMA_CDESC_BUF2_USED)) {
			buf2_len = le32_to_cpu(rx_desc->word2);
			pkt->rx_header_len = (buf2_len & AL_S2M_DESC_LEN2_MASK) >>
			AL_S2M_DESC_LEN2_SHIFT;
		}
		pkt->bufs[i].len = buf1_len & AL_S2M_DESC_LEN_MASK;
	}
	/* get flags from last desc */
	pkt->flags = le32_to_cpu(rx_desc->ctrl_meta);

	/* update L3/L4 proto index */
	pkt->l3_proto_idx = pkt->flags & AL_ETH_RX_L3_PROTO_IDX_MASK;
	pkt->l4_proto_idx = (pkt->flags >> AL_ETH_RX_L4_PROTO_IDX_SHIFT) &
				AL_ETH_RX_L4_PROTO_IDX_MASK;
	pkt->rxhash = (le32_to_cpu(rx_desc->len) & AL_ETH_RX_HASH_MASK) >>
			AL_ETH_RX_HASH_SHIFT;
	pkt->l3_offset = (le32_to_cpu(rx_desc->word2) & AL_ETH_RX_L3_OFFSET_MASK) >>
		AL_ETH_RX_L3_OFFSET_SHIFT;

	al_udma_cdesc_ack(rx_dma_q, rc);
	return rc;
}

#define AL_ETH_THASH_UDMA_SHIFT		0
#define AL_ETH_THASH_UDMA_MASK		(0xF << AL_ETH_THASH_UDMA_SHIFT)

#define AL_ETH_THASH_Q_SHIFT		4
#define AL_ETH_THASH_Q_MASK		(0x3 << AL_ETH_THASH_Q_SHIFT)

int al_eth_thash_table_set(struct al_hw_eth_adapter *adapter, u32 idx, u8 udma,
			   u32 queue)
{
	u32 entry;

	WARN_ON(idx >= AL_ETH_RX_THASH_TABLE_SIZE); /* valid THASH index */

	entry = (udma << AL_ETH_THASH_UDMA_SHIFT) & AL_ETH_THASH_UDMA_MASK;
	entry |= (queue << AL_ETH_THASH_Q_SHIFT) & AL_ETH_THASH_Q_MASK;

	writel(idx, adapter->ec_regs_base + AL_EC_RFW_THASH_TABLE_ADDR);
	writel(entry, adapter->ec_regs_base + AL_EC_RFW_THASH_TABLE_DATA);
	return 0;
}

int al_eth_fsm_table_set(struct al_hw_eth_adapter *adapter, u32 idx, u32 entry)
{
	WARN_ON(idx >= AL_ETH_RX_FSM_TABLE_SIZE); /* valid FSM index */

	writel(idx, adapter->ec_regs_base + AL_EC_RFW_FSM_TABLE_ADDR);
	writel(entry, adapter->ec_regs_base + AL_EC_RFW_FSM_TABLE_DATA);
	return 0;
}

static u32 al_eth_fwd_ctrl_entry_to_val(struct al_eth_fwd_ctrl_table_entry *entry)
{
	u32 val = 0;

	val &= ~GENMASK(3, 0);
	val |= (entry->prio_sel << 0) & GENMASK(3, 0);
	val &= ~GENMASK(7, 4);
	val |= (entry->queue_sel_1 << 4) & GENMASK(7, 4);
	val &= ~GENMASK(9, 8);
	val |= (entry->queue_sel_2 << 8) & GENMASK(9, 8);
	val &= ~GENMASK(13, 10);
	val |= (entry->udma_sel << 10) & GENMASK(13, 10);
	val &= ~GENMASK(17, 15);
	val |= (!!entry->filter << 19);

	return val;
}

void al_eth_ctrl_table_def_set(struct al_hw_eth_adapter *adapter,
			       bool use_table,
			       struct al_eth_fwd_ctrl_table_entry *entry)
{
	u32 val = al_eth_fwd_ctrl_entry_to_val(entry);

	if (use_table)
		val |= EC_RFW_CTRL_TABLE_DEF_SEL;

	writel(val, adapter->ec_regs_base + AL_EC_RFW_CTRL_TABLE_DEF);
}

void al_eth_hash_key_set(struct al_hw_eth_adapter *adapter, u32 idx, u32 val)
{
	writel(val, adapter->ec_regs_base + AL_EC_RFW_HASH_KEY(idx));
}

static u32 al_eth_fwd_mac_table_entry_to_val(struct al_eth_fwd_mac_table_entry *entry)
{
	u32 val = 0;

	val |= entry->filter ? EC_FWD_MAC_CTRL_RX_VAL_DROP : 0;
	val |= ((entry->udma_mask << EC_FWD_MAC_CTRL_RX_VAL_UDMA_SHIFT) &
					EC_FWD_MAC_CTRL_RX_VAL_UDMA_MASK);

	val |= ((entry->qid << EC_FWD_MAC_CTRL_RX_VAL_QID_SHIFT) &
					EC_FWD_MAC_CTRL_RX_VAL_QID_MASK);

	val |= entry->rx_valid ? EC_FWD_MAC_CTRL_RX_VALID : 0;

	val |= ((entry->tx_target << EC_FWD_MAC_CTRL_TX_VAL_SHIFT) &
					EC_FWD_MAC_CTRL_TX_VAL_MASK);

	val |= entry->tx_valid ? EC_FWD_MAC_CTRL_TX_VALID : 0;

	return val;
}

void al_eth_fwd_mac_table_set(struct al_hw_eth_adapter *adapter, u32 idx,
			      struct al_eth_fwd_mac_table_entry *entry)
{
	u32 val;

	WARN_ON(idx >= AL_ETH_FWD_MAC_NUM);

	val = (entry->addr[2] << 24) | (entry->addr[3] << 16) |
	      (entry->addr[4] << 8) | entry->addr[5];
	writel(val, adapter->ec_regs_base + AL_EC_FWD_MAC(idx) +
	       AL_EC_FWD_MAC_DATA_L);
	val = (entry->addr[0] << 8) | entry->addr[1];
	writel(val, adapter->ec_regs_base + AL_EC_FWD_MAC(idx) +
	       AL_EC_FWD_MAC_DATA_H);
	val = (entry->mask[2] << 24) | (entry->mask[3] << 16) |
	      (entry->mask[4] << 8) | entry->mask[5];
	writel(val, adapter->ec_regs_base + AL_EC_FWD_MAC(idx) +
	       AL_EC_FWD_MAC_MASK_L);
	val = (entry->mask[0] << 8) | entry->mask[1];
	writel(val, adapter->ec_regs_base + AL_EC_FWD_MAC(idx) +
	       AL_EC_FWD_MAC_MASK_H);

	val = al_eth_fwd_mac_table_entry_to_val(entry);
	writel(val, adapter->ec_regs_base + AL_EC_FWD_MAC(idx) +
	       AL_EC_FWD_MAC_CTRL);
}

void al_eth_mac_addr_store(void * __iomem ec_base, u32 idx, u8 *addr)
{
	void __iomem *ec_regs_base = ec_base;
	u32 val;

	val = (addr[2] << 24) | (addr[3] << 16) | (addr[4] << 8) | addr[5];
	writel(val, ec_regs_base + AL_EC_FWD_MAC(idx) + AL_EC_FWD_MAC_DATA_L);
	val = (addr[0] << 8) | addr[1];
	writel(val, ec_regs_base + AL_EC_FWD_MAC(idx) + AL_EC_FWD_MAC_DATA_H);
}

void al_eth_mac_addr_read(void * __iomem ec_base, u32 idx, u8 *addr)
{
	void __iomem *ec_regs_base = ec_base;
	u32 addr_lo = readl(ec_regs_base + AL_EC_FWD_MAC(idx) +
			    AL_EC_FWD_MAC_DATA_L);
	u16 addr_hi = readl(ec_regs_base + AL_EC_FWD_MAC(idx) +
			    AL_EC_FWD_MAC_DATA_H);

	addr[5] = addr_lo & 0xff;
	addr[4] = (addr_lo >> 8) & 0xff;
	addr[3] = (addr_lo >> 16) & 0xff;
	addr[2] = (addr_lo >> 24) & 0xff;
	addr[1] = addr_hi & 0xff;
	addr[0] = (addr_hi >> 8) & 0xff;
}

void al_eth_fwd_pbits_table_set(struct al_hw_eth_adapter *adapter, u32 idx, u8 prio)
{
	WARN_ON(idx >= AL_ETH_FWD_PBITS_TABLE_NUM); /* valid PBIT index */
	WARN_ON(prio >= AL_ETH_FWD_PRIO_TABLE_NUM); /* valid PRIO index */

	writel(idx, adapter->ec_regs_base + AL_EC_RFW_PBITS_TABLE_ADDR);
	writel(prio, adapter->ec_regs_base + AL_EC_RFW_PBITS_TABLE_DATA);
}

void al_eth_fwd_priority_table_set(struct al_hw_eth_adapter *adapter, u8 prio, u8 qid)
{
	WARN_ON(prio >= AL_ETH_FWD_PRIO_TABLE_NUM); /* valid PRIO index */

	writel(qid, adapter->ec_regs_base + AL_EC_RFW_PRIORITY_QUEUE(prio));
}

#define AL_ETH_RFW_FILTER_SUPPORTED(rev_id)	\
	(AL_ETH_RFW_FILTER_UNDET_MAC | \
	AL_ETH_RFW_FILTER_DET_MAC | \
	AL_ETH_RFW_FILTER_TAGGED | \
	AL_ETH_RFW_FILTER_UNTAGGED | \
	AL_ETH_RFW_FILTER_BC | \
	AL_ETH_RFW_FILTER_MC | \
	AL_ETH_RFW_FILTER_VLAN_VID | \
	AL_ETH_RFW_FILTER_CTRL_TABLE | \
	AL_ETH_RFW_FILTER_PROT_INDEX | \
	AL_ETH_RFW_FILTER_PARSE)

/* Configure the receive filters */
int al_eth_filter_config(struct al_hw_eth_adapter *adapter,
			 struct al_eth_filter_params *params)
{
	u32 reg;

	if (params->filters & ~(AL_ETH_RFW_FILTER_SUPPORTED(adapter->rev_id))) {
		netdev_err(adapter->netdev,
			   "[%s]: unsupported filter options (0x%08x)\n",
			   adapter->name, params->filters);
		return -EINVAL;
	}

	reg = readl(adapter->ec_regs_base + AL_EC_RFW_OUT_CFG);

	if (params->enable)
		reg |= EC_RFW_OUT_CFG_DROP_EN;
	else
		reg &= ~EC_RFW_OUT_CFG_DROP_EN;

	writel(reg, adapter->ec_regs_base + AL_EC_RFW_OUT_CFG);

	reg = readl(adapter->ec_regs_base + AL_EC_RFW_FILTER);
	reg &= ~AL_ETH_RFW_FILTER_SUPPORTED(adapter->rev_id);
	reg |= params->filters;
	writel(reg, adapter->ec_regs_base + AL_EC_RFW_FILTER);

	if (params->filters & AL_ETH_RFW_FILTER_PROT_INDEX) {
		int i;

		for (i = 0; i < AL_ETH_PROTOCOLS_NUM; i++) {
			reg = readl(adapter->ec_regs_base +
				    AL_EC_EPE_A_PROT_ACT(i));
			if (params->filter_proto[i])
				reg |= EC_EPE_A_PROT_ACT_DROP;
			else
				reg &= ~EC_EPE_A_PROT_ACT_DROP;
			writel(reg, adapter->ec_regs_base +
			       AL_EC_EPE_A_PROT_ACT(i));
		}
	}

	return 0;
}

int al_eth_flow_control_config(struct al_hw_eth_adapter *adapter,
			       struct al_eth_flow_control_params *params)
{
	u32 reg;
	int i;

	WARN_ON(!params); /* valid params pointer */

	switch (params->type) {
	case AL_ETH_FLOW_CONTROL_TYPE_LINK_PAUSE:
		netdev_dbg(adapter->netdev,
			   "[%s]: config flow control to link pause mode.\n",
			   adapter->name);

		/* config the mac */
		if (AL_ETH_IS_1G_MAC(adapter->mac_mode)) {
			/* set quanta value */
			writel(params->quanta,
			       adapter->mac_regs_base + AL_ETH_MAC_1G_PAUSE_QUANT);
			writel(params->quanta_th,
			       adapter->ec_regs_base + AL_EC_EFC_XOFF_TIMER_1G);

		} else if (AL_ETH_IS_10G_MAC(adapter->mac_mode) ||
			   AL_ETH_IS_25G_MAC(adapter->mac_mode)) {
			/* set quanta value */
			writel(params->quanta,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL01_PAUSE_QNTA);
			/* set quanta threshold value */
			writel(params->quanta_th,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL01_QNTA_THRES);
		} else {
			/* set quanta value */
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL01_PAUSE_QUANTA_ADDR,
						 params->quanta);
			/* set quanta threshold value */
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL01_QUANTA_THRESH_ADDR,
						 params->quanta_th);
		}

		if (params->obay_enable)
			/* Tx path FIFO, unmask pause_on from MAC when PAUSE packet received */
			writel(1, adapter->ec_regs_base + AL_EC_EFC_EC_PAUSE);
		else
			writel(0, adapter->ec_regs_base + AL_EC_EFC_EC_PAUSE);

		/* Rx path */
		if (params->gen_enable)
			/* enable generating xoff from ec fifo almost full indication in hysteresis mode */
			writel(BIT(EC_EFC_EC_XOFF_MASK_2_SHIFT),
			       adapter->ec_regs_base + AL_EC_EFC_EC_XOFF);
		else
			writel(0, adapter->ec_regs_base + AL_EC_EFC_EC_XOFF);

		if (AL_ETH_IS_1G_MAC(adapter->mac_mode))
			/* in 1G mode, enable generating xon from ec fifo in hysteresis mode*/
			writel(EC_EFC_XON_MASK_2 | EC_EFC_XON_MASK_1,
			       adapter->ec_regs_base + AL_EC_EFC_XON);

		/* set hysteresis mode thresholds */
		writel(params->rx_fifo_th_low | (params->rx_fifo_th_high << EC_EFC_RX_FIFO_HYST_TH_HIGH_SHIFT),
		       adapter->ec_regs_base + AL_EC_EFC_RX_FIFO_HYST);

		for (i = 0; i < 4; i++) {
			if (params->obay_enable)
				/* Tx path UDMA, unmask pause_on for all queues */
				writel(params->prio_q_map[i][0],
				       adapter->ec_regs_base + AL_EC_FC_UDMA(i) + AL_EC_FC_UDMA_Q_PAUSE_0);
			else
				writel(0, adapter->ec_regs_base +
				       AL_EC_FC_UDMA(i) +
				       AL_EC_FC_UDMA_Q_PAUSE_0);

			if (params->gen_enable)
				/* Rx path UDMA, enable generating xoff from UDMA queue almost full indication */
				writel(params->prio_q_map[i][0],
				       adapter->ec_regs_base + AL_EC_FC_UDMA(i) + AL_EC_FC_UDMA_Q_XOFF_0);
			else
				writel(0, adapter->ec_regs_base +
				       AL_EC_FC_UDMA(i) +
				       AL_EC_FC_UDMA_Q_XOFF_0);
		}
	break;
	case AL_ETH_FLOW_CONTROL_TYPE_PFC:
		netdev_dbg(adapter->netdev,
			   "[%s]: config flow control to PFC mode.\n",
			   adapter->name);
		WARN_ON(!!AL_ETH_IS_1G_MAC(adapter->mac_mode)); /* pfc not available for RGMII mode */

		for (i = 0; i < 4; i++) {
			int prio;
			int offset = 0;

			for (prio = 0; prio < 8; prio++) {
				offset += sizeof(u32);
				if (params->obay_enable)
					/* Tx path UDMA, unmask pause_on for all queues */
					writel(params->prio_q_map[i][prio],
					       adapter->ec_regs_base + AL_EC_FC_UDMA(i) + AL_EC_FC_UDMA_Q_PAUSE_0 + offset);
				else
					writel(0, adapter->ec_regs_base +
					       AL_EC_FC_UDMA(i) +
					       AL_EC_FC_UDMA_Q_PAUSE_0 + offset);

				if (params->gen_enable)
					writel(params->prio_q_map[i][prio],
					       adapter->ec_regs_base + AL_EC_FC_UDMA(i) + AL_EC_FC_UDMA_Q_XOFF_0 + offset);
				else
					writel(0, adapter->ec_regs_base +
					       AL_EC_FC_UDMA(i) +
					       AL_EC_FC_UDMA_Q_XOFF_0 + offset);
			}
		}

		/* Rx path */
		/* enable generating xoff from ec fifo almost full indication in hysteresis mode */
		if (params->gen_enable)
			writel(0xFF << EC_EFC_EC_XOFF_MASK_2_SHIFT,
			       adapter->ec_regs_base + AL_EC_EFC_EC_XOFF);
		else
			writel(0, adapter->ec_regs_base + AL_EC_EFC_EC_XOFF);

		/* set hysteresis mode thresholds */
		writel(params->rx_fifo_th_low | (params->rx_fifo_th_high << EC_EFC_RX_FIFO_HYST_TH_HIGH_SHIFT),
		       adapter->ec_regs_base + AL_EC_EFC_RX_FIFO_HYST);

		if (AL_ETH_IS_10G_MAC(adapter->mac_mode) || AL_ETH_IS_25G_MAC(adapter->mac_mode)) {
			/* config the 10g_mac */
			/* set quanta value (same value for all prios) */
			reg = params->quanta | (params->quanta << 16);
			writel(reg,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL01_PAUSE_QNTA);
			writel(reg,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL23_PAUSE_QNTA);
			writel(reg,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL45_PAUSE_QNTA);
			writel(reg,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL67_PAUSE_QNTA);
			/* set quanta threshold value (same value for all prios) */
			reg = params->quanta_th | (params->quanta_th << 16);
			writel(reg,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL01_QNTA_THRES);
			writel(reg,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL23_QNTA_THRES);
			writel(reg,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL45_QNTA_THRES);
			writel(reg,
			       adapter->mac_regs_base + AL_ETH_MAC_10G_CL67_QNTA_THRES);

			/* enable PFC in the 10g_MAC */
			reg = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
			reg |= BIT(19);
			writel(reg, adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
		} else {
			/* config the 40g_mac */
			/* set quanta value (same value for all prios) */
			reg = params->quanta | (params->quanta << 16);
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL01_PAUSE_QUANTA_ADDR, reg);
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL23_PAUSE_QUANTA_ADDR, reg);
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL45_PAUSE_QUANTA_ADDR, reg);
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL67_PAUSE_QUANTA_ADDR, reg);
			/* set quanta threshold value (same value for all prios) */
			reg = params->quanta_th | (params->quanta_th << 16);
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL01_QUANTA_THRESH_ADDR, reg);
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL23_QUANTA_THRESH_ADDR, reg);
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL45_QUANTA_THRESH_ADDR, reg);
			al_eth_40g_mac_reg_write(adapter,
						 ETH_MAC_GEN_V3_MAC_40G_CL67_QUANTA_THRESH_ADDR, reg);

			/* enable PFC in the 40g_MAC */
			reg = readl(adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
			reg |= BIT(19);
			writel(reg, adapter->mac_regs_base + AL_ETH_MAC_10G_CMD_CFG);
			reg = al_eth_40g_mac_reg_read(adapter, ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_ADDR);

			reg |= ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_PFC_MODE;

			al_eth_40g_mac_reg_write(adapter, ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_ADDR, reg);
		}
		break;
	default:
		netdev_err(adapter->netdev,
			   "[%s]: unsupported flow control type %d\n",
			   adapter->name, params->type);
		return -EINVAL;
	}
	return 0;
}

/* Traffic control */

int al_eth_flr_rmn(int (*pci_read_config_u32)(void *handle, int where, u32 *val),
		   int (*pci_write_config_u32)(void *handle, int where, u32 val),
		   void *handle, void __iomem *mac_base)
{
	void __iomem *mac_regs_base = mac_base;
	u32 cfg_reg_store[6];
	u32 reg;
	u32 mux_sel;
	int i = 0;

	(*pci_read_config_u32)(handle, AL_ADAPTER_GENERIC_CONTROL_0, &reg);

	/* reset 1G mac */
	reg |= AL_ADAPTER_GENERIC_CONTROL_0_ETH_RESET_1GMAC;
	(*pci_write_config_u32)(handle, AL_ADAPTER_GENERIC_CONTROL_0, reg);
	udelay(1000);
	/* don't reset 1G mac */
	reg &= ~AL_ADAPTER_GENERIC_CONTROL_0_ETH_RESET_1GMAC;
	/* prevent 1G mac reset on FLR */
	reg &= ~AL_ADAPTER_GENERIC_CONTROL_0_ETH_RESET_1GMAC_ON_FLR;
	/* prevent adapter reset */
	(*pci_write_config_u32)(handle, AL_ADAPTER_GENERIC_CONTROL_0, reg);

	mux_sel = readl(mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);

	/* save pci register that get reset due to flr*/
	(*pci_read_config_u32)(handle, AL_PCI_COMMAND, &cfg_reg_store[i++]);
	(*pci_read_config_u32)(handle, 0xC, &cfg_reg_store[i++]);
	(*pci_read_config_u32)(handle, 0x10, &cfg_reg_store[i++]);
	(*pci_read_config_u32)(handle, 0x18, &cfg_reg_store[i++]);
	(*pci_read_config_u32)(handle, 0x20, &cfg_reg_store[i++]);
	(*pci_read_config_u32)(handle, 0x110, &cfg_reg_store[i++]);

	/* do flr */
	(*pci_write_config_u32)(handle, AL_PCI_EXP_CAP_BASE + AL_PCI_EXP_DEVCTL, AL_PCI_EXP_DEVCTL_BCR_FLR);
	udelay(1000);
	/* restore command */
	i = 0;
	(*pci_write_config_u32)(handle, AL_PCI_COMMAND, cfg_reg_store[i++]);
	(*pci_write_config_u32)(handle, 0xC, cfg_reg_store[i++]);
	(*pci_write_config_u32)(handle, 0x10, cfg_reg_store[i++]);
	(*pci_write_config_u32)(handle, 0x18, cfg_reg_store[i++]);
	(*pci_write_config_u32)(handle, 0x20, cfg_reg_store[i++]);
	(*pci_write_config_u32)(handle, 0x110, cfg_reg_store[i++]);

	writel((readl(mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL) &
		~ETH_MAC_GEN_MUX_SEL_KR_IN_MASK) | mux_sel,
	        mac_regs_base + AL_ETH_MAC_GEN_MUX_SEL);

	/* set SGMII clock to 125MHz */
	writel(0x03320501, mac_regs_base + AL_ETH_MAC_SGMII_CLK_DIV);

	/* reset 1G mac */
	reg |= AL_ADAPTER_GENERIC_CONTROL_0_ETH_RESET_1GMAC;
	(*pci_write_config_u32)(handle, AL_ADAPTER_GENERIC_CONTROL_0, reg);

	udelay(1000);

	/* clear 1G mac reset */
	reg &= ~AL_ADAPTER_GENERIC_CONTROL_0_ETH_RESET_1GMAC;
	(*pci_write_config_u32)(handle, AL_ADAPTER_GENERIC_CONTROL_0, reg);

	/* reset SGMII mac clock to default */
	writel(0x00320501, mac_regs_base + AL_ETH_MAC_SGMII_CLK_DIV);
	udelay(1000);
	/* reset async fifo */
	reg = readl(mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);
	reg |= 0xF0;
	writel(reg, mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);
	reg = readl(mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);
	reg &= ~0xF0;
	writel(reg, mac_regs_base + AL_ETH_MAC_GEN_SD_FIFO_CTRL);

	return 0;
}

/* board params register 1 */
#define AL_HW_ETH_MEDIA_TYPE_MASK	(GENMASK(3, 0))
#define AL_HW_ETH_MEDIA_TYPE_SHIFT	0
#define AL_HW_ETH_EXT_PHY_SHIFT	4
#define AL_HW_ETH_PHY_ADDR_MASK	(GENMASK(9, 5))
#define AL_HW_ETH_PHY_ADDR_SHIFT	5
#define AL_HW_ETH_SFP_EXIST_SHIFT	10
#define AL_HW_ETH_AN_ENABLE_SHIFT	11
#define AL_HW_ETH_KR_LT_ENABLE_SHIFT	12
#define AL_HW_ETH_KR_FEC_ENABLE_SHIFT	13
#define AL_HW_ETH_MDIO_FREQ_MASK	(GENMASK(15, 14))
#define AL_HW_ETH_MDIO_FREQ_SHIFT	14
#define AL_HW_ETH_I2C_ADAPTER_ID_MASK	(GENMASK(19, 16))
#define AL_HW_ETH_I2C_ADAPTER_ID_SHIFT	16
#define AL_HW_ETH_EXT_PHY_IF_MASK	(GENMASK(21, 20))
#define AL_HW_ETH_EXT_PHY_IF_SHIFT	20
#define AL_HW_ETH_AUTO_NEG_MODE_SHIFT	22
#define AL_HW_ETH_REF_CLK_FREQ_MASK	(GENMASK(31, 29))
#define AL_HW_ETH_REF_CLK_FREQ_SHIFT	29

/* board params register 2 */
#define AL_HW_ETH_1000_BASE_X_SHIFT		1
#define AL_HW_ETH_1G_AN_DISABLE_SHIFT		2
#define AL_HW_ETH_1G_SPEED_MASK		(GENMASK(4, 3))
#define AL_HW_ETH_1G_SPEED_SHIFT		3
#define AL_HW_ETH_1G_HALF_DUPLEX_SHIFT		5
#define AL_HW_ETH_1G_FC_DISABLE_SHIFT		6
#define AL_HW_ETH_RETIMER_EXIST_SHIFT		7
#define AL_HW_ETH_RETIMER_BUS_ID_MASK		(GENMASK(11, 8))
#define AL_HW_ETH_RETIMER_BUS_ID_SHIFT		8
#define AL_HW_ETH_RETIMER_I2C_ADDR_MASK	(GENMASK(18, 12))
#define AL_HW_ETH_RETIMER_I2C_ADDR_SHIFT	12
#define AL_HW_ETH_RETIMER_CHANNEL_SHIFT	19
#define AL_HW_ETH_DAC_LENGTH_MASK		(GENMASK(23, 20))
#define AL_HW_ETH_DAC_LENGTH_SHIFT		20
#define AL_HW_ETH_DAC_SHIFT			24
#define AL_HW_ETH_RETIMER_TYPE_MASK		(GENMASK(26, 25))
#define AL_HW_ETH_RETIMER_TYPE_SHIFT		25
#define AL_HW_ETH_RETIMER_CHANNEL_2_MASK	(GENMASK(28, 27))
#define AL_HW_ETH_RETIMER_CHANNEL_2_SHIFT	27
#define AL_HW_ETH_RETIMER_TX_CHANNEL_MASK	(GENMASK(31, 29))
#define AL_HW_ETH_RETIMER_TX_CHANNEL_SHIFT	29

/* board params register 3 */
#define AL_HW_ETH_GPIO_SFP_PRESENT_MASK	(GENMASK(5, 0))
#define AL_HW_ETH_GPIO_SFP_PRESENT_SHIFT	0

int al_eth_board_params_set(void __iomem *mac_base,
			    struct al_eth_board_params *params)
{
	void __iomem *mac_regs_base = mac_base;
	u32 reg = 0;

	/* ************* Setting Board params register 1 **************** */
	reg &= ~AL_HW_ETH_MEDIA_TYPE_MASK;
	reg |= (params->media_type << AL_HW_ETH_MEDIA_TYPE_SHIFT) & AL_HW_ETH_MEDIA_TYPE_MASK;
	reg |= !!params->phy_exist << AL_HW_ETH_EXT_PHY_SHIFT;
	reg &= ~AL_HW_ETH_PHY_ADDR_MASK;
	reg |= (params->phy_mdio_addr << AL_HW_ETH_PHY_ADDR_SHIFT) & AL_HW_ETH_PHY_ADDR_MASK;

	reg |= !!params->sfp_plus_module_exist << AL_HW_ETH_SFP_EXIST_SHIFT;

	reg |= !!params->autoneg_enable << AL_HW_ETH_AN_ENABLE_SHIFT;
	reg |= !!params->kr_lt_enable << AL_HW_ETH_KR_LT_ENABLE_SHIFT;
	reg |= !!params->kr_fec_enable << AL_HW_ETH_KR_FEC_ENABLE_SHIFT;
	reg &= ~AL_HW_ETH_MDIO_FREQ_MASK;
	reg |= (params->mdio_freq << AL_HW_ETH_MDIO_FREQ_SHIFT) & AL_HW_ETH_MDIO_FREQ_MASK;
	reg &= ~AL_HW_ETH_I2C_ADAPTER_ID_MASK;
	reg |= (params->i2c_adapter_id << AL_HW_ETH_I2C_ADAPTER_ID_SHIFT) & AL_HW_ETH_I2C_ADAPTER_ID_MASK;
	reg &= ~AL_HW_ETH_EXT_PHY_IF_MASK;
	reg |= (params->phy_if << AL_HW_ETH_EXT_PHY_IF_SHIFT) & AL_HW_ETH_EXT_PHY_IF_MASK;

	reg |= (params->an_mode == AL_ETH_BOARD_AUTONEG_IN_BAND << AL_HW_ETH_AUTO_NEG_MODE_SHIFT);

	reg &= ~AL_HW_ETH_REF_CLK_FREQ_MASK;
	reg |= (params->ref_clk_freq << AL_HW_ETH_REF_CLK_FREQ_SHIFT) & AL_HW_ETH_REF_CLK_FREQ_MASK;

	WARN_ON(!reg);

	writel(reg, mac_regs_base + AL_ETH_MAC_1G_SCRATCH);

	/* ************* Setting Board params register 2 **************** */
	reg = 0;
	reg |= !!params->force_1000_base_x << AL_HW_ETH_1000_BASE_X_SHIFT;

	reg |= !!params->an_disable << AL_HW_ETH_1G_AN_DISABLE_SHIFT;

	reg &= ~AL_HW_ETH_1G_SPEED_MASK;
	reg |= (params->speed << AL_HW_ETH_1G_SPEED_SHIFT) & AL_HW_ETH_1G_SPEED_MASK;

	reg |= !!params->half_duplex << AL_HW_ETH_1G_HALF_DUPLEX_SHIFT;

	reg |= !!params->fc_disable << AL_HW_ETH_1G_FC_DISABLE_SHIFT;

	reg |= !!params->retimer_exist << AL_HW_ETH_RETIMER_EXIST_SHIFT;
	reg &= ~AL_HW_ETH_RETIMER_BUS_ID_MASK;
	reg |= (params->retimer_bus_id << AL_HW_ETH_RETIMER_BUS_ID_SHIFT) & AL_HW_ETH_RETIMER_BUS_ID_MASK;
	reg &= ~AL_HW_ETH_RETIMER_I2C_ADDR_MASK;
	reg |= (params->retimer_i2c_addr << AL_HW_ETH_RETIMER_I2C_ADDR_SHIFT) & AL_HW_ETH_RETIMER_I2C_ADDR_MASK;

	reg |= ((params->retimer_channel & BIT(0)) << AL_HW_ETH_RETIMER_CHANNEL_SHIFT);

	reg &= ~AL_HW_ETH_RETIMER_CHANNEL_2_MASK;
	reg |= ((params->retimer_channel & 0x6) >> 1 << AL_HW_ETH_RETIMER_CHANNEL_2_SHIFT) & AL_HW_ETH_RETIMER_CHANNEL_2_MASK;

	reg &= ~AL_HW_ETH_DAC_LENGTH_MASK;
	reg |= (params->dac_len << AL_HW_ETH_DAC_LENGTH_SHIFT) & AL_HW_ETH_DAC_LENGTH_MASK;
	reg |= (params->dac << AL_HW_ETH_DAC_SHIFT);

	reg &= ~AL_HW_ETH_RETIMER_TYPE_MASK;
	reg |= (params->retimer_type << AL_HW_ETH_RETIMER_TYPE_SHIFT) & AL_HW_ETH_RETIMER_TYPE_MASK;

	reg &= ~AL_HW_ETH_RETIMER_TX_CHANNEL_MASK;
	reg |= (params->retimer_tx_channel << AL_HW_ETH_RETIMER_TX_CHANNEL_SHIFT) & AL_HW_ETH_RETIMER_TX_CHANNEL_MASK;

	writel(reg, mac_regs_base + AL_ETH_MAC_10G_SCRATCH);

	/* ************* Setting Board params register 3 **************** */
	reg = 0;

	reg &= ~AL_HW_ETH_GPIO_SFP_PRESENT_MASK;
	reg |= (params->gpio_sfp_present << AL_HW_ETH_GPIO_SFP_PRESENT_SHIFT) & AL_HW_ETH_GPIO_SFP_PRESENT_MASK;

	writel(reg, mac_regs_base + AL_ETH_MAC_1G_MAC_0);

	return 0;
}

int al_eth_board_params_get(void __iomem *mac_base, struct al_eth_board_params *params)
{
	void __iomem *mac_regs_base = mac_base;
	u32 reg = readl(mac_regs_base + AL_ETH_MAC_1G_SCRATCH);

	/* check if the register was initialized, 0 is not a valid value */
	if (!reg)
		return -ENOENT;

	/* ************* Getting Board params register 1 **************** */
	params->media_type = (reg & AL_HW_ETH_MEDIA_TYPE_MASK)
			>> AL_HW_ETH_MEDIA_TYPE_SHIFT;
	if (((reg >> AL_HW_ETH_EXT_PHY_SHIFT) & 0x1))
		params->phy_exist = true;
	else
		params->phy_exist = false;

	params->phy_mdio_addr = (reg & AL_HW_ETH_PHY_ADDR_MASK) >>
			AL_HW_ETH_PHY_ADDR_SHIFT;

	if (((reg >> AL_HW_ETH_SFP_EXIST_SHIFT) & 0x1))
		params->sfp_plus_module_exist = true;
	else
		params->sfp_plus_module_exist = false;

	if (((reg >> AL_HW_ETH_AN_ENABLE_SHIFT) & 0x1))
		params->autoneg_enable = true;
	else
		params->autoneg_enable = false;

	if (((reg >> AL_HW_ETH_KR_LT_ENABLE_SHIFT) & 0x1))
		params->kr_lt_enable = true;
	else
		params->kr_lt_enable = false;

	if (((reg >> AL_HW_ETH_KR_FEC_ENABLE_SHIFT) & 0x1))
		params->kr_fec_enable = true;
	else
		params->kr_fec_enable = false;

	params->mdio_freq = (reg & AL_HW_ETH_MDIO_FREQ_MASK) >>
			AL_HW_ETH_MDIO_FREQ_SHIFT;

	params->i2c_adapter_id = (reg & AL_HW_ETH_I2C_ADAPTER_ID_MASK) >>
			AL_HW_ETH_I2C_ADAPTER_ID_SHIFT;

	params->phy_if = (reg & AL_HW_ETH_EXT_PHY_IF_MASK) >>
			AL_HW_ETH_EXT_PHY_IF_SHIFT;

	if (((reg >> AL_HW_ETH_AUTO_NEG_MODE_SHIFT) & 0x1))
		params->an_mode = true;
	else
		params->an_mode = false;

	params->ref_clk_freq = (reg & AL_HW_ETH_REF_CLK_FREQ_MASK) >>
			AL_HW_ETH_REF_CLK_FREQ_SHIFT;

	/* ************* Getting Board params register 2 **************** */
	reg = readl(mac_regs_base + AL_ETH_MAC_10G_SCRATCH);

	if (((reg >> AL_HW_ETH_1000_BASE_X_SHIFT) & 0x1))
		params->force_1000_base_x = true;
	else
		params->force_1000_base_x = false;

	if (((reg >> AL_HW_ETH_1G_AN_DISABLE_SHIFT) & 0x1))
		params->an_disable = true;
	else
		params->an_disable = false;

	params->speed = (reg & AL_HW_ETH_1G_SPEED_MASK) >>
			AL_HW_ETH_1G_SPEED_SHIFT;

	if (((reg >> AL_HW_ETH_1G_HALF_DUPLEX_SHIFT) & 0x1))
		params->half_duplex = true;
	else
		params->half_duplex = false;

	if (((reg >> AL_HW_ETH_1G_FC_DISABLE_SHIFT) & 0x1))
		params->fc_disable = true;
	else
		params->fc_disable = false;

	if (((reg >> AL_HW_ETH_RETIMER_EXIST_SHIFT) & 0x1))
		params->retimer_exist = true;
	else
		params->retimer_exist = false;

	params->retimer_bus_id = (reg & AL_HW_ETH_RETIMER_BUS_ID_MASK) >>
			AL_HW_ETH_RETIMER_BUS_ID_SHIFT;
	params->retimer_i2c_addr = (reg & AL_HW_ETH_RETIMER_I2C_ADDR_MASK) >>
			AL_HW_ETH_RETIMER_I2C_ADDR_SHIFT;

	params->retimer_channel =
		((((reg >> AL_HW_ETH_RETIMER_CHANNEL_SHIFT) & 0x1)) |
		 ((reg & AL_HW_ETH_RETIMER_CHANNEL_2_MASK) >>
		  AL_HW_ETH_RETIMER_CHANNEL_2_SHIFT) << 1);

	params->dac_len = (reg & AL_HW_ETH_DAC_LENGTH_MASK) >>
			AL_HW_ETH_DAC_LENGTH_SHIFT;

	if (((reg >> AL_HW_ETH_DAC_SHIFT) & 0x1))
		params->dac = true;
	else
		params->dac = false;

	params->retimer_type = (reg & AL_HW_ETH_RETIMER_TYPE_MASK) >>
			AL_HW_ETH_RETIMER_TYPE_SHIFT;

	params->retimer_tx_channel = (reg & AL_HW_ETH_RETIMER_TX_CHANNEL_MASK) >>
			AL_HW_ETH_RETIMER_TX_CHANNEL_SHIFT;

	/* ************* Getting Board params register 3 **************** */
	reg = readl(mac_regs_base + AL_ETH_MAC_1G_MAC_0);

	params->gpio_sfp_present = (reg & AL_HW_ETH_GPIO_SFP_PRESENT_MASK) >>
			AL_HW_ETH_GPIO_SFP_PRESENT_SHIFT;

	return 0;
}
