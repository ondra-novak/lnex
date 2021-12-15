#include <userver/http_server.h>
#include <shared/default_app.h>
#include <couchit/config.h>
#include <couchit/couchDB.h>
#include <userver/http_client.h>
#include <userver/ssl.h>
#include "../couchit/src/couchit/changes.h"
#include "../userver/static_webserver.h"
#include "server.h"
#include "walletsvc.h"

int main(int argc, char **argv) {

	using namespace userver;
	using namespace ondra_shared;
	using namespace lnex;

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

	auto section_wallet = app.config["wallet"];
	std::optional<WalletConfig> wcfg;
	if (section_wallet["enabled"].getBool(false)) {
		wcfg.emplace();
		wcfg->login = section_wallet.mandatory["login"].getString();
		wcfg->password= section_wallet.mandatory["password"].getString();
		wcfg->url= section_wallet.mandatory["url"].getString();
	}

	logProgress("---------- SERVER STAR ---------");
	for (const auto &x: bind_addr) logInfo("Listen: $1", x.toString(false));


	auto section_db = app.config["database"];

	couchit::Config dbcfg;
	dbcfg.authInfo.username = section_db.mandatory["username"].getString();
	dbcfg.authInfo.password = section_db.mandatory["password"].getString();
	dbcfg.baseUrl = section_db.mandatory["server_url"].getString();
	dbcfg.databaseName = section_db.mandatory["db_name"].getString();

	auto section_www = app.config["www"];
	std::optional<userver::StaticWebserver::Config> www_cfg;
	if (section_www["enabled"].getBool()) {
		www_cfg.emplace();
		www_cfg->cachePeriod = section_www["cache"].getUInt();
		www_cfg->document_root = section_www.mandatory["document_root"].getPath();
		www_cfg->indexFile = section_www.mandatory["index_file"].getString();
	}

	couchit::CouchDB couchdb(dbcfg);
	couchit::ChangesDistributor chdist(couchdb,true,true);
	ondra_shared::Scheduler sch = sch.create();


	if (wcfg.has_value()) {
		auto wallet = std::make_shared<WalletService>(couchdb,*wcfg, HttpClientCfg{"lnex.cz"});
		wallet->init(wallet, chdist, sch);
	}


	MyHttpServer server;
	server.addRPCPath("/RPC", {
			true,true,true,maxupload
	});
	server.add_listMethods();
	server.add_ping();
	if (www_cfg.has_value()) {
		server.addPath("", StaticWebserver(*www_cfg));
	}


	server.start(bind_addr, threads, dispatchers);



	sch.immediate() >> [provider = server.getAsyncProvider()]{
		userver::setThreadAsyncProvider(provider);
	};


	chdist.runService([&]{
		userver::setThreadAsyncProvider(server.getAsyncProvider());
	});
	server.stopOnSignal();
	server.addThread();
	server.stop();
	chdist.stopService();
	logProgress("---------- SERVER STOP ---------");

}
