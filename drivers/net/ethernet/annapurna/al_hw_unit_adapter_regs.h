/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __AL_HW_UNIT_ADAPTER_REGS_H__
#define __AL_HW_UNIT_ADAPTER_REGS_H__

#define AL_PCI_COMMAND		0x04	/* 16 bits */

#define AL_PCI_EXP_CAP_BASE		0x40
#define AL_PCI_EXP_DEVCTL		8       /* Device Control */
#define  AL_PCI_EXP_DEVCTL_BCR_FLR	0x8000  /* Bridge Configuration Retry / FLR */

#define AL_ADAPTER_GENERIC_CONTROL_0		0x1E0

/* When set, all transactions through the PCI conf & mem BARs get timeout */
#define AL_ADAPTER_GENERIC_CONTROL_0_ETH_RESET_1GMAC		BIT(18)
#define AL_ADAPTER_GENERIC_CONTROL_0_ETH_RESET_1GMAC_ON_FLR	BIT(26)

#endif
