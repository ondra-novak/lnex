/*
 * user.h
 *
 *  Created on: 4. 12. 2021
 *      Author: ondra
 */

#ifndef SRC_MAIN_USER_H_
#define SRC_MAIN_USER_H_
#include <imtjson/jwt.h>
#include <imtjson/string.h>
#include <shared/shared_object.h>
#include <map>

namespace json {
	class RpcRequest;
}

namespace lnex {

struct UserInfo {
	bool valid;
	bool admin;
	json::String id;
};

class SessionParser {
public:

	SessionParser(const std::string &pem_file, std::size_t cacheSize);

	UserInfo parse(const std::string_view &session, json::Value &parsed) const;

	void remberToken(const std::string_view &session, json::Value parsed);



protected:
	json::PJWTCrypto jwt;
	const std::size_t cacheSize;
	std::map<std::string, json::Value, std::less<> > parsedTokens, parsedTokens2;
};

class AuthUser: public ondra_shared::SharedObject<SessionParser> {
public:
	using Super = ondra_shared::SharedObject<SessionParser>;

	AuthUser(const std::string &pem_file, std::size_t cacheSize);
	UserInfo operator()(const std::string_view &session);
	UserInfo operator()(json::RpcRequest &req, bool set_error);
};



}



#endif /* SRC_MAIN_USER_H_ */
