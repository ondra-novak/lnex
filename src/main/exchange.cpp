/*
 * exchange.cpp
 *
 *  Created on: 4. 12. 2021
 *      Author: ondra
 */

#include "exchange.h"

namespace lnex  {

void Exchange::init_monitor(std::shared_ptr<Exchange> me, couchit::ChangesDistributor &chdist, ondra_shared::Scheduler sch) {


	me->lastIdDoc = me->db.getLocal("exchange_lastid",couchit::CouchDB::flgCreateNew);
	auto since = me->lastIdDoc["since"];
	chdist.addFn([me,sch](const couchit::ChangeEvent &ev){
		return onChange(me, ev, sch);
	},since,{couchit::CouchDB::Filter::no_filter});
}

json::String Exchange::createDeposit(json::String order, Satoshi amount, const Invoice &invoice) {
	json::String id({"dp.", invoice.id});
	couchit::Document doc;
	doc.setID(id);
	doc.setItems({
		{"order", order},
		{"amount", amount},
		{"invoice",invoice.text},
		{"expiration",invoice.toJsonExpiration()},
		{"filled", false}
	});

	doc.enableTimestamp();
	db.put(doc);
	return id;
}

bool Exchange::changeInvoiceStatus(json::String id, bool paid) {
	json::String docid({"dp.", id});
	json::Value st = db.get(docid, couchit::CouchDB::flgNullIfMissing);
	if (st.hasValue() && !st["filled"].getBool()) {
		couchit::Document doc(st);
		if (paid) {
			doc.set("filled", true);
		} else {
			doc.setDeleted({}, true);
		}
		db.put(doc);
		return true;
	} else {
		return false;
	}

}

bool Exchange::onChange(std::shared_ptr<Exchange> me, const couchit::ChangeEvent &ev, ondra_shared::Scheduler sch) {
	if (ev.idle) return true;
	if (ev.seqId.hasValue()) {
		me->seqId = ev.seqId;
	}
	if (ev.id.substr(0,3) == "ex.") {
		//TODO process event

//		json::Value exdoc = ex.doc;


		if (me->seqId.hasValue()) {
			sch.remove(me->tm);
			me->tm = sch.after(std::chrono::milliseconds(500)) >> [me, id = me->seqId]{
				me->lastIdDoc.set("since", id);
				me->db.put(me->lastIdDoc);
			};
			me->seqId = json::Value();
		}

	}
	return true;
}

}


