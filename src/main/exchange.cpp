/*
 * exchange.cpp
 *
 *  Created on: 4. 12. 2021
 *      Author: ondra
 */

#include <couchit/changeset.h>
#include <couchit/query.h>
#include <couchit/result.h>
#include <shared/logOutput.h>
#include "exchange.h"

using ondra_shared::logError;

namespace lnex  {

json::NamedEnum<TradeState> strTradeState({
	{TradeState::attantion,"attantion"},
	{TradeState::canceled,"canceled"},
	{TradeState::created,"created"},
	{TradeState::filled,"filled"},
	{TradeState::open,"open"},
	{TradeState::open_recv,"open_recv"},
	{TradeState::open_send,"open_send"},
	{TradeState::refund,"refund"},
});

static json::Value designDocWithdraw = json::Object{
	{"_id","_design/withdraw"},
	{"language","javascript"},
	{"views",json::Object{
		{"to_pay", json::Object {
			{"map",R"js(
function(doc) {
	if (doc._id.substr(0,3) == "ex." && doc.asset && doc.asset.invoice && doc.asset.amount && doc.status == "filled" && doc.state && doc.state.withdraw) {
		var withdraw = doc.state.withdraw;
		if (!withdraw.filled && !withdraw.failed) {
			emit(withdraw.timestamp || 0, [doc.asset.invoice, doc.asset.type, doc.asset.amount]);
		}
	}
}
)js"}
		}},
		{"by_invoice", json::Object {
			{"map", R"js(
function(doc) {
	if (doc._id.substr(0,3) == "ex." && doc.status == "filled" && doc.state && doc.state.withdraw) {
		var withdraw = doc.state.withdraw;
		if (!withdraw.filled && !withdraw.failed) {
			emit(doc.asset.invoice);
		}		
	}
}
)js"}
		} }
	} }
};

static json::Value designDocExchange = json::Object {
	{"_id","_design/exchange"},
	{"language","javascript"},
	{"views", json::Object {
		{"order_balance", json::Object {
			{"map","function(doc) {\n  if (doc._id.substr(0,3) == \"ex.\") {\n   \tvar asset = doc.asset;\n    var order = doc.order;\n    var status = doc.status;\n    if (asset && order && status && status != \"canceled\") {\n     \tvar amount = asset.amount || 0;\n\t\t\tvar fee = asset.fee || 0;\n      if (amount >0) {\n            emit(order, -(amount+fee));\n      }\n    }    \n  } else if (doc._id.substr(0,3) == \"dp.\") {\n   \tvar order = doc.order;\n    var amount = doc.amount || 0;\n    var filled = doc.filled;\n    if (order && amount > 0 && filled) {\n     \temit(order, amount); \n    }    \n  }\n}"},
			{"reduce","_sum"}
		}},
		{"open_trades",json::Object{
			//[[true,acc],type] - sends
			//[[false, acc],type] - receives
			//[true, expiration] - open trade, when expires current state
			//types - "buyer" - filled send side
			//        "seller - filled receive side
			//		  "refund - filled refund side
			//		  "canceled - filled receive side but
			{"map",R"js(
function(doc) {
	if (doc._id.substr(0,3) == "ex.") {
		var status = doc.status;
		if (status) {
			var cm = status == "open" || status == "open_recv" || status == "open_send" || status == "attantion" || status == "refund";  			
			if (cm || status == "canceled") {
					emit([true, doc.buyer],"buyer");
					emit([false, doc.seller],"seller");
					emit([true, doc.seller],"refund");
			}
			if (cm || status == "created") {
				emit(true, [doc.status_timestamp, status]);
			}
		}
	}
}
)js"}
		}}
	} }
};

static couchit::View to_pay_view("_design/withdraw/_view/to_pay",couchit::View::update);
static couchit::View by_invoice_view("_design/withdraw/_view/by_invoice",couchit::View::update|couchit::View::includeDocs);
static couchit::View order_balance_view("_design/exchange/_view/order_balance", couchit::View::reduce|couchit::View::update);
static couchit::View open_trades_view("_design/exchange/_view/open_trades", couchit::View::update);

void Exchange::init_monitor(std::shared_ptr<Exchange> me, couchit::ChangesDistributor &chdist) {

	me->db.putDesignDocument(designDocExchange);

	me->reloadOpenTrades();

	me->lastIdDoc = me->db.getLocal("exchange_lastid",couchit::CouchDB::flgCreateNew);
	auto since = me->lastIdDoc["since"];
	chdist.addFn([me](const couchit::ChangeEvent &ev){
		return me->onChange(ev);
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

PayInfo Exchange::getInvoiceToPay() const {
	couchit::Result res(db.createQuery(to_pay_view).limit(1).exec());
	if (res.empty()) return {InvoiceType::invalid,"",0};
	else {
		json::Value v = res[0]["value"];
		return {strInvoiceType[v[1].toString()], v[0].toString(), v[1].getUIntLong()};
	}
}

void Exchange::markInvoice(const json::String &invoice, PayResult payres, const json::String &info) {
	couchit::Result res(db.createQuery(by_invoice_view).key(invoice).exec());
	if (!res.empty()) {
		auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		couchit::Document doc(res[0]["doc"]);
		auto state = doc.object("state");
		auto withdraw = doc.object("withdraw");
		withdraw.set("timestamp", now);
		switch (payres) {
		case PayResult::paid:
			withdraw.set("filled", true);
			withdraw.set("tx", info);
			break;
		case PayResult::failed:
			withdraw.set("failed", true);
			withdraw.set("last_error", info);
			break;
		default:
		case PayResult::retry:
			withdraw.set("last_error", info);
			break;

		}
		try {
			db.put(doc);
		} catch (const couchit::UpdateException &e) {
			if (e.getError(0).isConflict()) markInvoice(invoice, payres, info);
			else throw;
		}

	}
}

void Exchange::init_withdraw() {
	db.putDesignDocument(designDocWithdraw);
}

bool Exchange::onChange(const couchit::ChangeEvent &ev) {
	auto now = std::chrono::system_clock::now();
	if (now > delayed_seqId) {
		if (seqId.hasValue()) {
			lastIdDoc.set("since", seqId);
			try {
				db.put(lastIdDoc);
				seqId = json::Value();
				delayed_seqId = std::chrono::system_clock::time_point::max();
			} catch (const couchit::UpdateException &e) {
				if (e.getError(0).isConflict()) {
					lastIdDoc = db.get(lastIdDoc.getID(), couchit::CouchDB::flgCreateNew);
				}
			}
		}
	}
	if (now > delayed_orders) {
		try {
			updateOrderBalance();
			ordersToUpdate.clear();
			delayed_orders = std::chrono::system_clock::time_point::max();
		} catch (std::exception &e) {
			delayed_orders = now + std::chrono::seconds(1);
			logError("Failed to update orders: $1", e.what());
		}
	}

	try {
		processTimeouts(now);
	} catch (std::exception &e) {
		logError("Error while processing timeouts: $1", e.what());
	}

	if (ev.idle) return true;
	if (ev.seqId.hasValue()) {
		seqId = ev.seqId;
		delayed_seqId = now + std::chrono::seconds(1);
	}

	auto evdoc = ev.doc;
	if (!ev.deleted) {

		//exchange
		if (ev.id.substr(0,3) == "ex.") {
			auto status = evdoc["status"];
			auto order = evdoc["order"];
			if (!status.hasValue()) {
				lockOrderBalance(evdoc);
			} else {
				if (order.type() == json::string) {
					if (ordersToUpdate.empty()) delayed_orders = now + std::chrono::seconds(1);
					ordersToUpdate.insert(order);
				}
				updateTrade(evdoc);

			}
		} else
		//deposit - update orders if filled
		if (ev.id.substr(0,3) == "dp." && evdoc["filled"].getBool()) {
			auto order = evdoc["order"];
			if (order.type() == json::string) {
				if (ordersToUpdate.empty()) delayed_orders = now + std::chrono::seconds(1);
				ordersToUpdate.insert(order);
			}
		}
	}
	return true;
}

void Exchange::updateOrderBalance() {
	std::unordered_map<json::Value, Satoshi> orderMap;
	json::Value keys (json::array, ordersToUpdate.begin(), ordersToUpdate.end(), [&](json::Value order){
		orderMap.emplace(order,0);
		return order;
	});
	couchit::Result res(db.createQuery(order_balance_view).keys(keys).exec());
	for (couchit::Row rw: res) {
		auto order = rw.key;
		Satoshi amount = rw.value.getUIntLong();
		orderMap[order] = amount;
	};

	do {
		couchit::Changeset chset(db.createChangeset());
		couchit::Result orders(db.allDocs(couchit::View::includeDocs).keys(keys).exec());
		for (couchit::Row rw: orders) {
			if (rw.doc.type() == json::object) {
				couchit::Document doc(rw.doc);
				Satoshi needBalance = orderMap[rw.key];
				if (doc["balance"].getUIntLong() != needBalance) {
					doc.set("balance", needBalance);
					chset.update(doc);
				}
			}
		}
		try {
			chset.commit();
			return;
		} catch (couchit::UpdateException &e) {
			json::Array newkeys;
			for (const auto &err: e.getErrors()) {
				if (err.isConflict()) {
					newkeys.push_back(err.docid);
				}
			}
			keys = newkeys;
			if (keys.empty()) throw;
		}
	} while (true);


}


void Exchange::lockOrderBalance(const json::Value &exchangeDoc) {
	auto order = exchangeDoc["order"];
	couchit::Result res(db.createQuery(order_balance_view).key(order).exec());
	if (!res.empty()) {
		Satoshi balance = res[0]["value"].getUIntLong();
		Satoshi toLock = exchangeDoc["asset"]["amount"].getUIntLong();
		Satoshi fee = exchangeDoc["asset"]["fee"].getUIntLong();

		if (toLock+fee <= balance) {
			couchit::Document doc (exchangeDoc);
			if (doc["withdraw"].getBool()) {
				//withdraw
				doc.set("status","filled");
			} else {
				//exchange - set to created
				doc.set("status","created");
			}
			db.put(doc);
			return;
		}
	}
	couchit::Document doc (exchangeDoc);
	doc.setDeleted({});
	db.put(doc);
}

bool Exchange::processWireTransfer(const WireIdent &wident) {
	return false;
}

bool Exchange::cmpOpenTrades(const OpenTrade &a, const OpenTrade &b) {
	return a.timeout > b.timeout;
}

void Exchange::reloadOpenTrades() {
	openTrades.clear();
	couchit::Result res(db.createQuery(open_trades_view).key(true).exec());
	for (couchit::Row rw: res) {
		openTrades.push_back(makeOpenTrade(rw.id.toString(), rw.value[0].getUIntLong(), strTradeState[rw.value[1].getString()]));
	}
	std::make_heap(openTrades.begin(), openTrades.end(), cmpOpenTrades);
}

Exchange::OpenTrade Exchange::makeOpenTrade(const json::String orderId, std::uint64_t timeout, TradeState state) {
	std::uint64_t exp = 0;
	switch (state) {
	case TradeState::attantion: exp = 7*24*60;break;
	case TradeState::canceled: exp = 30*24*60;break;
	case TradeState::created: exp = 30;break;
	default:break;
	case TradeState::open: exp = 6*60 ;break;
	case TradeState::open_recv: exp = 24*60 ;break;
	case TradeState::open_send: exp = 7*24*60;break;
	}
	return {std::chrono::system_clock::time_point(std::chrono::milliseconds(timeout + exp*60000)), orderId};
}

void Exchange::updateTrade(const json::Value &trade) {
	auto id = trade["_id"].toString();
	auto iter = std::find_if(openTrades.begin(), openTrades.end(), [&](const OpenTrade &c) {
		return c.tradeId == id;
	});
	if (iter != openTrades.end()) openTrades.erase(iter);
	auto status = trade["status"];
	if (status.defined()) {
		openTrades.push_back(makeOpenTrade(id, trade["status_timeout"].getUIntLong(), strTradeState[status.getString()]));
		std::make_heap(openTrades.begin(), openTrades.end(), cmpOpenTrades);
	}
}

void Exchange::processTimeouts(const std::chrono::system_clock::time_point &tp) {
	while (openTrades.empty()) {
		const auto &top = openTrades[0];
		if (top.timeout > tp) break;
		auto tradeId = top.tradeId;
		std::pop_heap(openTrades.begin(), openTrades.end(), cmpOpenTrades);
		openTrades.pop_back();
		processTimeoutedTrade(tradeId,tp);
	}
}

void Exchange::processTimeoutedTrade(const json::String &id, const std::chrono::system_clock::time_point &tp) {

	json::Value v = db.get(id, couchit::CouchDB::flgNullIfMissing);
	if (v.hasValue()) {
		json::Value status = v["status"];
		if (status.defined()) {
			couchit::Document doc(v);
			TradeState newstate;
			auto st = strTradeState[status.getString()];
			switch (st) {
			case TradeState::created: newstate = TradeState::open;break;
			case TradeState::open: newstate = TradeState::canceled;break;
			case TradeState::open_recv: newstate = TradeState::canceled;break;
			case TradeState::open_send: newstate = TradeState::attantion;break;
			case TradeState::attantion: newstate = TradeState::filled;break;
			case TradeState::canceled:
				doc.setDeleted({}, true);
				db.put(doc,{},false);
				return;
			default: return;
			}
			doc.set("status",strTradeState[newstate]);
			doc.set("status_timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count());
			db.put(doc,{},false);
		}
	}
}

}


