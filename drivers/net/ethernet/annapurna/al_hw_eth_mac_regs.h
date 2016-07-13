/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __AL_HW_ETH_MAC_REGS_H__
#define __AL_HW_ETH_MAC_REGS_H__

/* cmd_cfg */
#define ETH_1G_MAC_CMD_CFG_TX_ENA	BIT(0)
#define ETH_1G_MAC_CMD_CFG_RX_ENA	BIT(1)
/* enable Half Duplex */
#define ETH_1G_MAC_CMD_CFG_HD_EN	BIT(10)
/* enable 1G speed */
#define ETH_1G_MAC_CMD_CFG_1G_SPD	BIT(3)
/* enable 10M speed */
#define ETH_1G_MAC_CMD_CFG_10M_SPD	BIT(25)

/* cmd_cfg */
#define ETH_10G_MAC_CMD_CFG_TX_ENA				BIT(0)
#define ETH_10G_MAC_CMD_CFG_RX_ENA				BIT(1)

/* mdio_cfg_status */
#define ETH_10G_MAC_MDIO_CFG_HOLD_TIME_MASK	0x0000001c
#define ETH_10G_MAC_MDIO_CFG_HOLD_TIME_SHIFT	2

#define ETH_10G_MAC_MDIO_CFG_HOLD_TIME_7_CLK	3

/* control */
#define ETH_10G_MAC_CONTROL_AN_EN_MASK	0x00001000

/* if_mode */
#define ETH_10G_MAC_IF_MODE_SGMII_EN_MASK	0x00000001
#define ETH_10G_MAC_IF_MODE_SGMII_AN_MASK	0x00000002
#define ETH_10G_MAC_IF_MODE_SGMII_SPEED_MASK	0x0000000c
#define ETH_10G_MAC_IF_MODE_SGMII_SPEED_SHIFT	2
#define ETH_10G_MAC_IF_MODE_SGMII_DUPLEX_MASK	0x00000010
#define ETH_10G_MAC_IF_MODE_SGMII_DUPLEX_SHIFT	4

#define ETH_10G_MAC_IF_MODE_SGMII_SPEED_10M	0
#define ETH_10G_MAC_IF_MODE_SGMII_SPEED_100M	1
#define ETH_10G_MAC_IF_MODE_SGMII_SPEED_1G	2

#define ETH_10G_MAC_IF_MODE_SGMII_DUPLEX_FULL	0
#define ETH_10G_MAC_IF_MODE_SGMII_DUPLEX_HALF	1

/*
 * Selection of the input for the "set_1000" input of the RGMII converter
 * 0 - From MAC
 * 1 - From register set_1000_def (automatic speed selection)
 */
#define ETH_MAC_GEN_RGMII_CFG_SET_1000_SEL BIT(0)
/*
 * Selection of the input for the "set_10" input of the RGMII converter:
 * 0 - From MAC
 * 1 - From register set_10_def (automatic speed selection)
 */
#define ETH_MAC_GEN_RGMII_CFG_SET_10_SEL BIT(4)
/* Enable automatic speed selection (based on PHY in-band status information) */
#define ETH_MAC_GEN_RGMII_CFG_ENA_AUTO   BIT(8)

#define ETH_MAC_GEN_MUX_SEL_KR_IN_MASK   0x0000C000

/*
 * LED source selection:
 * 0 – Default reg
 * 1 – Rx activity
 * 2 – Tx activity
 * 3 – Rx | Tx activity
 * 4-9 – SGMII LEDs
 */
#define ETH_MAC_GEN_LED_CFG_SEL_MASK     0x0000000F

/* turn the led on/off based on default value field (ETH_MAC_GEN_LED_CFG_DEF) */
#define ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG	0

/* LED default value */
#define ETH_MAC_GEN_LED_CFG_DEF          BIT(4)

#define ETH_MAC_SGMII_REG_ADDR_CTRL_REG	0x0
#define ETH_MAC_SGMII_REG_ADDR_IF_MODE_REG 0x14

#define ETH_MAC_SGMII_REG_DATA_CTRL_AN_ENABLE			BIT(12)
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_EN			BIT(0)
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_AN			BIT(1)
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_10		0x0
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_100		0x1
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_1000		0x2
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_DUPLEX		BIT(4)

/* command config */
#define ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_ADDR	0x00000008
#define ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_TX_ENA	BIT(0)
#define ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_RX_ENA	BIT(1)
#define ETH_MAC_GEN_V3_MAC_40G_COMMAND_CONFIG_PFC_MODE	BIT(19)

/* frame length */
#define ETH_MAC_GEN_V3_MAC_40G_FRM_LENGTH_ADDR		0x00000014

#define ETH_MAC_GEN_V3_MAC_40G_CL01_PAUSE_QUANTA_ADDR	0x00000054
#define ETH_MAC_GEN_V3_MAC_40G_CL23_PAUSE_QUANTA_ADDR	0x00000058
#define ETH_MAC_GEN_V3_MAC_40G_CL45_PAUSE_QUANTA_ADDR	0x0000005C
#define ETH_MAC_GEN_V3_MAC_40G_CL67_PAUSE_QUANTA_ADDR	0x00000060
#define ETH_MAC_GEN_V3_MAC_40G_CL01_QUANTA_THRESH_ADDR	0x00000064
#define ETH_MAC_GEN_V3_MAC_40G_CL23_QUANTA_THRESH_ADDR	0x00000068
#define ETH_MAC_GEN_V3_MAC_40G_CL45_QUANTA_THRESH_ADDR	0x0000006C
#define ETH_MAC_GEN_V3_MAC_40G_CL67_QUANTA_THRESH_ADDR	0x00000070

/* spare */
#define ETH_MAC_GEN_V3_SPARE_CHICKEN_DISABLE_TIMESTAMP_STRETCH BIT(0)

#endif /* __AL_HW_ETH_MAC_REGS_H__ */
