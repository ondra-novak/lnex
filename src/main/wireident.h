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
	///identified other side of connection. If empty, other side was not identified - (use other items)
	json::String other_connection;
	///reference string - if known
	json::String ref;
	///variable symbol
	json::String vs;
	///specific symbol
	json::String ss;
	///amount in cents (*100)
	std::uint64_t amount;
	///if true, the amount was paid, if false, amount was received (true = negative amount)
	bool paid;
};


}



#endif /* SRC_MAIN_WIREIDENT_H_ */
