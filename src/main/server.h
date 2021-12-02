/*
 * server.h
 *
 *  Created on: 30. 5. 2021
 *      Author: ondra
 */

#ifndef SRC_MAIN_SERVER_H_
#define SRC_MAIN_SERVER_H_
#include <userver_jsonrpc/rpcServer.h>


class MyHttpServer: public userver::RpcHttpServer {
public:

	virtual void log(userver::ReqEvent event, const userver::HttpServerRequest &req) override;
	virtual void log(const userver::HttpServerRequest &req, userver::LogLevel lev,  const std::string_view &msg) override;
	virtual void logDirect(const std::string_view &s) override;
	virtual void unhandled() override;

};





#endif /* SRC_MAIN_SERVER_H_ */
