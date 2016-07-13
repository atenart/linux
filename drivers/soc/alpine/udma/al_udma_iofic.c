/*
 * Annapurna Labs UDMA-specific IOFIC helpers
 *
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/soc/alpine/al_hw_udma_iofic.h>
#include <linux/soc/alpine/al_hw_udma_regs.h>

/* configure the interrupt registers, interrupts will are kept masked */
static int al_udma_main_iofic_config(void __iomem *iofic,
				     enum al_iofic_mode mode)
{
	switch (mode) {
	case AL_IOFIC_MODE_LEGACY:
		al_iofic_config(iofic, AL_INT_GROUP_A,
				INT_CONTROL_GRP_SET_ON_POSEDGE |
				INT_CONTROL_GRP_MASK_MSI_X |
				INT_CONTROL_GRP_CLEAR_ON_READ);
		al_iofic_config(iofic, AL_INT_GROUP_B,
				INT_CONTROL_GRP_CLEAR_ON_READ |
				INT_CONTROL_GRP_MASK_MSI_X);
		al_iofic_config(iofic, AL_INT_GROUP_C,
				INT_CONTROL_GRP_CLEAR_ON_READ |
				INT_CONTROL_GRP_MASK_MSI_X);
		al_iofic_config(iofic, AL_INT_GROUP_D,
				INT_CONTROL_GRP_SET_ON_POSEDGE |
				INT_CONTROL_GRP_MASK_MSI_X |
				INT_CONTROL_GRP_CLEAR_ON_READ);
		break;
	case AL_IOFIC_MODE_MSIX_PER_Q:
		al_iofic_config(iofic, AL_INT_GROUP_A,
				INT_CONTROL_GRP_SET_ON_POSEDGE |
				INT_CONTROL_GRP_AUTO_MASK |
				INT_CONTROL_GRP_AUTO_CLEAR);
		al_iofic_config(iofic, AL_INT_GROUP_B,
				INT_CONTROL_GRP_AUTO_CLEAR |
				INT_CONTROL_GRP_AUTO_MASK |
				INT_CONTROL_GRP_CLEAR_ON_READ);
		al_iofic_config(iofic, AL_INT_GROUP_C,
				INT_CONTROL_GRP_AUTO_CLEAR |
				INT_CONTROL_GRP_AUTO_MASK |
				INT_CONTROL_GRP_CLEAR_ON_READ);
		al_iofic_config(iofic, AL_INT_GROUP_D,
				INT_CONTROL_GRP_SET_ON_POSEDGE |
				INT_CONTROL_GRP_CLEAR_ON_READ |
				INT_CONTROL_GRP_MASK_MSI_X);
		break;
	case AL_IOFIC_MODE_MSIX_PER_GROUP:
		al_iofic_config(iofic, AL_INT_GROUP_A,
				INT_CONTROL_GRP_SET_ON_POSEDGE |
				INT_CONTROL_GRP_AUTO_CLEAR |
				INT_CONTROL_GRP_AUTO_MASK);
		al_iofic_config(iofic, AL_INT_GROUP_B,
				INT_CONTROL_GRP_CLEAR_ON_READ |
				INT_CONTROL_GRP_MASK_MSI_X);
		al_iofic_config(iofic, AL_INT_GROUP_C,
				INT_CONTROL_GRP_CLEAR_ON_READ |
				INT_CONTROL_GRP_MASK_MSI_X);
		al_iofic_config(iofic, AL_INT_GROUP_D,
				INT_CONTROL_GRP_SET_ON_POSEDGE |
				INT_CONTROL_GRP_CLEAR_ON_READ |
				INT_CONTROL_GRP_MASK_MSI_X);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* configure the UDMA interrupt registers, interrupts are kept masked */
int al_udma_iofic_config(void __iomem *regs,
			 enum al_iofic_mode mode, u32 m2s_errors_disable,
			 u32 m2s_aborts_disable, u32 s2m_errors_disable,
			 u32 s2m_aborts_disable)
{
	int rc;

	rc = al_udma_main_iofic_config(regs + UDMA_GEN_IRQ_MAIN_IOFIC, mode);
	if (rc)
		return rc;

	al_iofic_unmask(regs + UDMA_GEN_IRQ_SECONDARY_IOFIC,
			AL_INT_GROUP_A, ~m2s_errors_disable);
	al_iofic_abort_mask(regs + UDMA_GEN_IRQ_SECONDARY_IOFIC,
			    AL_INT_GROUP_A, m2s_aborts_disable);

	al_iofic_unmask(regs + UDMA_GEN_IRQ_SECONDARY_IOFIC,
			AL_INT_GROUP_B, ~s2m_errors_disable);
	al_iofic_abort_mask(regs + UDMA_GEN_IRQ_SECONDARY_IOFIC,
			    AL_INT_GROUP_B, s2m_aborts_disable);

	return 0;
}

/* returns the offset of the unmask register for a given group */
u32 __iomem *al_udma_iofic_unmask_offset_get(void __iomem *regs,
					     enum al_udma_iofic_level level,
					     int group)
{
	WARN_ON(!al_udma_iofic_level_and_group_valid(level, group));
	return al_iofic_unmask_offset_get(
			al_udma_iofic_reg_base_get(regs, level), group);
}
