/*
 * walletsvc.h
 *
 *  Created on: 13. 12. 2021
 *      Author: ondra
 */

#ifndef SRC_MAIN_WALLETSVC_H_
#define SRC_MAIN_WALLETSVC_H_
#include <couchit/changes.h>
#include <couchit/couchDB.h>
#include <shared/scheduler.h>

#include "wallet.h"

namespace lnex {

class WalletService {
public:
	WalletService(couchit::CouchDB &db,const WalletConfig &cfg, userver::HttpClientCfg &&httpc);

	static void init(std::shared_ptr<WalletService> me, couchit::ChangesDistributor &chdist, ondra_shared::Scheduler sch);

protected:
	couchit::CouchDB &db;
	WalletControl wallet;

	void checkWalletState();
	void processWalletRequest(const couchit::ChangeEvent &ev);
	bool onInvoiceChange(json::String id, bool paid);
};

} /* namespace lnex */

#endif /* SRC_MAIN_WALLETSVC_H_ */
