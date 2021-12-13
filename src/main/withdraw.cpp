/*
 * invoice.cpp
 *
 *  Created on: 5. 12. 2021
 *      Author: ondra
 */

#include "withdraw.h"

namespace lnex {


json::NamedEnum<InvoiceType> strInvoiceType({
	{InvoiceType::invalid,"invalid"},
	{InvoiceType::address,"address"},
	{InvoiceType::ln,"ln"},
});



}




