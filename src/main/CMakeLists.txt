cmake_minimum_required(VERSION 3.1) 

add_executable (lnex
		start.cpp
		server.cpp
		wallet.cpp
		user.cpp
		exchange.cpp
		withdraw.cpp
		walletsvc.cpp
)
target_link_libraries (lnex LINK_PUBLIC userver_jsonrpc couchit userver imtjson gd stdc++fs crypto pthread)
