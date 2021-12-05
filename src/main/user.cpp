/*
 * user.cpp
 *
 *  Created on: 4. 12. 2021
 *      Author: ondra
 */

#include <imtjson/jwtcrypto.h>
#include <imtjson/rpc.h>
#include <openssl/pem.h>
#include "user.h"
#include <memory>

namespace lnex {

SessionParser::SessionParser(const std::string &pem_file, std::size_t cacheSize)
	:cacheSize(cacheSize)
{
	std::unique_ptr<FILE, decltype(&fclose)> f (fopen(pem_file.c_str(), "rb"), &fclose);
	if (f == nullptr) throw std::runtime_error("Unable to open pubkey: "+pem_file);
	EC_KEY *l = nullptr;
	if (PEM_read_EC_PUBKEY(f.get(), &l, 0, 0) == nullptr) {
				throw std::runtime_error("Unable to parse pubkey: "+pem_file);
	}
	jwt = new json::JWTCrypto_ES(l, 256);
}

UserInfo SessionParser::parse(const std::string_view &session, json::Value &parsed) const {
	json::Value r;
	auto iter = parsedTokens.find(session);
	if (iter == parsedTokens.end()) {
		iter = parsedTokens2.find(session);
		if (iter == parsedTokens2.end()) {
			  r = json::checkJWTTime(json::parseJWT(session, jwt));
			  parsed = r;
		}
	}
	if (!r.hasValue()) {
		return {false};
	} else {
		auto id = r["id"];
		auto adm = r["adm"];
		return {
			true, adm.getBool(), id.getString()
		};
	}
}

void SessionParser::remberToken(const std::string_view &session, json::Value parsed) {
	parsedTokens.emplace(std::string(session), parsed);
	if (parsedTokens.size() > cacheSize) {
		std::swap(parsedTokens, parsedTokens2);
		parsedTokens.clear();
	}
}

AuthUser::AuthUser(const std::string &pem_file, std::size_t cacheSize) {
	Super::operator=(make(pem_file, cacheSize));
}

UserInfo AuthUser::operator ()(const std::string_view &session) {
	json::Value p;
	UserInfo out = lock_shared()->parse(session, p);
	if (p.defined()) {
		lock()->remberToken(session, p);
	}
	return out;
}

UserInfo AuthUser::operator ()(json::RpcRequest &req, bool set_error) {
	json::Value ses = req.getContext()["session"];
	if (ses.type() == json::string) {
		UserInfo r = operator()(ses.getString());
		if (r.valid) return r;
	}
	if (set_error) req.setError(401,"Invalid session");
	return {false};
}

}


