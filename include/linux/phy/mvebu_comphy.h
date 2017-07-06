/*
 * Copyright (C) 2017 Marvell
 *
 * Antoine Tenart <antoine.tenart@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DT_BINDINGS_MVEBU_COMPHY
#define _DT_BINDINGS_MVEBU_COMPHY

enum mvebu_comphy_mode {
	COMPHY_UNUSED,
	COMPHY_SATA_P0,
	COMPHY_SATA_P1,
	COMPHY_SGMII_P0,
	COMPHY_SGMII_P1,
	COMPHY_SGMII_P2,
	COMPHY_HS_SGMII_P0,
	COMPHY_HS_SGMII_P1,
	COMPHY_HS_SGMII_P2,
	COMPHY_RXAUI_L0,
	COMPHY_RXAUI_L1,
	COMPHY_10GKR,
	/* placeholder */
	COMPHY_MAX,
};

#endif /* _DT_BINDINGS_MVEBU_COMPHY */
