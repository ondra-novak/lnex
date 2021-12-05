/*
 * invoice.h
 *
 *  Created on: 4. 12. 2021
 *      Author: ondra
 */

#ifndef SRC_MAIN_INVOICE_H_
#define SRC_MAIN_INVOICE_H_
#include <imtjson/string.h>
#include "units.h"
#include <chrono>

namespace lnex {

struct Invoice {
	json::String id;
	json::String text;
	std::chrono::system_clock::time_point expiration;
	Satoshi amount;


	json::Value toJsonExpiration() const {
		return json::Value(std::chrono::duration_cast<std::chrono::milliseconds>(expiration.time_since_epoch()).count());
	}
};


}



#endif /* SRC_MAIN_INVOICE_H_ */
