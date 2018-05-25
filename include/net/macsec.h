// SPDX-License-Identifier: (GPL-2.0 OR MIT)
#ifndef _NET_MACSEC_H_
#define _NET_MACSEC_H_

enum netdev_macsec_commands {
	MACSEC_ADD_RXSC,
	MACSEC_DEL_RXSC,
	MACSEC_UPD_RXSC,
	MACSEC_ADD_TXSC,
	MACSEC_DEL_TXSC,
	MACSEC_UPD_TXSC,
	MACSEC_ADD_RXSA,
	MACSEC_DEL_RXSA,
	MACSEC_UPD_RXSA,
	MACSEC_ADD_TXSA,
	MACSEC_DEL_TXSA,
	MACSEC_UPD_TXSA,
};

struct netdev_macsec {
	enum netdev_macsec_commands command;
	union {
		struct macsec_rx_sc *rx_sc;
		struct macsec_tx_sc *tx_sc;
		struct macsec_rx_sa *rx_sa;
		struct macsec_tx_sa *tx_sa;
	};
};

#endif
