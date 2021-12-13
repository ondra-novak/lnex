/*
 * withdraw.h
 *
 *  Created on: 5. 12. 2021
 *      Author: ondra
 */

#ifndef SRC_MAIN_WITHDRAW_H_
#define SRC_MAIN_WITHDRAW_H_
#include <imtjson/namedEnum.h>
#include <imtjson/string.h>
#include <main/units.h>

namespace lnex {

enum class PayResult {
	paid,
	retry,
	failed,
};

enum class InvoiceType {
	invalid,
	//ligtning invoice
	ln,
	//BTC address
	address,
};

extern json::NamedEnum<InvoiceType> strInvoiceType;


struct PayInfo {
	InvoiceType type;
	json::String text;
	Satoshi amount;
};


}




#endif /* SRC_MAIN_WITHDRAW_H_ */
