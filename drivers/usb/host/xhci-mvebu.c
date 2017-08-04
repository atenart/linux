/*
 * Copyright (C) 2014 Marvell
 * Author: Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/mbus.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-utmi-cp110.h>

#include <linux/usb.h>
#include <linux/usb/hcd.h>

#include "xhci-plat.h"
#include "xhci-mvebu.h"

#define USB3_MAX_WINDOWS	4
#define USB3_WIN_CTRL(w)	(0x0 + ((w) * 8))
#define USB3_WIN_BASE(w)	(0x4 + ((w) * 8))

static void xhci_mvebu_mbus_config(void __iomem *base,
			const struct mbus_dram_target_info *dram)
{
	int win;

	/* Clear all existing windows */
	for (win = 0; win < USB3_MAX_WINDOWS; win++) {
		writel(0, base + USB3_WIN_CTRL(win));
		writel(0, base + USB3_WIN_BASE(win));
	}

	/* Program each DRAM CS in a seperate window */
	for (win = 0; win < dram->num_cs; win++) {
		const struct mbus_dram_window *cs = dram->cs + win;

		writel(((cs->size - 1) & 0xffff0000) | (cs->mbus_attr << 8) |
		       (dram->mbus_dram_target_id << 4) | 1,
		       base + USB3_WIN_CTRL(win));

		writel((cs->base & 0xffff0000), base + USB3_WIN_BASE(win));
	}
}

int xhci_mvebu_mbus_init_quirk(struct usb_hcd *hcd)
{
	struct device *dev = hcd->self.controller;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource	*res;
	void __iomem *base;
	const struct mbus_dram_target_info *dram;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res)
		return -ENODEV;

	/*
	 * We don't use devm_ioremap() because this mapping should
	 * only exists for the duration of this probe function.
	 */
	base = ioremap(res->start, resource_size(res));
	if (!base)
		return -ENODEV;

	dram = mv_mbus_dram_info();
	xhci_mvebu_mbus_config(base, dram);

	/*
	 * This memory area was only needed to configure the MBus
	 * windows, and is therefore no longer useful.
	 */
	iounmap(base);

	return 0;
}

static int xhci_mvebu_configure_phy(struct xhci_plat_priv *priv,
				    struct device *dev, const char *phy_name,
				    int phy_mode)
{
	struct phy *phy;
	int error;

	phy = devm_of_phy_get(dev, dev->of_node, phy_name);
	if (IS_ERR(phy))
		return PTR_ERR(phy);

	error = phy_set_mode(phy, phy_mode);
	if (error) {
		dev_err(dev, "unable to set mode of '%s' phy: %d\n", phy_name,
			error);
		return error;
	}

	error = phy_power_on(phy);
	if (error) {
		dev_err(dev, "unable to power on '%s' phy: %d\n", phy_name,
			error);
		return error;
	}
	return 0;
}

/*
 * on A8K only, configure COMPHY (USB3) and UTMI (USB2) phys.
 */
int xhci_mvebu_init_phys(struct usb_hcd *hcd)
{
	struct xhci_plat_priv *priv = hcd_to_xhci_priv(hcd);
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);;

	if (hcd == xhci->shared_hcd)
		xhci_mvebu_configure_phy(priv, hcd->self.controller,
					 "usb3", PHY_MODE_USB_HOST);
	else
		xhci_mvebu_configure_phy(priv, hcd->self.controller,
					 "usb2", UTMI_PHY_USB2_HOST);

	return 0;
}
