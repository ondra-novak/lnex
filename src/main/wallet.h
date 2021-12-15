/*
 * wallet.h
 *
 *  Created on: 3. 12. 2021
 *      Author: ondra
 */

#ifndef SRC_MAIN_WALLET_H_
#define SRC_MAIN_WALLET_H_
#include <shared/scheduler.h>
#include <shared/shared_object.h>
#include <string>
#include <userver/http_client.h>
#include <userver_jsonrpc/rpcClient.h>

#include "invoice.h"

namespace lnex {

struct WalletConfig {
	std::string url;
	std::string login;
	std::string password;
};

enum class LNPayStatus {
	//invoice has been paid
	paid,
	//temporary error, retry
	retry,
	//invoice is expired (or paid)
	expired,
	//invoice is invalid
	invalid,

};

struct LNPayResult {
	LNPayStatus status;
	json::Value details;
};


class Wallet {
public:
	Wallet(const WalletConfig &cfg, userver::HttpClientCfg &&httpc);

	json::Value listRequests() const;
	json::Value addRequest(Satoshi amount, const json::String &message);
	json::Value addLnRequest(Satoshi amount, const json::String &message);
	json::Value removeRequest(const json::String &reqId);
	json::Value balance() const;
	json::Value decodeInvoice(const json::String &invoice) const;

	LNPayResult lnpay(const json::String invoice);



protected:

	mutable userver::HttpJsonRpcClient rpc_client;
	std::string authline;

};



///Callback when pay
/**
 * @param id payment id
 * @param msg invoice message
 * @param paid true paid, false expired
 * @retval true delete invoice
 * @retval false ignore invoice
 */
using PaymentCallback = userver::CallbackT<bool(json::String id, json::String msg, bool paid)>;
using WalletMonitor = userver::CallbackT<void(bool, json::Value)>;

class WalletControl: public Wallet {
public:

	WalletControl(const WalletConfig &cfg, userver::HttpClientCfg &&httpc, PaymentCallback &&cb, WalletMonitor &&wm);


	Invoice createInvoice(bool lightning, Satoshi amount, const json::String &message);
	Invoice parseInvoice(const json::String &invoice);


	static std::size_t start(ondra_shared::SharedObject<WalletControl> me, ondra_shared::Scheduler sch);

	///manually check state (can be called from scheduler)
	void checkState();


protected:


	PaymentCallback cb;
	WalletMonitor wm;
	int pingCounter = 0;

};

using PWalletControl = ondra_shared::SharedObject<WalletControl>;

}



#endif /* SRC_MAIN_WALLET_H_ */
