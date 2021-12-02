#include <userver/http_server.h>
#include <shared/default_app.h>
#include <couchit/config.h>
#include <couchit/couchDB.h>
#include "../couchit/src/couchit/changes.h"
#include "../userver/static_webserver.h"
#include "server.h"

int main(int argc, char **argv) {

	using namespace userver;
	using namespace ondra_shared;

	ondra_shared::DefaultApp app({}, std::cerr);

	if (!app.init(argc, argv)) {
		std::cerr<<"Failed to initialize application" << std::endl;
		return 1;
	}

	auto section_server = app.config["server"];
	NetAddrList bind_addr = NetAddr::fromString(
			section_server.mandatory["listen"].getString(),
			"9500");
	unsigned int threads = section_server.mandatory["threads"].getUInt();
	unsigned int dispatchers = section_server["dispatchers"].getUInt(1);
	unsigned int maxupload = section_server.mandatory["max_upload_mb"].getUInt()*1024*1024;

	logProgress("---------- SERVER STAR ---------");
	for (const auto &x: bind_addr) logInfo("Listen: $1", x.toString(false));


	auto section_db = app.config["database"];

	couchit::Config dbcfg;
	dbcfg.authInfo.username = section_db.mandatory["username"].getString();
	dbcfg.authInfo.password = section_db.mandatory["password"].getString();
	dbcfg.baseUrl = section_db.mandatory["server_url"].getString();
	dbcfg.databaseName = section_db.mandatory["db_name"].getString();

	auto section_www = app.config["www"];
	userver::StaticWebserver::Config cfg;
	cfg.cachePeriod = section_www["cache"].getUInt();
	cfg.document_root = section_www.mandatory["document_root"].getPath();
	cfg.indexFile = section_www.mandatory["index_file"].getString();

	couchit::CouchDB couchdb(dbcfg);
	couchit::ChangesDistributor chdist(couchdb,true,true);



	MyHttpServer server;
	server.addRPCPath("/RPC", {
			true,true,true,maxupload
	});
	server.add_listMethods();
	server.add_ping();
/*
	server.addPath("/changes", [changes](std::unique_ptr<userver::HttpServerRequest> &req, std::string_view  ){
		auto last_id = req->get("Last-Event-ID");
		Value since;
		if (!last_id.empty()) since = last_id; else since = "0";
		req->setStatus(200);
		req->set("Cache-Control","no-cache");
		req->setContentType("text/event-stream");
		req->set("Connection","close");
		req->set("X-Accel-Buffering","no");
		changes->registerStream(std::move(req), since);
		return true;
	});*/
	server.addPath("", StaticWebserver(cfg));


	server.start(bind_addr, threads, dispatchers);

	chdist.runService([&]{
		userver::setThreadAsyncProvider(server.getAsyncProvider());
	});
	server.stopOnSignal();
	server.addThread();
	server.stop();
	chdist.stopService();
	logProgress("---------- SERVER STOP ---------");

}
