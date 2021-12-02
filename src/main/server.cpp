/*
 * server.cpp
 *
 *  Created on: 30. 5. 2021
 *      Author: ondra
 */

#include <shared/logOutput.h>
#include "server.h"

using ondra_shared::logDebug;
using ondra_shared::logError;
using ondra_shared::logProgress;
using ondra_shared::logWarning;



void MyHttpServer::log(userver::ReqEvent event, const userver::HttpServerRequest &req) {
		if (event == userver::ReqEvent::done) {
			auto now = std::chrono::system_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::microseconds>(now-req.getRecvTime());
			char buff[100];
			snprintf(buff,100,"%1.3f ms", dur.count()*0.001);
			logProgress("[HTTP-$1] $2 $3 $4 $5 $6", req.getIdent(), req.getStatus(), req.getMethod(), req.getHost(), req.getPath(), buff);
		}
	}


void MyHttpServer::log(const userver::HttpServerRequest &req,userver::LogLevel lev,  const std::string_view &msg) {
	static const char *templ = "[HTTP-$1] $2";
	switch (lev) {
	case userver::LogLevel::debug: logDebug(templ,req.getIdent(), msg);break;
	case userver::LogLevel::progress: logProgress(templ,req.getIdent(), msg);break;
	case userver::LogLevel::warning: logWarning(templ,req.getIdent(), msg);break;
	case userver::LogLevel::error: logError(templ,req.getIdent(), msg);break;
	}
}
void MyHttpServer::logDirect(const std::string_view &s) {
	logProgress("[RPC-Direct] $1",s);
};
void MyHttpServer::unhandled() {
	try {
		throw;
	} catch (const std::exception &e) {
		logError("Unhandled exception: $1", e.what());
	} catch (...) {
		logError("Unhandled exception: <unknown>");
	}
}
