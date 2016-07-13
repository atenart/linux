/*
 * Annapurna Labs IOFIC helpers
 *
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __AL_IOFIC_H__
#define __AL_IOFIC_H__

#include <linux/io.h>
#include <linux/slab.h>

#define CTRL_GROUP(group)		((group) * 0x40)
#define GROUP_INT_MODE(group, vector)	(0x400 + (group) * 0x20 + (vector) * 0x8)

#define INT_CAUSE_GROUP		0x0
#define INT_MASK_GROUP		0x10
#define INT_MASK_CLEAR_GROUP	0x18
#define INT_CONTROL_GROUP	0x28
#define INT_ABORT_MASK_GROUP	0x30

#define INT_CONTROL_GRP_CLEAR_ON_READ	BIT(0)
#define INT_CONTROL_GRP_AUTO_MASK	BIT(1)
#define INT_CONTROL_GRP_AUTO_CLEAR	BIT(2)
#define INT_CONTROL_GRP_SET_ON_POSEDGE	BIT(3)
#define INT_CONTROL_GRP_MASK_MSI_X	BIT(5)
/* MSI-X AWID value, same ID for all cause bits */
#define INT_CONTROL_GRP_MOD_RES_MASK	0xf000000
#define INT_CONTROL_GRP_MOD_RES_SHIFT	0x18

#define INT_MOD_INTV_MASK		0x000000ff
#define INT_MOD_INTV_SHIFT		0x0


/*
 * Configure the interrupt controller registers, actual interrupts are still
 * masked at this stage.
 *
 * @param regs_base regs pointer to interrupt controller registers
 * @param group the interrupt group.
 * @param flags flags of Interrupt Control Register
 */

static inline void al_iofic_config(void __iomem *base, int group, u32 flags)
{
	writel(flags, base + CTRL_GROUP(group) + INT_CONTROL_GROUP);
}

/*
 * Configure the moderation timer resolution for a given group.
 *
 * Applies for both msix and legacy mode.
 *
 * @param regs_base pointer to unit registers
 * @param group the interrupt group
 * @param resolution resolution of the timer interval, the resolution determines
 * the rate of decrementing the interval timer, setting value N means that the
 * interval timer will be decremented each (N+1) * (0.68) micro seconds.
 */
static inline void al_iofic_moder_res_config(void __iomem *base, int group,
					     u8 resolution)
{
	u32 reg = readl(base + CTRL_GROUP(group) + INT_CONTROL_GROUP);

	reg &= ~INT_CONTROL_GRP_MOD_RES_MASK;
	reg |= resolution << INT_CONTROL_GRP_MOD_RES_SHIFT;

	writel(reg, base + CTRL_GROUP(group) + INT_CONTROL_GROUP);
}

/*
 * Configure the moderation timer interval for a given msix vector.
 *
 * @param regs_base pointer to unit registers
 * @param group the interrupt group
 * @param vector vector index
 * @param interval interval between interrupts, 0 disable
 */
static inline void al_iofic_msix_moder_interval_config(void __iomem *base,
						       int group, u8 vector,
						       u8 interval)
{
	u32 reg = readl(base + GROUP_INT_MODE(group, vector));

	reg &= ~INT_MOD_INTV_MASK;
	reg |= interval << INT_MOD_INTV_SHIFT;

	writel(reg, base + GROUP_INT_MODE(group, vector));
}

/*
 * Unmask specific interrupts for a given group.
 *
 * This functions guarantees atomic operations, it is performance optimized as
 * it will not require read-modify-write. The unmask done using the interrupt
 * mask clear register, so it's safe to call it while the mask is changed by
 * the HW (auto mask) or another core.
 *
 * @param regs_base pointer to unit registers
 * @param group the interrupt group
 * @param mask bitwise of interrupts to unmask, set bits will be unmasked.
 */
static inline void al_iofic_unmask(void __iomem *base, int group, u32 mask)
{
	/*
	 * use the mask clear register, no need to read the mask register
	 * itself. write 0 to unmask, 1 has no effect
	 */
	writel(~mask, base + CTRL_GROUP(group) + INT_MASK_CLEAR_GROUP);
}

/*
 * Mask specific interrupts for a given group.
 *
 * This functions modifies interrupt mask register, the callee must make sure
 * the mask is not changed by another cpu.
 *
 * @param regs_base pointer to unit registers
 * @param group the interrupt group
 * @param mask bitwise of interrupts to mask, set bits will be masked.
 */
static inline void al_iofic_mask(void __iomem *base, int group, u32 mask)
{
	u32 reg = readl(base + CTRL_GROUP(group) + INT_MASK_GROUP);
	reg |= mask;
	writel(reg, base + CTRL_GROUP(group) + INT_MASK_GROUP);
}

/*
 * Read interrupt cause register for a given group.
 *
 * This will clear the set bits if the clear on read mode is enabled.
 *
 * @param regs_base pointer to unit registers
 * @param group the interrupt group
 */
static inline u32 al_iofic_read_cause(void __iomem *base, int group)
{
	return readl(base + CTRL_GROUP(group) + INT_CAUSE_GROUP);
}

/*
 * Unmask specific interrupts from aborting the udma a given group.
 *
 * @param regs_base pointer to unit registers
 * @param group the interrupt group
 * @param mask bitwise of interrupts to mask
 */
static inline void al_iofic_abort_mask(void __iomem *base, int group, u32 mask)
{
	writel(mask, base + CTRL_GROUP(group) + INT_ABORT_MASK_GROUP);
}

/* return the offset of the unmask register for a given group */
static inline u32 __iomem *al_iofic_unmask_offset_get(void __iomem *base,
						      int group)
{
	return base + CTRL_GROUP(group) + INT_MASK_CLEAR_GROUP;
}

#endif
