/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __AL_HW_ETH_H__
#define __AL_HW_ETH_H__

#include <linux/types.h>
#include <linux/soc/alpine/al_hw_udma.h>

#ifndef AL_ETH_PKT_MAX_BUFS
#ifndef AL_ETH_EX
#define AL_ETH_PKT_MAX_BUFS 19
#else
#define AL_ETH_PKT_MAX_BUFS 30
#endif
#endif

#define AL_ETH_UDMA_TX_QUEUES		4
#define AL_ETH_UDMA_RX_QUEUES		4

#define AL_ETH_UDMA_TX_CDESC_SZ		8
#define AL_ETH_UDMA_RX_CDESC_SZ		16

/* PCI Adapter Device/Revision ID */
#define AL_ETH_REV_ID_1			1 /* Alpine V1 */
#define AL_ETH_REV_ID_2			2 /* Alpine V2 basic */
#define AL_ETH_REV_ID_3			3 /* Alpine V2 advanced */

/* PCI BARs */
#define AL_ETH_UDMA_BAR			0
#define AL_ETH_EC_BAR			4
#define AL_ETH_MAC_BAR			2

#define AL_ETH_MAX_FRAME_LEN		10000
#define AL_ETH_MIN_FRAME_LEN		60

#define AL_ETH_TSO_MSS_MIN_VAL		1
#define AL_ETH_TSO_MSS_MAX_VAL		(AL_ETH_MAX_FRAME_LEN - 200)

enum AL_ETH_PROTO_ID {
	AL_ETH_PROTO_ID_UNKNOWN = 0,
	AL_ETH_PROTO_ID_IPv4	= 8,
	AL_ETH_PROTO_ID_IPv6	= 11,
	AL_ETH_PROTO_ID_TCP	= 12,
	AL_ETH_PROTO_ID_UDP	= 13,
	AL_ETH_PROTO_ID_FCOE    = 21,
	AL_ETH_PROTO_ID_GRH     = 22, /* RoCE l3 header */
	AL_ETH_PROTO_ID_BTH     = 23, /* RoCE l4 header */
	AL_ETH_PROTO_ID_ANY	= 32, /* for sw usage only */
};

#define AL_ETH_PROTOCOLS_NUM	(AL_ETH_PROTO_ID_ANY)

enum AL_ETH_TX_TUNNEL_MODE {
	AL_ETH_NO_TUNNELING	= 0,
	AL_ETH_TUNNEL_NO_UDP	= 1, /* NVGRE / IP over IP */
	AL_ETH_TUNNEL_WITH_UDP	= 3,	/* VXLAN */
};

#define AL_ETH_RX_THASH_TABLE_SIZE	BIT(8)
#define AL_ETH_RX_FSM_TABLE_SIZE	BIT(7)
#define AL_ETH_RX_HASH_KEY_NUM		10
#define AL_ETH_FWD_MAC_NUM		32
#define AL_ETH_FWD_PBITS_TABLE_NUM	BIT(3)
#define AL_ETH_FWD_PRIO_TABLE_NUM	BIT(3)

/* MAC media mode */
enum al_eth_mac_mode {
	AL_ETH_MAC_MODE_RGMII,
	AL_ETH_MAC_MODE_SGMII,
	AL_ETH_MAC_MODE_SGMII_2_5G,
	AL_ETH_MAC_MODE_10GbE_Serial,	/* Applies to XFI and KR modes */
	AL_ETH_MAC_MODE_10G_SGMII,	/* SGMII using the 10G MAC, don't use*/
	AL_ETH_MAC_MODE_XLG_LL_40G,	/* applies to 40G mode using the 40G low latency (LL) MAC */
	AL_ETH_MAC_MODE_KR_LL_25G,	/* applies to 25G mode using the 10/25G low latency (LL) MAC */
	AL_ETH_MAC_MODE_XLG_LL_50G,	/* applies to 50G mode using the 40/50G low latency (LL) MAC */
	AL_ETH_MAC_MODE_XLG_LL_25G	/* applies to 25G mode using the 40/50G low latency (LL) MAC */
};

/* interface type used for MDIO */
enum al_eth_mdio_if {
	AL_ETH_MDIO_IF_1G_MAC = 0,
	AL_ETH_MDIO_IF_10G_MAC = 1
};

/* MDIO protocol type */
enum al_eth_mdio_type {
	AL_ETH_MDIO_TYPE_CLAUSE_22 = 0,
	AL_ETH_MDIO_TYPE_CLAUSE_45 = 1
};

/* flow control mode */
enum al_eth_flow_control_type {
	AL_ETH_FLOW_CONTROL_TYPE_LINK_PAUSE,
	AL_ETH_FLOW_CONTROL_TYPE_PFC
};

/* Tx to Rx switching decision type */
enum al_eth_tx_switch_dec_type {
	AL_ETH_TX_SWITCH_TYPE_MAC = 0,
	AL_ETH_TX_SWITCH_TYPE_VLAN_TABLE = 1,
	AL_ETH_TX_SWITCH_TYPE_VLAN_TABLE_AND_MAC = 2,
	AL_ETH_TX_SWITCH_TYPE_BITMAP = 3
};

/* Tx to Rx VLAN ID selection type */
enum al_eth_tx_switch_vid_sel_type {
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_VLAN1 = 0,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_VLAN2 = 1,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_NEW_VLAN1 = 2,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_NEW_VLAN2 = 3,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_DEFAULT_VLAN1 = 4,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_FINAL_VLAN1 = 5
};

/*
 * Rx descriptor configurations
 * Note: when selecting rx descriptor field to inner packet, then that field
 * will be set according to inner packet when packet is tunneled, for non-tunneled
 * packets, the field will be set according to the packets header
 */

/* selection of the LRO_context_value result in the Metadata */
enum al_eth_rx_desc_lro_context_val_res {
	AL_ETH_LRO_CONTEXT_VALUE = 0,	/* LRO_context_value */
	AL_ETH_L4_OFFSET = 1,		/* L4_offset */
};

/* selection of the L4 offset in the Metadata */
enum al_eth_rx_desc_l4_offset_sel {
	AL_ETH_L4_OFFSET_OUTER = 0, /* set L4 offset of the outer packet */
	AL_ETH_L4_OFFSET_INNER = 1, /* set L4 offset of the inner packet */
};

/* selection of the L4 checksum result in the Metadata */
enum al_eth_rx_desc_l4_chk_res_sel {
	AL_ETH_L4_INNER_CHK = 0, /* L4 checksum */
	/* Logic AND between outer and inner L4 checksum result */
	AL_ETH_L4_INNER_OUTER_CHK = 1,
};

/* selection of the L3 checksum result in the Metadata */
enum al_eth_rx_desc_l3_chk_res_sel {
	AL_ETH_L3_CHK_TYPE_0 = 0, /* L3 checksum */
	/* L3 checksum or RoCE/FCoE CRC, based on outer header */
	AL_ETH_L3_CHK_TYPE_1 = 1,
	/*
	 * If tunnel exist = 0, L3 checksum or RoCE/FCoE CRC, based on outer
	 * header. Else, logic AND between outer L3 checksum (Ipv4) and inner
	 * CRC (RoCE or FcoE)
	 */
	AL_ETH_L3_CHK_TYPE_2 = 2,
	/*
	 * combination of the L3 checksum result and CRC result,based on the
	 * checksum and RoCE/FCoE CRC input selections.
	 */
	AL_ETH_L3_CHK_TYPE_3 = 3,
};

/* selection of the L3 protocol index in the Metadata */
enum al_eth_rx_desc_l3_proto_idx_sel {
	AL_ETH_L3_PROTO_IDX_OUTER = 0, /* set L3 proto index of the outer packet */
	AL_ETH_L3_PROTO_IDX_INNER = 1, /* set L3 proto index of the inner packet */
};

/* selection of the L3 offset in the Metadata */
enum al_eth_rx_desc_l3_offset_sel {
	AL_ETH_L3_OFFSET_OUTER = 0, /* set L3 offset of the outer packet */
	AL_ETH_L3_OFFSET_INNER = 1, /* set L3 offset of the inner packet */
};

/* selection of the L4 protocol index in the Metadata */
enum al_eth_rx_desc_l4_proto_idx_sel {
	AL_ETH_L4_PROTO_IDX_OUTER = 0, /* set L4 proto index of the outer packet */
	AL_ETH_L4_PROTO_IDX_INNER = 1, /* set L4 proto index of the inner packet */
};

/* selection of the frag indication in the Metadata */
enum al_eth_rx_desc_frag_sel {
	AL_ETH_FRAG_OUTER = 0, /* set frag of the outer packet */
	AL_ETH_FRAG_INNER = 1, /* set frag of the inner packet */
};

/* Ethernet Rx completion descriptor */
struct al_eth_rx_cdesc {
	u32 ctrl_meta;
	u32 len;
	u32 word2;
	u32 word3;
};

/* Flow Contol parameters */
struct al_eth_flow_control_params{
	enum al_eth_flow_control_type type; /* flow control type */
	bool		obay_enable; /* stop tx when pause received */
	bool		gen_enable; /* generate pause frames */
	u16	rx_fifo_th_high;
	u16	rx_fifo_th_low;
	u16	quanta;
	u16	quanta_th;
	/*
	 * For each UDMA, defines the mapping between
	 * PFC priority and queues(in bit mask).
	 * same mapping used for obay and generation.
	 * for example:
	 * if prio_q_map[1][7] = 0xC, then TX queues 2
	 * and 3 of UDMA 1 will be stopped when pause
	 * received with priority 7, also, when RX queues
	 * 2 and 3 of UDMA 1 become almost full, then
	 * pause frame with priority 7 will be sent.
	 *
	 * note:
	 * 1) if specific a queue is not used, the caller must
	 * make set the prio_q_map to 0 otherwise that queue
	 * will make the controller keep sending PAUSE packets.
	 * 2) queues of unused UDMA must be treated as above.
	 * 3) when working in LINK PAUSE mode, only entries at
	 * priority 0 will be considered.
	 */
	u8	prio_q_map[4][8];
};

/* Packet Tx flags */
#define AL_ETH_TX_FLAGS_TSO		BIT(7)  /* Enable TCP/UDP segmentation offloading */
#define AL_ETH_TX_FLAGS_IPV4_L3_CSUM	BIT(13) /* Enable IPv4 header checksum calculation */
#define AL_ETH_TX_FLAGS_L4_CSUM		BIT(14) /* Enable TCP/UDP checksum calculation */
#define AL_ETH_TX_FLAGS_L4_PARTIAL_CSUM	BIT(17) /* L4 partial checksum calculation */
#define AL_ETH_TX_FLAGS_L2_MACSEC_PKT	BIT(16) /* L2 Packet type 802_3 or 802_3_MACSEC, V2 */
#define AL_ETH_TX_FLAGS_L2_DIS_FCS	BIT(15) /* Disable CRC calculation*/
#define AL_ETH_TX_FLAGS_TS		BIT(21) /* Timestamp the packet */

#define AL_ETH_TX_FLAGS_INT		AL_M2S_DESC_INT_EN
#define AL_ETH_TX_FLAGS_NO_SNOOP	AL_M2S_DESC_NO_SNOOP_H

/* this structure used for tx packet meta data */
struct al_eth_meta_data{
	u8 store :1; /* store the meta into the queues cache */
	u8 words_valid :4; /* valid bit per word */

	u8 vlan1_cfi_sel:2;
	u8 vlan2_vid_sel:2;
	u8 vlan2_cfi_sel:2;
	u8 vlan2_pbits_sel:2;
	u8 vlan2_ether_sel:2;

	u16 vlan1_new_vid:12;
	u8 vlan1_new_cfi :1;
	u8 vlan1_new_pbits :3;
	u16 vlan2_new_vid:12;
	u8 vlan2_new_cfi :1;
	u8 vlan2_new_pbits :3;

	u8 l3_header_len; /* in bytes */
	u8 l3_header_offset;
	u8 l4_header_len; /* in words(32-bits) */

	/* rev 0 specific */
	u8 mss_idx_sel:3; /* for TSO, select the register that holds the MSS */

	/* rev 1 specific */
	u8 ts_index:4; /* index of regiser where to store the tx timestamp */
	u16 mss_val :14; /* for TSO, set the mss value */
	u8 outer_l3_offset; /* for tunneling mode. up to 64 bytes */
	u8 outer_l3_len; /* for tunneling mode. up to 128 bytes */
};

/* Packet Rx flags when adding buffer to receive queue */

/*
 * Target-ID to be assigned to the packet descriptors
 * Requires Target-ID in descriptor to be enabled for the specific UDMA
 * queue.
 */
#define AL_ETH_RX_FLAGS_TGTID_MASK	GENMASK(15, 0)
#define AL_ETH_RX_FLAGS_INT		AL_M2S_DESC_INT_EN

/* Packet Rx flags set by HW when receiving packet */
#define AL_ETH_RX_ERROR			BIT(16) /* layer 2 errors (FCS, bad len, etc) */
#define AL_ETH_RX_FLAGS_L4_CSUM_ERR	BIT(14)
#define AL_ETH_RX_FLAGS_L3_CSUM_ERR	BIT(13)

/* Packet Rx flags - word 3 in Rx completion descriptor */

/* packet structure. used for packet transmission and reception */
struct al_eth_pkt {
	u32 flags; /* see flags above, depends on context(tx or rx) */
	enum AL_ETH_PROTO_ID l3_proto_idx;
	enum AL_ETH_PROTO_ID l4_proto_idx;
	u8 source_vlan_count:2;
	u8 vlan_mod_add_count:2;
	u8 vlan_mod_del_count:2;
	u8 vlan_mod_v1_ether_sel:2;
	u8 vlan_mod_v1_vid_sel:2;
	u8 vlan_mod_v1_pbits_sel:2;

	/* rev 1 specific */
	enum AL_ETH_TX_TUNNEL_MODE tunnel_mode;
	enum AL_ETH_PROTO_ID outer_l3_proto_idx; /* for tunneling mode */

	/*
	 * Target-ID to be assigned to the packet descriptors
	 * Requires Target-ID in descriptor to be enabled for the specific UDMA
	 * queue.
	 */
	u16 tgtid;

	u32 rx_header_len; /* header buffer length of rx packet, not used */
	struct al_eth_meta_data *meta; /* if null, then no meta added */
	u16 rxhash;
	u16 l3_offset;

	u8 num_of_bufs;
	struct al_buf bufs[AL_ETH_PKT_MAX_BUFS];
};

/* Ethernet Adapter private data structure used by this driver */
struct al_hw_eth_adapter {
	u8 rev_id; /* PCI adapter revision ID */
	u8 udma_id; /* the id of the UDMA used by this adapter */

	struct net_device *netdev;

	void __iomem *unit_regs;
	void __iomem *udma_regs_base;
	void __iomem *ec_regs_base;
	void __iomem *ec_ints_base;
	void __iomem *mac_regs_base;
	struct interrupt_controller_ctrl __iomem *mac_ints_base;

	char *name; /* the upper layer must keep the string area */

	struct al_udma tx_udma;
	struct al_udma rx_udma;

	u8 enable_rx_parser; /* config and enable rx parsing */

	enum al_eth_flow_control_type fc_type; /* flow control*/

	enum al_eth_mac_mode mac_mode;
	enum al_eth_mdio_if	mdio_if; /* which mac mdio interface to use */
	enum al_eth_mdio_type mdio_type; /* mdio protocol type */
	bool shared_mdio_if; /* when true, the mdio interface is shared with other controllers.*/
	u8 curr_lt_unit;
};

/* parameters from upper layer */
struct al_eth_adapter_params {
	u8 rev_id; /* PCI adapter revision ID */
	u8 udma_id; /* the id of the UDMA used by this adapter */
	struct net_device *netdev;
	u8 enable_rx_parser; /* when true, the rx epe parser will be enabled */
	void __iomem *udma_regs_base; /* UDMA register base address */
	void __iomem *ec_regs_base;
	void __iomem *mac_regs_base;
	char *name; /* the upper layer must keep the string area */
};

/*
 * initialize the ethernet adapter's DMA
 * - initialize the adapter data structure
 * - initialize the Tx and Rx UDMA
 * - enable the Tx and Rx UDMA, the rings will be still disabled at this point.
 *
 * @param adapter pointer to the private structure
 * @param params the parameters passed from upper layer
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_adapter_init(struct al_hw_eth_adapter *adapter, struct al_eth_adapter_params *params);

/*
 * stop the DMA of the ethernet adapter
 *
 * @param adapter pointer to the private structure
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_adapter_stop(struct al_hw_eth_adapter *adapter);

/*
 * Configure and enable a queue ring
 *
 * @param adapter pointer to the private structure
 * @param type tx or rx
 * @param qid queue index
 * @param q_params queue parameters
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_queue_config(struct al_hw_eth_adapter *adapter, enum al_udma_type type, u32 qid,
			struct al_udma_q_params *q_params);

/* MAC layer */

/*
 * configure the mac media type.
 * this function only sets the mode, but not the speed as certain mac modes
 * support multiple speeds as will be negotiated by the link layer.
 * @param adapter pointer to the private structure.
 * @param mode media mode
 *
 * @return 0 on success. negative errno on failure.
 */
int al_eth_mac_config(struct al_hw_eth_adapter *adapter, enum al_eth_mac_mode mode);

/*
 * stop the mac tx and rx paths.
 * @param adapter pointer to the private structure.
 *
 * @return 0 on success. negative error on failure.
 */
int al_eth_mac_stop(struct al_hw_eth_adapter *adapter);

/*
 * start the mac tx and rx paths.
 * @param adapter pointer to the private structure.
 *
 * @return 0 on success. negative error on failure.
 */
int al_eth_mac_start(struct al_hw_eth_adapter *adapter);

/*
 * update link auto negotiation speed and duplex mode
 * this function assumes the mac mode already set using the al_eth_mac_config()
 * function.
 *
 * @param adapter pointer to the private structure
 * @param force_1000_base_x set to true to force the mac to work on 1000baseX
 *	  (not relevant to RGMII)
 * @param an_enable set to true to enable auto negotiation
 *	  (not relevant to RGMII)
 * @param speed in mega bits, e.g 1000 stands for 1Gbps (relevant only in case
 *	  an_enable is false)
 * @param full_duplex set to true to enable full duplex mode (relevant only
 *	  in case an_enable is false)
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_mac_link_config(struct al_hw_eth_adapter *adapter,
			   bool force_1000_base_x,
			   bool an_enable,
			   u32 speed,
			   bool full_duplex);

/*
 * configure minimum and maximum rx packet length
 *
 * @param adapter pointer to the private structure
 * @param min_rx_len minimum rx packet length
 * @param max_rx_len maximum rx packet length
 * both length limits in bytes and it includes the MAC Layer header and FCS.
 * @return 0 on success, otherwise on failure.
 */
int al_eth_rx_pkt_limit_config(struct al_hw_eth_adapter *adapter, u32 min_rx_len, u32 max_rx_len);

/* Reference clock frequency (platform specific) */
enum al_eth_ref_clk_freq {
	AL_ETH_REF_FREQ_375_MHZ		= 0,
	AL_ETH_REF_FREQ_187_5_MHZ	= 1,
	AL_ETH_REF_FREQ_250_MHZ		= 2,
	AL_ETH_REF_FREQ_500_MHZ		= 3,
	AL_ETH_REF_FREQ_428_MHZ         = 4,
};

/*
 * configure the MDIO hardware interface
 * @param adapter pointer to the private structure
 * @param mdio_type clause type
 * @param shared_mdio_if set to true if multiple controllers using the same
 * @param ref_clk_freq reference clock frequency
 * @param mdio_clk_freq_khz the required MDC/MDIO clock frequency [Khz]
 * MDIO pins of the chip.
 *
 * @return 0 on success, otherwise on failure.
 */
int al_eth_mdio_config(struct al_hw_eth_adapter *adapter,
		       enum al_eth_mdio_type mdio_type,
		       bool shared_mdio_if,
		       enum al_eth_ref_clk_freq ref_clk_freq,
		       unsigned int mdio_clk_freq_khz);

/*
 * read mdio register
 * this function uses polling mode, and as the mdio is slow interface, it might
 * block the cpu for long time (milliseconds).
 * @param adapter pointer to the private structure
 * @param phy_addr address of mdio phy
 * @param device address of mdio device (used only in CLAUSE 45)
 * @param reg index of the register
 * @param val pointer for read value of the register
 *
 * @return 0 on success, negative errno on failure
 */
int al_eth_mdio_read(struct al_hw_eth_adapter *adapter, u32 phy_addr,
		     u32 device, u32 reg, u16 *val);

/*
 * write mdio register
 * this function uses polling mode, and as the mdio is slow interface, it might
 * block the cpu for long time (milliseconds).
 * @param adapter pointer to the private structure
 * @param phy_addr address of mdio phy
 * @param device address of mdio device (used only in CLAUSE 45)
 * @param reg index of the register
 * @param val value to write
 *
 * @return 0 on success, negative errno on failure
 */
int al_eth_mdio_write(struct al_hw_eth_adapter *adapter, u32 phy_addr,
		      u32 device, u32 reg, u16 val);

/*
 * prepare packet descriptors in tx queue.
 *
 * This functions prepares the descriptors for the given packet in the tx
 * submission ring. the caller must call al_eth_tx_pkt_action() below
 * in order to notify the hardware about the new descriptors.
 *
 * @param tx_dma_q pointer to UDMA tx queue
 * @param pkt the packet to transmit
 *
 * @return number of descriptors used for this packet, 0 if no free
 * room in the descriptors ring
 */
int al_eth_tx_pkt_prepare(struct al_hw_eth_adapter *adapter,
			  struct al_udma_q *tx_dma_q, struct al_eth_pkt *pkt);

/*
 * Trigger the DMA about previously added tx descriptors.
 *
 * @param tx_dma_q pointer to UDMA tx queue
 * @param tx_descs number of descriptors to notify the DMA about.
 * the tx_descs can be sum of descriptor numbers of multiple prepared packets,
 * this way the caller can use this function to notify the DMA about multiple
 * packets.
 */
void al_eth_tx_dma_action(struct al_udma_q *tx_dma_q, u32 tx_descs);

/*
 * get number of completed tx descriptors, upper layer should derive from
 * this information which packets were completed.
 *
 * @param tx_dma_q pointer to UDMA tx queue
 *
 * @return number of completed tx descriptors.
 */
int al_eth_comp_tx_get(struct al_hw_eth_adapter *adapter,
		       struct al_udma_q *tx_dma_q);

/*
 * add buffer to receive queue
 *
 * @param rx_dma_q pointer to UDMA rx queue
 * @param buf pointer to data buffer
 * @param flags bitwise of AL_ETH_RX_FLAGS
 * @param header_buf this is not used for far and header_buf should be set to
 * NULL.
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_rx_buffer_add(struct al_hw_eth_adapter *adapter,
			 struct al_udma_q *rx_dma_q, struct al_buf *buf,
			 u32 flags, struct al_buf *header_buf);

/*
 * notify the hw engine about rx descriptors that were added to the receive queue
 *
 * @param rx_dma_q pointer to UDMA rx queue
 * @param descs_num number of rx descriptors
 */
void al_eth_rx_buffer_action(struct al_hw_eth_adapter *adapter,
			     struct al_udma_q *rx_dma_q, u32 descs_num);

/*
 * get packet from RX completion ring
 *
 * @param rx_dma_q pointer to UDMA rx queue
 * @param pkt pointer to a packet data structure, this function fills this
 * structure with the information about the received packet. the buffers
 * structures filled only with the length of the data written into the buffer,
 * the address fields are not updated as the upper layer can retrieve this
 * information by itself because the hardware uses the buffers in the same order
 * were those buffers inserted into the ring of the receive queue.
 * this structure should be allocated by the caller function.
 *
 * @return return number of descriptors or 0 if no completed packet found.
 */
u32 al_eth_pkt_rx(struct al_hw_eth_adapter *adapter, struct al_udma_q *rx_dma_q,
		  struct al_eth_pkt *pkt);

/* RX parser table */
struct al_eth_epe_p_reg_entry {
	u32 data;
	u32 mask;
	u32 ctrl;
};

struct al_eth_epe_control_entry {
	u32 data[6];
};

/* Flow Steering and filtering */
int al_eth_thash_table_set(struct al_hw_eth_adapter *adapter, u32 idx, u8 udma, u32 queue);

/*
 * FSM table has 7 bits input address:
 *  bits[2:0] are the outer packet's type (IPv4, TCP...)
 *  bits[5:3] are the inner packet's type
 *  bit[6] is set when packet is tunneled.
 *
 * The output of each entry:
 *  bits[1:0] - input selection: selects the input for the thash (2/4 tuple, inner/outer)
 *  bit[2] - selects whether to use thash output, or default values for the queue and udma
 *  bits[6:3] default UDMA mask: the UDMAs to select when bit 2 above was unset
 *  bits[9:5] defualt queue: the queue index to select when bit 2 above was unset
 */

#define AL_ETH_FSM_ENTRY_IPV4_TCP	   0
#define AL_ETH_FSM_ENTRY_IPV4_UDP	   1
#define AL_ETH_FSM_ENTRY_IPV6_TCP	   2
#define AL_ETH_FSM_ENTRY_IPV6_UDP	   3
#define AL_ETH_FSM_ENTRY_IPV6_NO_UDP_TCP   4
#define AL_ETH_FSM_ENTRY_IPV4_NO_UDP_TCP   5

#define AL_ETH_FSM_ENTRY_OUTER(idx)	   ((idx) & 7)

/* FSM DATA format */
#define AL_ETH_FSM_DATA_OUTER_2_TUPLE	0
#define AL_ETH_FSM_DATA_OUTER_4_TUPLE	1

#define AL_ETH_FSM_DATA_HASH_SEL	BIT(2)

#define AL_ETH_FSM_DATA_DEFAULT_Q_SHIFT		5
#define AL_ETH_FSM_DATA_DEFAULT_UDMA_SHIFT	3

/* set fsm table entry */
int al_eth_fsm_table_set(struct al_hw_eth_adapter *adapter, u32 idx, u32 entry);

enum AL_ETH_FWD_CTRL_IDX_VLAN_TABLE_OUT {
	AL_ETH_FWD_CTRL_IDX_VLAN_TABLE_OUT_0 = 0,
	AL_ETH_FWD_CTRL_IDX_VLAN_TABLE_OUT_1 = 1,
	AL_ETH_FWD_CTRL_IDX_VLAN_TABLE_OUT_ANY = 2,
};

enum AL_ETH_FWD_CTRL_IDX_TUNNEL {
	AL_ETH_FWD_CTRL_IDX_TUNNEL_NOT_EXIST = 0,
	AL_ETH_FWD_CTRL_IDX_TUNNEL_EXIST = 1,
	AL_ETH_FWD_CTRL_IDX_TUNNEL_ANY = 2,
};

enum AL_ETH_FWD_CTRL_IDX_VLAN {
	AL_ETH_FWD_CTRL_IDX_VLAN_NOT_EXIST = 0,
	AL_ETH_FWD_CTRL_IDX_VLAN_EXIST = 1,
	AL_ETH_FWD_CTRL_IDX_VLAN_ANY = 2,
};

enum AL_ETH_FWD_CTRL_IDX_MAC_TABLE {
	AL_ETH_FWD_CTRL_IDX_MAC_TABLE_NO_MATCH = 0,
	AL_ETH_FWD_CTRL_IDX_MAC_TABLE_MATCH = 1,
	AL_ETH_FWD_CTRL_IDX_MAC_TABLE_ANY = 2,
};

enum AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE {
	AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE_UC = 0, /* unicast */
	AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE_MC = 1, /* multicast */
	AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE_BC = 2, /* broadcast */
	AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE_ANY = 4, /* for sw usage */
};

enum AL_ETH_CTRL_TABLE_PRIO_SEL {
	AL_ETH_CTRL_TABLE_PRIO_SEL_PBITS_TABLE	= 0,
	AL_ETH_CTRL_TABLE_PRIO_SEL_DSCP_TABLE	= 1,
	AL_ETH_CTRL_TABLE_PRIO_SEL_TC_TABLE	= 2,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG1		= 3,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG2		= 4,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG3		= 5,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG4		= 6,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG5		= 7,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG6		= 7,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG7		= 9,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG8		= 10,
	AL_ETH_CTRL_TABLE_PRIO_SEL_VAL_3	= 11,
	AL_ETH_CTRL_TABLE_PRIO_SEL_VAL_0	= 12,
};

/* where to select the initial queue from */
enum AL_ETH_CTRL_TABLE_QUEUE_SEL_1 {
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_PRIO_TABLE	= 0,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_THASH_TABLE	= 1,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_MAC_TABLE		= 2,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_MHASH_TABLE	= 3,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_REG1		= 4,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_REG2		= 5,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_REG3		= 6,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_REG4		= 7,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_VAL_3		= 12,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_VAL_0		= 13,
};

/* target queue will be built up from the priority and initial queue */
enum AL_ETH_CTRL_TABLE_QUEUE_SEL_2 {
	AL_ETH_CTRL_TABLE_QUEUE_SEL_2_PRIO_TABLE	= 0, /* target queue is the output of priority table */
	AL_ETH_CTRL_TABLE_QUEUE_SEL_2_PRIO		= 1, /* target queue is the priority */
	AL_ETH_CTRL_TABLE_QUEUE_SEL_2_PRIO_QUEUE	= 2, /* target queue is initial queue[0], priority[1] */
	AL_ETH_CTRL_TABLE_QUEUE_SEL_2_NO_PRIO		= 3, /* target queue is the initial */
};

enum AL_ETH_CTRL_TABLE_UDMA_SEL {
	AL_ETH_CTRL_TABLE_UDMA_SEL_THASH_TABLE		= 0,
	AL_ETH_CTRL_TABLE_UDMA_SEL_THASH_AND_VLAN	= 1,
	AL_ETH_CTRL_TABLE_UDMA_SEL_VLAN_TABLE		= 2,
	AL_ETH_CTRL_TABLE_UDMA_SEL_VLAN_AND_MAC		= 3,
	AL_ETH_CTRL_TABLE_UDMA_SEL_MAC_TABLE		= 4,
	AL_ETH_CTRL_TABLE_UDMA_SEL_MAC_AND_MHASH	= 5,
	AL_ETH_CTRL_TABLE_UDMA_SEL_MHASH_TABLE		= 6,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG1			= 7,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG2			= 8,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG3			= 9,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG4			= 10,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG5			= 11,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG6			= 12,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG7			= 13,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG8			= 14,
	AL_ETH_CTRL_TABLE_UDMA_SEL_VAL_0		= 15,
};

struct al_eth_fwd_ctrl_table_entry {
	enum AL_ETH_CTRL_TABLE_PRIO_SEL prio_sel;
	enum AL_ETH_CTRL_TABLE_QUEUE_SEL_1 queue_sel_1; /* queue id source */
	enum AL_ETH_CTRL_TABLE_QUEUE_SEL_2 queue_sel_2; /* mix queue id with priority */
	enum AL_ETH_CTRL_TABLE_UDMA_SEL udma_sel;
	bool filter; /* set to true to enable filtering */
};

/*
 * Configure default control table entry
 *
 * @param adapter pointer to the private structure
 * @param use_table set to true if control table is used, when set to false
 * then control table will be bypassed and the entry value will be used.
 * @param entry defines the value to be used when bypassing control table.
 *
 * @return 0 on success. otherwise on failure.
 */
void al_eth_ctrl_table_def_set(struct al_hw_eth_adapter *adapter,
			       bool use_table,
			       struct al_eth_fwd_ctrl_table_entry *entry);

/*
 * Configure hash key initial registers
 * Those registers define the initial key values, those values used for
 * the THASH and MHASH hash functions.
 *
 * @param adapter pointer to the private structure
 * @param idx the register index
 * @param val the register value
 *
 * @return 0 on success. otherwise on failure.
 */
void al_eth_hash_key_set(struct al_hw_eth_adapter *adapter, u32 idx, u32 val);

struct al_eth_fwd_mac_table_entry {
	u8		addr[6]; /* byte 0 is the first byte seen on the wire */
	u8		mask[6];
	bool		tx_valid;
	u8		tx_target;
	bool		rx_valid;
	u8		udma_mask; /* target udma */
	u8		qid; /* target queue */
	bool		filter; /* set to true to enable filtering */
};

/*
 * Configure mac table entry
 * The HW traverse this table and looks for match from lowest index,
 * when the packets MAC DA & mask == addr, and the valid bit is set, then match occurs.
 *
 * @param adapter pointer to the private structure
 * @param idx the entry index within the mac table.
 * @param entry the contents of the MAC table entry
 */
void al_eth_fwd_mac_table_set(struct al_hw_eth_adapter *adapter, u32 idx,
			      struct al_eth_fwd_mac_table_entry *entry);

void al_eth_mac_addr_store(void * __iomem ec_base, u32 idx, u8 *addr);
void al_eth_mac_addr_read(void * __iomem ec_base, u32 idx, u8 *addr);

/*
 * Configure pbits table entry
 * The HW uses this table to translate between vlan pbits field to priority.
 * The vlan pbits is used as the index of this table.
 *
 * @param adapter pointer to the private structure
 * @param idx the entry index within the table.
 * @param prio the priority to set for this entry
 */
void al_eth_fwd_pbits_table_set(struct al_hw_eth_adapter *adapter, u32 idx, u8 prio);

/*
 * Configure priority table entry
 * The HW uses this table to translate between priority to queue index.
 * The priority is used as the index of this table.
 *
 * @param adapter pointer to the private structure
 * @param prio the entry index within the table.
 * @param qid the queue index to set for this entry (priority).
 */
void al_eth_fwd_priority_table_set(struct al_hw_eth_adapter *adapter, u8 prio, u8 qid);

/* filter undetected MAC DA */
#define AL_ETH_RFW_FILTER_UNDET_MAC          BIT(0)
/* filter specific MAC DA based on MAC table output */
#define AL_ETH_RFW_FILTER_DET_MAC            BIT(1)
/* filter all tagged */
#define AL_ETH_RFW_FILTER_TAGGED             BIT(2)
/* filter all untagged */
#define AL_ETH_RFW_FILTER_UNTAGGED           BIT(3)
/* filter all broadcast */
#define AL_ETH_RFW_FILTER_BC                 BIT(4)
/* filter all multicast */
#define AL_ETH_RFW_FILTER_MC                 BIT(5)
/* filter packet based on parser drop */
#define AL_ETH_RFW_FILTER_PARSE              BIT(6)
/* filter packet based on VLAN table output */
#define AL_ETH_RFW_FILTER_VLAN_VID           BIT(7)
/* filter packet based on control table output */
#define AL_ETH_RFW_FILTER_CTRL_TABLE         BIT(8)
/* filter packet based on protocol index */
#define AL_ETH_RFW_FILTER_PROT_INDEX         BIT(9)
/* filter packet based on WoL decision */
#define AL_ETH_RFW_FILTER_WOL		     BIT(10)

struct al_eth_filter_params {
	bool		enable;
	u32	filters; /* bitmask of AL_ETH_RFW_FILTER.. for filters to enable */
	bool		filter_proto[AL_ETH_PROTOCOLS_NUM]; /* set true for protocols to filter */
};

/*
 * Configure the receive filters
 * this function enables/disables filtering packets and which filtering
 * types to apply.
 * filters that indicated in tables (MAC table, VLAN and Control tables)
 * are not configured by this function. This functions only enables/disables
 * respecting the filter indication from those tables.
 *
 * @param adapter pointer to the private structure
 * @param params the parameters passed from upper layer
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_filter_config(struct al_hw_eth_adapter *adapter, struct al_eth_filter_params *params);

int al_eth_flow_control_config(struct al_hw_eth_adapter *adapter, struct al_eth_flow_control_params *params);

struct al_eth_eee_params {
	u8 enable;
	u32 tx_eee_timer; /* time in cycles the interface delays prior to entering eee state */
	u32 min_interval; /* minimum interval in cycles between two eee states */
	u32 stop_cnt; /* time in cycles to stop Tx mac i/f after getting out of eee state */
	bool fast_wake; /* fast_wake is only applicable to 40/50G, otherwise the mode is deep_sleep */
};

/*
 * configure EEE mode
 * @param adapter pointer to the private structure.
 * @param params pointer to the eee input parameters.
 *
 * @return return 0 on success. otherwise on failure.
 */
int al_eth_eee_config(struct al_hw_eth_adapter *adapter,
		      struct al_eth_eee_params *params);

/*
 * get EEE configuration
 * @param adapter pointer to the private structure.
 * @param params pointer to the eee output parameters.
 *
 * @return return 0 on success. otherwise on failure.
 */
int al_eth_eee_get(struct al_hw_eth_adapter *adapter,
		   struct al_eth_eee_params *params);

/* enum for methods when updating systime using triggers */
enum al_eth_pth_update_method {
	AL_ETH_PTH_UPDATE_METHOD_SET = 0, /* Set the time in int/ext update time */
	AL_ETH_PTH_UPDATE_METHOD_INC = 1, /* increment */
	AL_ETH_PTH_UPDATE_METHOD_DEC = 2, /* decrement */
	AL_ETH_PTH_UPDATE_METHOD_ADD_TO_LAST = 3, /* Set to last time + int/ext update time.*/
};

/* systime internal update trigger types */
enum al_eth_pth_int_trig {
	/* use output pulse as trigger */
	AL_ETH_PTH_INT_TRIG_OUT_PULSE_0 = 0,
	/* use the int update register write as a trigger */
	AL_ETH_PTH_INT_TRIG_REG_WRITE = 1,
};

/* get statistics */
struct al_eth_mac_stats {
	/* sum the data and padding octets (i.e. without header and FCS) received with a valid frame. */
	u64 aOctetsReceivedOK;
	/* sum of Payload and padding octets of frames transmitted without error*/
	u64 aOctetsTransmittedOK;
	/* total number of packets received. Good and bad packets */
	u32 etherStatsPkts;
	/* number of received unicast packets */
	u32 ifInUcastPkts;
	/* number of received multicast packets */
	u32 ifInMulticastPkts;
	/* number of received broadcast packets */
	u32 ifInBroadcastPkts;
	/* Number of frames received with FIFO Overflow, CRC, Payload Length, Jabber and Oversized, Alignment or PHY/PCS error indication */
	u32 ifInErrors;

	/* number of transmitted unicast packets */
	u32 ifOutUcastPkts;
	/* number of transmitted multicast packets */
	u32 ifOutMulticastPkts;
	/* number of transmitted broadcast packets */
	u32 ifOutBroadcastPkts;
	/* number of frames transmitted with FIFO Overflow, FIFO Underflow or Controller indicated error */
	u32 ifOutErrors;

	/* number of Frame received without error (Including Pause Frames). */
	u32 aFramesReceivedOK;
	/* number of Frames transmitter without error (Including Pause Frames) */
	u32 aFramesTransmittedOK;
	/* number of packets received with less than 64 octets */
	u32 etherStatsUndersizePkts;
	/* Too short frames with CRC error, available only for RGMII and 1G Serial modes */
	u32 etherStatsFragments;
	/* Too long frames with CRC error */
	u32 etherStatsJabbers;
	/* packet that exceeds the valid maximum programmed frame length */
	u32 etherStatsOversizePkts;
	/* number of frames received with a CRC error */
	u32 aFrameCheckSequenceErrors;
	/* number of frames received with alignment error */
	u32 aAlignmentErrors;
	/* number of dropped packets due to FIFO overflow */
	u32 etherStatsDropEvents;
	/* number of transmitted pause frames. */
	u32 aPAUSEMACCtrlFramesTransmitted;
	/* number of received pause frames. */
	u32 aPAUSEMACCtrlFramesReceived;
	/* frame received exceeded the maximum length programmed with register FRM_LGTH, available only for 10G modes */
	u32 aFrameTooLongErrors;
	/*
	 * Received frame with bad length/type (between 46 and 0x600 or less
	 * than 46 for packets longer than 64), available only for 10G modes
	 */
	u32 aInRangeLengthErrors;
	/* Valid VLAN tagged frames transmitted */
	u32 VLANTransmittedOK;
	/* Valid VLAN tagged frames received */
	u32 VLANReceivedOK;
	/* Total number of octets received. Good and bad packets */
	u32 etherStatsOctets;

	/* packets of 64 octets length is received (good and bad frames are counted) */
	u32 etherStatsPkts64Octets;
	/* Frames (good and bad) with 65 to 127 octets */
	u32 etherStatsPkts65to127Octets;
	/* Frames (good and bad) with 128 to 255 octets */
	u32 etherStatsPkts128to255Octets;
	/* Frames (good and bad) with 256 to 511 octets */
	u32 etherStatsPkts256to511Octets;
	/* Frames (good and bad) with 512 to 1023 octets */
	u32 etherStatsPkts512to1023Octets;
	/* Frames (good and bad) with 1024 to 1518 octets */
	u32 etherStatsPkts1024to1518Octets;
	/* frames with 1519 bytes to the maximum length programmed in the register FRAME_LENGTH. */
	u32 etherStatsPkts1519toX;

	u32 eee_in;
	u32 eee_out;
};

/*
 * perform Function Level Reset RMN
 *
 * Addressing RMN: 714
 *
 * @param pci_read_config_u32 pointer to function that reads register from pci header
 * @param pci_write_config_u32 pointer to function that writes register from pci header
 * @param handle pointer passes to the above functions as first parameter
 * @param mac_base base address of the MAC registers
 *
 * @return 0.
 */
int al_eth_flr_rmn(int (*pci_read_config_u32)(void *handle, int where, u32 *val),
		   int (*pci_write_config_u32)(void *handle, int where, u32 val),
		   void *handle, void __iomem	*mac_base);

enum al_eth_board_media_type {
	AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT		= 0,
	AL_ETH_BOARD_MEDIA_TYPE_RGMII			= 1,
	AL_ETH_BOARD_MEDIA_TYPE_10GBASE_SR		= 2,
	AL_ETH_BOARD_MEDIA_TYPE_SGMII			= 3,
	AL_ETH_BOARD_MEDIA_TYPE_1000BASE_X		= 4,
	AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT_AUTO_SPEED	= 5,
	AL_ETH_BOARD_MEDIA_TYPE_SGMII_2_5G		= 6,
	AL_ETH_BOARD_MEDIA_TYPE_NBASE_T			= 7,
	AL_ETH_BOARD_MEDIA_TYPE_25G			= 8,
};

enum al_eth_board_mdio_freq {
	AL_ETH_BOARD_MDIO_FREQ_2_5_MHZ	= 0,
	AL_ETH_BOARD_MDIO_FREQ_1_MHZ	= 1,
};

enum al_eth_board_ext_phy_if {
	AL_ETH_BOARD_PHY_IF_MDIO	= 0,
	AL_ETH_BOARD_PHY_IF_XMDIO	= 1,
	AL_ETH_BOARD_PHY_IF_I2C		= 2,

};

enum al_eth_board_auto_neg_mode {
	AL_ETH_BOARD_AUTONEG_OUT_OF_BAND	= 0,
	AL_ETH_BOARD_AUTONEG_IN_BAND		= 1,

};

/* declare the 1G mac active speed when auto negotiation disabled */
enum al_eth_board_1g_speed {
	AL_ETH_BOARD_1G_SPEED_1000M		= 0,
	AL_ETH_BOARD_1G_SPEED_100M		= 1,
	AL_ETH_BOARD_1G_SPEED_10M		= 2,
};

enum al_eth_retimer_channel {
	AL_ETH_RETIMER_CHANNEL_A		= 0,
	AL_ETH_RETIMER_CHANNEL_B		= 1,
	AL_ETH_RETIMER_CHANNEL_C		= 2,
	AL_ETH_RETIMER_CHANNEL_D		= 3,
	AL_ETH_RETIMER_CHANNEL_E		= 4,
	AL_ETH_RETIMER_CHANNEL_F		= 5,
	AL_ETH_RETIMER_CHANNEL_G		= 6,
	AL_ETH_RETIMER_CHANNEL_H		= 7,
	AL_ETH_RETIMER_CHANNEL_MAX		= 8
};

/* list of supported retimers */
enum al_eth_retimer_type {
	AL_ETH_RETIMER_BR_210			= 0,
	AL_ETH_RETIMER_BR_410			= 1,
	AL_ETH_RETIMER_DS_25			= 2,
	AL_ETH_RETIMER_TYPE_MAX			= 4,
};

/*
 * Structure represents the board information. this info set by boot loader
 * and read by OS driver.
 */
struct al_eth_board_params {
	enum al_eth_board_media_type	media_type;
	bool		phy_exist; /* external phy exist */
	u8		phy_mdio_addr; /* mdio address of external phy */
	bool		sfp_plus_module_exist; /* SFP+ module connected */
	bool		autoneg_enable; /* enable Auto-Negotiation */
	bool		kr_lt_enable; /* enable KR Link-Training */
	bool		kr_fec_enable; /* enable KR FEC */
	enum al_eth_board_mdio_freq	mdio_freq; /* MDIO frequency */
	u8		i2c_adapter_id; /* identifier for the i2c adapter to use to access SFP+ module */
	enum al_eth_board_ext_phy_if	phy_if; /* phy interface */
	enum al_eth_board_auto_neg_mode	an_mode; /* auto-negotiation mode (in-band / out-of-band) */
	enum al_eth_ref_clk_freq	ref_clk_freq; /* reference clock frequency */
	bool		force_1000_base_x; /* set mac to 1000 base-x mode (instead sgmii) */
	bool		an_disable; /* disable auto negotiation */
	enum al_eth_board_1g_speed	speed; /* port speed if AN disabled */
	bool		half_duplex; /* force half duplex if AN disabled */
	bool		fc_disable; /* disable flow control */
	bool		retimer_exist; /* retimer is exist on the board */
	u8		retimer_bus_id; /* in what i2c bus the retimer is on */
	u8		retimer_i2c_addr; /* i2c address of the retimer */
	enum al_eth_retimer_channel retimer_channel; /* what channel connected to this port (Rx) */
	bool		dac; /* assume direct attached cable is connected if auto detect is off or failed */
	u8		dac_len; /* assume this cable length if auto detect is off or failed  */
	enum al_eth_retimer_type retimer_type; /* the type of the specific retimer */
	enum al_eth_retimer_channel retimer_tx_channel; /* what channel connected to this port (Tx) */
	u8		gpio_sfp_present; /* gpio number of sfp present for this port. 0 if not exist */
};

/*
 * set board parameter of the eth port
 * this function used to set the board parameters into scratchpad
 * registers. those parameters can be read later by OS driver.
 *
 * @param mac_base the virtual address of the mac registers (PCI BAR 2)
 * @param params pointer to structure the includes the parameters
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_board_params_set(void * __iomem mac_base, struct al_eth_board_params *params);

/*
 * get board parameter of the eth port
 * this function used to get the board parameters from scratchpad
 * registers.
 *
 * @param mac_base the virtual address of the mac registers (PCI BAR 2)
 * @param params pointer to structure where the parameters will be stored.
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_board_params_get(void * __iomem mac_base, struct al_eth_board_params *params);

/*
 * Wake-On-Lan (WoL)
 *
 * The following few functions configure the Wake-On-Lan packet detection
 * inside the Integrated Ethernet MAC.
 *
 * There are other alternative ways to set WoL, such using the
 * external 1000Base-T transceiver to set WoL mode.
 *
 * These APIs do not set the system-wide power-state, nor responsible on the
 * transition from Sleep to Normal power state.
 *
 * For system level considerations, please refer to Annapurna Labs Alpine Wiki.
 */
/* Interrupt enable WoL MAC DA Unicast detected  packet */
#define AL_ETH_WOL_INT_UNICAST		BIT(0)
/* Interrupt enable WoL L2 Multicast detected  packet */
#define AL_ETH_WOL_INT_MULTICAST	BIT(1)
/* Interrupt enable WoL L2 Broadcast detected  packet */
#define AL_ETH_WOL_INT_BROADCAST	BIT(2)
/* Interrupt enable WoL IPv4 detected  packet */
#define AL_ETH_WOL_INT_IPV4		BIT(3)
/* Interrupt enable WoL IPv6 detected  packet */
#define AL_ETH_WOL_INT_IPV6		BIT(4)
/* Interrupt enable WoL EtherType+MAC DA detected  packet */
#define AL_ETH_WOL_INT_ETHERTYPE_DA	BIT(5)
/* Interrupt enable WoL EtherType+L2 Broadcast detected  packet */
#define AL_ETH_WOL_INT_ETHERTYPE_BC	BIT(6)
/* Interrupt enable WoL parser detected  packet */
/* Interrupt enable WoL magic detected  packet */
#define AL_ETH_WOL_INT_MAGIC		BIT(8)
/* Interrupt enable WoL magic+password detected  packet */
#define AL_ETH_WOL_INT_MAGIC_PSWD	BIT(9)

/* Forward enable WoL MAC DA Unicast detected  packet */
#define AL_ETH_WOL_FWRD_UNICAST		BIT(0)
/* Forward enable WoL L2 Multicast detected  packet */
#define AL_ETH_WOL_FWRD_MULTICAST	BIT(1)
/* Forward enable WoL L2 Broadcast detected  packet */
#define AL_ETH_WOL_FWRD_BROADCAST	BIT(2)
/* Forward enable WoL IPv4 detected  packet */
/* Forward enable WoL IPv6 detected  packet */
/* Forward enable WoL EtherType+MAC DA detected  packet */
/* Forward enable WoL EtherType+L2 Broadcast detected  packet */
/* Forward enable WoL parser detected  packet */

struct al_eth_wol_params {
	 /* 6 bytes array of destanation address for magic packet detection */
	u8 *dest_addr;
	u8 *pswd;	/* 6 bytes array of the password to use */
	u8 *ipv4;
	u8 *ipv6;
	u16 ethr_type1; /* first ethertype to use */
	u16 ethr_type2; /* secound ethertype to use */
	/*
	 * Bitmask of AL_ETH_WOL_FWRD_* of the packet types needed to be
	 * forwarded.
	 */
	u16 forward_mask;
	/*
	 * Bitmask of AL_ETH_WOL_INT_* of the packet types that will send
	 * interrupt to wake the system.
	 */
	u16 int_mask;
};

/*
 * enable the wol mechanism
 * set what type of packets will wake up the system and what type of packets
 * neet to forward after the system is up
 *
 * beside this function wol filter also need to be set by
 * calling al_eth_filter_config with AL_ETH_RFW_FILTER_WOL
 *
 * @param adapter pointer to the private structure
 * @param wol the parameters needed to configure the wol
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_wol_enable(
		struct al_hw_eth_adapter *adapter,
		struct al_eth_wol_params *wol);

/*
 * Disable the WoL mechnism.
 *
 * @param adapter pointer to the private structure
 *
 * @return 0 on success. otherwise on failure.
 */
int al_eth_wol_disable(
		struct al_hw_eth_adapter *adapter);

enum AL_ETH_TX_GCP_ALU_OPSEL {
	AL_ETH_TX_GCP_ALU_L3_OFFSET			= 0,
	AL_ETH_TX_GCP_ALU_OUTER_L3_OFFSET		= 1,
	AL_ETH_TX_GCP_ALU_L3_LEN			= 2,
	AL_ETH_TX_GCP_ALU_OUTER_L3_LEN			= 3,
	AL_ETH_TX_GCP_ALU_L4_OFFSET			= 4,
	AL_ETH_TX_GCP_ALU_L4_LEN			= 5,
	AL_ETH_TX_GCP_ALU_TABLE_VAL			= 10
};

enum AL_ETH_RX_GCP_ALU_OPSEL {
	AL_ETH_RX_GCP_ALU_OUTER_L3_OFFSET		= 0,
	AL_ETH_RX_GCP_ALU_INNER_L3_OFFSET		= 1,
	AL_ETH_RX_GCP_ALU_OUTER_L4_OFFSET		= 2,
	AL_ETH_RX_GCP_ALU_INNER_L4_OFFSET		= 3,
	AL_ETH_RX_GCP_ALU_OUTER_L3_HDR_LEN_LAT		= 4,
	AL_ETH_RX_GCP_ALU_INNER_L3_HDR_LEN_LAT		= 5,
	AL_ETH_RX_GCP_ALU_OUTER_L3_HDR_LEN_SEL		= 6,
	AL_ETH_RX_GCP_ALU_INNER_L3_HDR_LEN_SEL		= 7,
	AL_ETH_RX_GCP_ALU_PARSE_RESULT_VECTOR_OFFSET_1	= 8,
	AL_ETH_RX_GCP_ALU_PARSE_RESULT_VECTOR_OFFSET_2	= 9,
	AL_ETH_RX_GCP_ALU_TABLE_VAL			= 10
};

enum AL_ETH_ALU_OPCODE {
	AL_ALU_FWD_A				= 0,
	AL_ALU_ARITHMETIC_ADD			= 1,
	AL_ALU_ARITHMETIC_SUBTRACT		= 2,
	AL_ALU_BITWISE_AND			= 3,
	AL_ALU_BITWISE_OR			= 4,
	AL_ALU_SHIFT_RIGHT_A_BY_B		= 5,
	AL_ALU_SHIFT_LEFT_A_BY_B		= 6,
	AL_ALU_BITWISE_XOR			= 7,
	AL_ALU_FWD_INV_A			= 16,
	AL_ALU_ARITHMETIC_ADD_INV_A_AND_B	= 17,
	AL_ALU_ARITHMETIC_SUBTRACT_INV_A_AND_B	= 18,
	AL_ALU_BITWISE_AND_INV_A_AND_B		= 19,
	AL_ALU_BITWISE_OR_INV_A_AND_B		= 20,
	AL_ALU_SHIFT_RIGHT_INV_A_BY_B		= 21,
	AL_ALU_SHIFT_LEFT_INV_A_BY_B		= 22,
	AL_ALU_BITWISE_XOR_INV_A_AND_B		= 23,
	AL_ALU_ARITHMETIC_ADD_A_AND_INV_B	= 33,
	AL_ALU_ARITHMETIC_SUBTRACT_A_AND_INV_B	= 34,
	AL_ALU_BITWISE_AND_A_AND_INV_B		= 35,
	AL_ALU_BITWISE_OR_A_AND_INV_B		= 36,
	AL_ALU_SHIFT_RIGHT_A_BY_INV_B		= 37,
	AL_ALU_SHIFT_LEFT_A_BY_INV_B		= 38,
	AL_ALU_BITWISE_XOR_A_AND_INV_B		= 39,
	AL_ALU_ARITHMETIC_ADD_INV_A_AND_INV_B	= 49,
	AL_ALU_ARITHMETIC_SUBTRACT_INV_A_AND	= 50,
	AL_ALU_BITWISE_AND_INV_A_AND_INV_B	= 51,
	AL_ALU_BITWISE_OR_INV_A_AND_INV_B	= 52,
	AL_ALU_SHIFT_RIGHT_INV_A_BY_INV_B	= 53,
	AL_ALU_SHIFT_LEFT_INV_A_BY_INV_B	= 54,
	AL_ALU_BITWISE_XOR_INV_A_AND_INV_B	= 55,
};

#endif		/* __AL_HW_ETH_H__ */
