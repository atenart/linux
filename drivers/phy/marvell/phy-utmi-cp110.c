/*
 * phy-utmi-cp110.c for cp110-utmi
 * Created by <nschichan@freebox.fr> on Mon Jul 24 18:09:45 2017
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>

#include <linux/phy/phy-utmi-cp110.h>

#include "phy-utmi-cp110.h"

#define UTMI_PHY_COUNT	2

struct utmi_cp110_priv;

struct utmi_cp110_phy {
	struct phy *phy;
	struct utmi_cp110_priv *priv;
	u32 mux;
	int index;
};

struct utmi_cp110_priv {
	void __iomem *utmi_unit_regs;
	struct regmap *regmap;

	struct device *dev;
	struct utmi_cp110_phy phys[2];
};

static const struct of_device_id mvebu_comphy_of_match[] = {
	{ .compatible = "marvell,cp110-utmi-phy", },
	{ },
};
MODULE_DEVICE_TABLE(of, mvebu_comphy_of_match);

static void __iomem *map_resource(struct platform_device *pdev,
				const char *resource_name)
{
	struct resource *res;
	void *ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, resource_name);
	if (!res) {
		dev_err(&pdev->dev, "unable to get resource %s\n",
			resource_name);
		return NULL;
	}

	ret = devm_ioremap_resource(&pdev->dev, res);
	if (!ret) {
		dev_err(&pdev->dev, "unable to remap resource %pR: (%s)\n",
			res, resource_name);
	}
	dev_dbg(&pdev->dev, "ioremaped resource %pR: (%s)\n",
		res, resource_name);
	return ret;
}

/*
 * relative to priv->regmap register space.
 */
#define UPHY_CFG_REG_OFF(n)		(0x440 + (n) * 4)
#define USB_CONFIG_REG			(0x420)

/*
 * read/write from/to utmi configuration registers.
 */
static u32 uphy_config_read(struct utmi_cp110_phy *uphy)
{
	u32 ret;

	regmap_read(uphy->priv->regmap, UPHY_CFG_REG_OFF(uphy->index), &ret);
	return ret;
}

static void uphy_config_write(u32 value, struct utmi_cp110_phy *uphy)
{
	regmap_write(uphy->priv->regmap, UPHY_CFG_REG_OFF(uphy->index), value);
}

/*
 * read/write from/to usb configuration register
 */
static u32 usb_config_read(struct utmi_cp110_priv *priv)
{
	u32 ret;

	regmap_read(priv->regmap, USB_CONFIG_REG, &ret);
	return ret;
}

static void usb_config_write(u32 value, struct utmi_cp110_priv *priv)
{
	regmap_write(priv->regmap, USB_CONFIG_REG, value);
}

/*
 * read/write from/to utmi PHY unit registers.
 */
static u32 uphy_unit_read(struct utmi_cp110_phy *uphy, u32 off)
{
	u32 ret = readl(uphy->priv->utmi_unit_regs + off +
			0x1000 * uphy->index);

	return ret;
}

static void uphy_unit_write(u32 value, struct utmi_cp110_phy *uphy, u32 off)
{
	writel(value, uphy->priv->utmi_unit_regs + off + 0x1000 * uphy->index);
}

static void utmi_power_down(struct utmi_cp110_phy *uphy)
{
	u32 reg;

	reg = uphy_config_read(uphy);
	reg &= ~UTMI_PHY_CFG_PU_MASK;
	uphy_config_write(reg, uphy);

	reg = usb_config_read(uphy->priv);
	if (uphy->mux == UTMI_PHY_USB2_DEVICE) {
		/*
		 * if in USB device mode, configure mux while the phy
		 * is powered down.
		 */
		reg |= UTMI_USB_CFG_DEVICE_EN_MASK;
		reg &= ~UTMI_USB_CFG_DEVICE_MUX_MASK;
		reg |= (uphy->index << UTMI_USB_CFG_DEVICE_MUX_OFFSET);
	} else {
		/*
		 * otherwise disable USB3 device mode.
		 */
		reg &= ~UTMI_USB_CFG_DEVICE_EN_MASK;
	}
	usb_config_write(reg, uphy->priv);

	/*
	 * set suspend mode and test_sel
	 */
	reg = uphy_unit_read(uphy, UTMI_CTRL_STATUS0_REG);
	reg |= UTMI_CTRL_STATUS0_SUSPENDM_MASK;
	reg |= UTMI_CTRL_STATUS0_TEST_SEL_MASK;
	uphy_unit_write(reg, uphy, UTMI_CTRL_STATUS0_REG);

	mdelay(2);
}

static void utmi_pll_power_off(struct utmi_cp110_phy *uphy)
{
	u32 reg;

	/*
	 * FIXME: this is a global bit for both UTMI phys, is it safe
	 * to power it down when the other phy is powered on ?
	 */
	reg = usb_config_read(uphy->priv);
	reg &= ~UTMI_USB_CFG_PLL_MASK;
	usb_config_write(reg, uphy->priv);
}

static void utmi_pll_power_on(struct utmi_cp110_phy *uphy)
{
	u32 reg;

	/*
	 * FIXME: see above.
	 */
	reg = usb_config_read(uphy->priv);
	reg |= UTMI_USB_CFG_PLL_MASK;
	usb_config_write(reg, uphy->priv);
}

static void utmi_phy_configure(struct utmi_cp110_phy *uphy)
{
	u32 reg;

	reg = uphy_unit_read(uphy, UTMI_PLL_CTRL_REG);
	/* Reference Clock Divider Select */
	reg &= ~UTMI_PLL_CTRL_REFDIV_MASK;
	reg |= 0x5 << UTMI_PLL_CTRL_REFDIV_OFFSET;
	/* Feedback Clock Divider Select - 90 for 25Mhz*/
	reg &= ~UTMI_PLL_CTRL_FBDIV_MASK;
	reg |= 0x60 << UTMI_PLL_CTRL_FBDIV_OFFSET;
	/* Select LPFR - 0x0 for 25Mhz/5=5Mhz*/
	reg &= ~UTMI_PLL_CTRL_SEL_LPFR_MASK;
	reg |= 0x0 << UTMI_PLL_CTRL_SEL_LPFR_OFFSET;
	uphy_unit_write(reg, uphy, UTMI_PLL_CTRL_REG);

	/* Impedance Calibration Threshold Setting */
	reg = uphy_unit_read(uphy, UTMI_CALIB_CTRL_REG);
	reg &= ~UTMI_CALIB_CTRL_IMPCAL_VTH_MASK;
	reg |= 0x6 << UTMI_CALIB_CTRL_IMPCAL_VTH_OFFSET;
	uphy_unit_write(reg, uphy, UTMI_CALIB_CTRL_REG);

	reg = uphy_unit_read(uphy, UTMI_TX_CH_CTRL_REG);
	/* Set LS TX driver strength coarse control */
	reg &= ~UTMI_TX_CH_CTRL_DRV_EN_LS_MASK;
	reg |= 0x3 << UTMI_TX_CH_CTRL_DRV_EN_LS_OFFSET;
	/* Set LS TX driver fine adjustment */
	reg &= ~UTMI_TX_CH_CTRL_IMP_SEL_LS_MASK;
	reg |= 0x3 << UTMI_TX_CH_CTRL_IMP_SEL_LS_OFFSET;
	uphy_unit_write(reg, uphy, UTMI_TX_CH_CTRL_REG);

	reg = uphy_unit_read(uphy, UTMI_RX_CH_CTRL0_REG);
	/* Enable SQ */
	reg &= ~UTMI_RX_CH_CTRL0_SQ_DET_MASK;
	reg |= 0x0 << UTMI_RX_CH_CTRL0_SQ_DET_OFFSET;
	/* Enable analog squelch detect */
	reg &= ~UTMI_RX_CH_CTRL0_SQ_ANA_DTC_MASK;
	reg |= 0x1 << UTMI_RX_CH_CTRL0_SQ_ANA_DTC_OFFSET;
	uphy_unit_write(reg, uphy, UTMI_RX_CH_CTRL0_REG);

	/* Set External squelch calibration number */
	reg = uphy_unit_read(uphy, UTMI_RX_CH_CTRL1_REG);
	reg &= ~UTMI_RX_CH_CTRL1_SQ_AMP_CAL_MASK;
	reg |= 0x1 << UTMI_RX_CH_CTRL1_SQ_AMP_CAL_OFFSET;
	/* Enable the External squelch calibration */
	reg &= ~UTMI_RX_CH_CTRL1_SQ_AMP_CAL_EN_MASK;
	reg |= 0x1 << UTMI_RX_CH_CTRL1_SQ_AMP_CAL_EN_OFFSET;
	uphy_unit_write(reg, uphy, UTMI_RX_CH_CTRL1_REG);

	/* Set Control VDAT Reference Voltage - 0.325V */
	reg = uphy_unit_read(uphy, UTMI_CHGDTC_CTRL_REG);
	reg &= ~UTMI_CHGDTC_CTRL_VDAT_MASK;
	reg |= 0x1 << UTMI_CHGDTC_CTRL_VDAT_OFFSET;
	/* Set Control VSRC Reference Voltage - 0.6V */
	reg &= ~UTMI_CHGDTC_CTRL_VSRC_MASK;
	reg |= 0x1 << UTMI_CHGDTC_CTRL_VSRC_OFFSET;
	uphy_unit_write(reg, uphy, UTMI_CHGDTC_CTRL_REG);
}

static int utmi_power_up(struct utmi_cp110_phy *uphy)
{
	u32 reg;
	u32 tries;

	/*
	 * set PU bit
	 */
	reg = uphy_config_read(uphy);
	reg |= UTMI_PHY_CFG_PU_MASK;
	uphy_config_write(reg, uphy);

	/*
	 * clear  test_sel
	 */
	reg = uphy_unit_read(uphy, UTMI_CTRL_STATUS0_REG);
	reg &= ~UTMI_CTRL_STATUS0_TEST_SEL_MASK;
	uphy_unit_write(reg, uphy, UTMI_CTRL_STATUS0_REG);

	/*
	 * wait for impedance calibration
	 */
	tries = 100;
	while (tries) {
		u32 reg = uphy_unit_read(uphy, UTMI_CALIB_CTRL_REG);

		if ((reg & (UTMI_CALIB_CTRL_IMPCAL_DONE_MASK |
			   UTMI_CALIB_CTRL_PLLCAL_DONE_MASK)) ==
		    (UTMI_CALIB_CTRL_IMPCAL_DONE_MASK |
		     UTMI_CALIB_CTRL_PLLCAL_DONE_MASK))
			break;
		mdelay(2);
		--tries;
	}
	if (!tries)
		return -ETIMEDOUT;

	/*
	 * wait for PLL ready.
	 */
	tries = 100;
	while (tries) {
		u32 reg = uphy_unit_read(uphy, UTMI_PLL_CTRL_REG);

		if (reg & UTMI_PLL_CTRL_PLL_RDY_MASK)
			break;
		mdelay(2);
		--tries;
	}
	if (!tries)
		return -ETIMEDOUT;

	return 0;
}

static int utmi_cp110_power_off(struct phy *phy)
{
	struct utmi_cp110_phy *uphy = phy_get_drvdata(phy);

	utmi_power_down(uphy);
	return 0;
}

static int utmi_cp110_power_on(struct phy *phy)
{
	struct utmi_cp110_phy *uphy = phy_get_drvdata(phy);
	int err;

	utmi_power_down(uphy);
	utmi_pll_power_off(uphy);
	utmi_phy_configure(uphy);
	err = utmi_power_up(uphy);
	utmi_pll_power_on(uphy);

	return err;
}

static int utmi_cp110_set_mode(struct phy *phy, enum phy_mode mode)
{
	struct utmi_cp110_phy *uphy = phy_get_drvdata(phy);

	pr_info("utmi_cp110_set_mode(): mode %d\n", mode);
	if (mode >= (int)UTMI_PHY_MAX)
		return -EINVAL;
	uphy->mux = mode;
	return 0;
}

static const struct phy_ops utmi_cp110_phy_ops = {
	.power_on = utmi_cp110_power_on,
	.power_off = utmi_cp110_power_off,
	.set_mode = utmi_cp110_set_mode,
	.owner = THIS_MODULE,
};

static struct phy *utmi_cp110_phy_of_xlate(struct device *dev,
					 struct of_phandle_args *args)
{
	struct utmi_cp110_priv *priv = dev_get_drvdata(dev);
	u32 index = args->args[0];
	struct utmi_cp110_phy *uphy;

	if (index >= UTMI_PHY_COUNT)
		return NULL;

	uphy = &priv->phys[index];

	return uphy->phy;
}

static int utmi_cp110_probe(struct platform_device *pdev)
{
	struct utmi_cp110_priv *priv;
	struct phy_provider *provider;
	int i;

	priv = devm_kzalloc(&pdev->dev, sizeof (*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;
	priv->regmap =
		syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						"marvell,system-controller");
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	priv->utmi_unit_regs = map_resource(pdev, "utmi-unit");
	if (!priv->utmi_unit_regs)
		return -ENOMEM;

	for (i = 0; i < UTMI_PHY_COUNT; ++i) {
		dev_dbg(&pdev->dev, "creating phy %d\n", i);
		priv->phys[i].phy = devm_phy_create(&pdev->dev, NULL,
						    &utmi_cp110_phy_ops);
		if (IS_ERR(priv->phys[i].phy)) {
			int err = PTR_ERR(priv->phys[i].phy);
			dev_err(&pdev->dev, "devm_phy_create: failed with %d\n", err);
			return err;
		}
		priv->phys[i].priv = priv;
		priv->phys[i].index = i;
		phy_set_drvdata(priv->phys[i].phy, &priv->phys[i]);
	}
	platform_set_drvdata(pdev, priv);

	provider = devm_of_phy_provider_register(&pdev->dev,
						 utmi_cp110_phy_of_xlate);
	if (!provider)
		dev_err(&pdev->dev, "failed to register OF phy provider.\n");
	return PTR_ERR_OR_ZERO(provider);
}

static struct platform_driver utmi_cp110_driver = {
	.probe	= utmi_cp110_probe,
	.driver	= {
		.name		= "phy-utmi-cp110",
		.owner		= THIS_MODULE,
		.of_match_table	= mvebu_comphy_of_match,
	 },
};
module_platform_driver(utmi_cp110_driver);
