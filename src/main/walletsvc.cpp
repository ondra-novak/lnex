/*
 * walletsvc.cpp
 *
 *  Created on: 13. 12. 2021
 *      Author: ondra
 */

#include <couchit/document.h>
#include <couchit/query.h>
#include <couchit/result.h>
#include <imtjson/object.h>
#include <main/walletsvc.h>
#include <shared/logOutput.h>

using ondra_shared::logWarning;

namespace lnex {

json::Value walletDesignDoc = json::Object {
	{"_id","_design/deposit"},
	{"language","javascript"},
	{"views",json::Object{
		{"invoices",json::Object{
			{"map",R"js(function(doc) {
if (doc._id.substr(0,3) == "dp." && !doc.filled && doc.req_id) {
	emit(doc.req_id);
}
})js"}
		}}
	}}

};

static couchit::View waitingInvoices("_design/deposit/_view/invoices", couchit::View::includeDocs);


WalletService::WalletService(couchit::CouchDB &db,const WalletConfig &cfg, userver::HttpClientCfg &&httpc)
		:db(db),wallet(cfg, std::move(httpc), [this](json::String id, json::String msg, bool paid){
			return onInvoiceChange(id, paid);
	},[this](bool, json::Value){})
{

}

void WalletService::init(std::shared_ptr<WalletService> me, couchit::ChangesDistributor &chdist, ondra_shared::Scheduler sch) {
	me->db.putDesignDocument(walletDesignDoc);
	sch.each(std::chrono::seconds(2)) >> [me]{
		me->checkWalletState();
	};
	chdist.addFn([me](const couchit::ChangeEvent &ev){
		me->processWalletRequest(ev);
		return true;
	});
}

void WalletService::processWalletRequest(const couchit::ChangeEvent &ev) {
	try {
		if (ev.idle) return;
		if (ev.id.substr(0,3)=="dp.") {
			if (!ev.doc["invoice"].defined() && !ev.doc["filled"].getBool()) {
				std::string_view dpid =  ev.id.substr(3);
				Invoice inv = wallet.createInvoice(true, ev.doc["amount"].getUIntLong(), json::String({"LNEX Deposit",dpid}));
				couchit::Document doc(ev.doc);
				doc.set("invoice", inv.text);
				doc.set("req_id", inv.id);
				doc.set("expiration", std::chrono::duration_cast<std::chrono::milliseconds>(inv.expiration.time_since_epoch()).count());
				db.put(doc);
			}
		}else  if (ev.id.substr(0,3)=="ub.") { //user buy order - ub.<userid>
			json::Value invoice = ev.doc["invoice"];
			if (invoice.defined()) {
				std::hash<json::Value> jhsh;
				auto nh = jhsh(invoice.stripKey());
				auto ch = ev.doc["invoice_hash"].getUIntLong();
				if (ch != nh) {
						Invoice inv = wallet.parseInvoice(ev.doc["invoice"].toString());
						couchit::Document doc(ev.doc);
						doc.set("invoice_hash", nh);
						doc.set("amount", inv.amount);
						doc.set("req_id", inv.id);
						doc.set("expiration", std::chrono::duration_cast<std::chrono::milliseconds>(inv.expiration.time_since_epoch()).count());
						db.put(doc);
				}

			}
		}
	} catch (std::exception &e) {
		logWarning("Exception in processWalletRequest: $1", e.what());
	}
}

void WalletService::checkWalletState() {
	wallet.checkState();
}

bool WalletService::onInvoiceChange(json::String id, bool paid) {
	couchit::Result res(db.createQuery(waitingInvoices).key(id).includeDocs().exec());
	if (res.empty()) return !paid;
	couchit::Document doc(couchit::Row(res).doc);
	if (paid) {
		doc.set("filled", true);
	} else {
		doc.setDeleted({}, false);
	}
	db.put(doc);
	return true;
}

} /* namespace lnex */
