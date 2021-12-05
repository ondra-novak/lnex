/*
 * exchange.h
 *
 *  Created on: 4. 12. 2021
 *      Author: ondra
 */

#ifndef SRC_MAIN_EXCHANGE_H_
#define SRC_MAIN_EXCHANGE_H_
#include <couchit/changes.h>
#include <couchit/couchDB.h>
#include <couchit/document.h>
#include <shared/scheduler.h>

#include "invoice.h"

#include "units.h"

namespace lnex {


class Exchange {
public:


	Exchange(couchit::CouchDB &db):db(db) {}


	static void init_monitor(std::shared_ptr<Exchange> me, couchit::ChangesDistributor &chdist, ondra_shared::Scheduler sch);


	json::String createDeposit(json::String order, Satoshi amount, const Invoice &invoice);
	bool changeInvoiceStatus(json::String id, bool paid);





protected:


	couchit::CouchDB &db;


	couchit::Document lastIdDoc;

	std::size_t tm = (std::size_t)-1;
	json::Value seqId;


	static bool onChange(std::shared_ptr<Exchange> me, const couchit::ChangeEvent &ev, ondra_shared::Scheduler sch);


};




}



#endif /* SRC_MAIN_EXCHANGE_H_ */
