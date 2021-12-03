/*
 * wallet.cpp
 *
 *  Created on: 3. 12. 2021
 *      Author: ondra
 */

#include <imtjson/binary.h>
#include <shared/logOutput.h>
#include "wallet.h"

using ondra_shared::logError;


namespace lnex {


std::string createAuthLine(const WalletConfig &cfg) {
	json::String authStr({cfg.login,":",cfg.password});
	std::string authLine="Basic ";
	json::base64->encodeBinaryValue(json::map_str2bin(authStr.str()) , [&](std::string_view str){
		authLine.append(str);
	});

	return authLine;
}


Wallet::Wallet(const WalletConfig &cfg, userver::HttpClientCfg &&httpc)
:rpc_client(std::move(httpc), cfg.url)
{
	rpc_client.setHeaders({
		{"Authorization",createAuthLine(cfg)}
	});
}

json::Value Wallet::listRequests() const {
	return rpc_client("list_requests", json::array);
}

json::Value Wallet::addRequest(double amount, const json::String &message) {
	char numbuff[100];
	sprintf(numbuff, "%1.8f",amount);
	return rpc_client("add_request", {
			json::Value::preciseNumber(numbuff), message, 24*60*60*7} );
}

json::Value Wallet::addLnRequest(double amount, const json::String &message) {
	char numbuff[100];
	sprintf(numbuff, "%1.8f",amount);
	return rpc_client("add_lightning_request", {
			json::Value::preciseNumber(numbuff), message});
}

json::Value Wallet::removeRequest(const json::String &reqId) {
	return rpc_client("rmrequest", json::Value(json::array,{reqId}));
}

WalletControl::WalletControl(const WalletConfig &cfg, userver::HttpClientCfg &&httpc, PaymentCallback &&cb, WalletMonitor &&wm)
	:Wallet(cfg, std::move(httpc)),cb(std::move(cb)), wm(std::move(wm))
{

}

static std::chrono::system_clock::time_point expiration(json::Value r) {
	auto tm = r["timestamp"].getUIntLong();
	auto sz = r["expiration"].getUIntLong();
	auto exptm = tm+sz;
	return std::chrono::system_clock::from_time_t(exptm);
}

WalletControl::Invoice WalletControl::createInvoice(bool lightning, double amount, const json::String &message) {
	if (lightning) {
		json::Value r = addLnRequest(amount, message);
		return {
			r["rhash"].toString(),
			r["invoice"].toString(),
			expiration(r)
		};
	}
	else {
		json::Value r = addRequest(amount, message);
		if (r.type() == json::boolean && r.getBool() == false) throw std::runtime_error("Gap limit reached");
		return {
			r["address"].toString(),
			r["URI"].toString(),
			expiration(r)
		};
	}

}

std::size_t WalletControl::start(ondra_shared::SharedObject<WalletControl> me,
		ondra_shared::Scheduler sch) {
	return sch.each(std::chrono::seconds(2)) >> [me]() mutable {
		me.lock()->checkState();
	};
}

void WalletControl::checkState() {
	try {
		json::Value r = listRequests();
		for (json::Value row: r) {
			int status = row["status"].getUInt();
			if (status == 1 || status == 3) {
				json::String reqId;
				if (row["is_lightning"].getBool()) {
					reqId = row["rhash"].toString();
				} else {
					reqId = row["address"].toString();
				}
				json::String msg = row["message"].toString();
				try {
					if (cb(reqId, msg, status == 3)) {
						removeRequest(reqId);
					}
				} catch (std::exception &e) {
					logError("Failed to process request: $1 - error: $2", reqId.str(), e.what());
				}
			}
		}
		if (pingCounter <= 0) {
			wm(true, balance());
			pingCounter = 50;
		} else {
			pingCounter--;
		}
	} catch (std::exception &e) {
		logError("Wallet connection failed: $1", e.what());
		if (pingCounter >= 0) {
			wm(false, json::Value());
			pingCounter = -1;
		}
	}
}

json::Value Wallet::balance() const {
	return rpc_client("getbalance", json::array);
}

LNPayResult Wallet::lnpay(const json::String invoice) {
	json::RpcResult ret = rpc_client("lnpay",{invoice, 10,60});
	if (ret.isError()) {
		int code = ret.getErrorCode();
		if (code == 1) {
			auto msg = ret.getErrorMessage();
			if (msg == "This invoice has been paid already") return {LNPayStatus::paid, ret};
			if (msg == "This invoice has expired") return {LNPayStatus::expired, ret};
			if (msg == "Bad bech32 checksum") return {LNPayStatus::invalid, ret};
			return {LNPayStatus::retry, ret};
		} else {
			return {LNPayStatus::retry, ret};
		}
	} else {
		json::Value result = ret;
		if (result["success"].getBool()) return {LNPayStatus::paid, ret};
		else return {LNPayStatus::retry, ret};
	}
}

}

