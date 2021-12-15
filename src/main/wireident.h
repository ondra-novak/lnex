/*
 * wireident.h
 *
 *  Created on: 5. 12. 2021
 *      Author: ondra
 */

#ifndef SRC_MAIN_WIREIDENT_H_
#define SRC_MAIN_WIREIDENT_H_
#include <imtjson/string.h>


namespace lnex {

struct WireIdent {
	///bank connection currently processed
	json::String connection;
	///identified other side of connections. May be more candidates (as partial informations are sent)
	/** If empty, then any connection is explored */
	std::vector<json::String> other_connections;
	///reference string - if known - reference string is searched for substring (can contain other info)
	///if reference is sent structured, then all informations are included separated by comma
	json::Value ref;
	///variable symbol
	json::Value vs;
	///specific symbol
	json::Value ss;
	///booking date - for record
	json::Value date;
	///Identification of other side
	/** Can be anything bban, iban, name, etc... It is used to identify refunds */
	json::Value ident;
	///amount in cents (*100)
	std::uint64_t amount;
	///validity time
	std::uint64_t time;


	///if true, the amount was paid, if false, amount was received (true = negative amount)
	bool paid;

};


}



#endif /* SRC_MAIN_WIREIDENT_H_ */
