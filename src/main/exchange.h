/*
 * exchange.h
 *
 *  Created on: 4. 12. 2021
 *      Author: ondra
 */

#ifndef SRC_MAIN_EXCHANGE_H_
#define SRC_MAIN_EXCHANGE_H_
#include <set>
#include <unordered_set>
#include <couchit/changes.h>
#include <couchit/couchDB.h>
#include <couchit/document.h>
#include <shared/scheduler.h>
#include <userver/callback.h>

#include "units.h"
#include "invoice.h"
#include "wireident.h"
#include "withdraw.h"


namespace lnex {

enum class TradeState {
	undecided,
	created,
	open,
	open_recv,
	open_send,
	filled,
	attantion,
	refund,
	canceled,
};

extern json::NamedEnum<TradeState> strTradeState;


class Exchange {
public:


	Exchange(couchit::CouchDB &db):db(db) {}

	///init withdraw features - need to service handling withdraw
	void init_withdraw();
	///init monitoring - needs to service handling whole exchange (except withdraw)
	static void init_monitor(std::shared_ptr<Exchange> me, couchit::ChangesDistributor &chdist);


	///Create deposit request
	/**Deposit is created in wallet, then stored by this function to database */
	json::String createDeposit(json::String order, Satoshi amount, const Invoice &invoice);
	///Change invoice status - reported by wallet
	/**Once the request is paid or expired, this function must be called
		@param id request id
		@param paid true if paid, false if expired
		@retval true done
		@retval false deposit object not found
		*/
	bool changeInvoiceStatus(json::String id, bool paid);

	///Retrieve first invoice to by paid
	/**
	 * @return invoice to pay - withdraw - if invalid, nothing to withdraw
	 */
	PayInfo getInvoiceToPay() const;

	///Mark invoice as paying progress
	/**
	 * @param invoice invoice text
	 * @param payres result of payment
	 * @param info if invoice is paid, contains information about payment, otherwise contains last error
	 */
	void markInvoice(const json::String &invoice, PayResult payres, const json::String &info);


	///Searchs open trades, if trade found, it is market apropriately
	/**
	 * @param wident wire identification
	 */
	void processWireTransfer(const WireIdent &wident);
	///Determines, whether bank connection is still active
	/**
	 * @retval true still active, reschedule
	 * @retval false no longer active, don't monitor
	 */
	bool isStillActiveConnection(const json::String &connection) const;





protected:


	couchit::CouchDB &db;


	couchit::Document lastIdDoc;

	std::size_t tm = (std::size_t)-1;
	json::Value seqId;


	bool onChange(const couchit::ChangeEvent &ev);

	std::chrono::system_clock::time_point delayed_seqId;
	std::chrono::system_clock::time_point delayed_orders;
	std::unordered_set<json::Value> ordersToUpdate;

	void updateOrderBalance();
	void lockOrderBalance(const json::Value &exchangeDoc);


	struct OpenTrade {
		std::chrono::system_clock::time_point timeout;
		json::String tradeId;
	};

	using OpenTrades = std::vector<OpenTrade>;

	OpenTrades openTrades;


	void reloadOpenTrades();
	static bool cmpOpenTrades(const OpenTrade &a, const OpenTrade &b);
	static OpenTrade makeOpenTrade(const json::String orderId, std::uint64_t timeout, TradeState state);
	void updateTrade(const json::Value &trade);

	void processTimeouts(const std::chrono::system_clock::time_point &tp);
	void processTimeoutedTrade(const json::String &id, const std::chrono::system_clock::time_point &tp);
};




}



#endif /* SRC_MAIN_EXCHANGE_H_ */
