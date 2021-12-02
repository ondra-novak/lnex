cmake_minimum_required(VERSION 3.1)
project (lnex)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/)

include_directories(BEFORE src/imtjson/src src/couchit/src src)
add_compile_options(-std=c++17)
add_compile_options(-Wall -Werror -Wno-noexcept-type)

set(USERVER_NO_SSL 1)

execute_process(COMMAND git submodule update --init)

add_subdirectory (src/imtjson/src/imtjson)
add_subdirectory (src/userver)
add_subdirectory (src/userver_jsonrpc)
add_subdirectory (src/main)
add_subdirectory (src/couchit/src/couchit)
add_subdirectory (www)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/opt/lnex" CACHE PATH "Default path to install" FORCE)
endif()

install(PROGRAMS bin/lnex DESTINATION bin)
install(FILES conf/lnex.conf DESTINATION "conf")
install(FILES conf/auth.conf DESTINATION "conf")  
install(FILES www/release/index.html www/release/user.html www/release/wait.html DESTINATION "www")
install(DIRECTORY "www/release/conf" DESTINATION "www") 
install(DIRECTORY "www/release/img" DESTINATION "www") 
install(DIRECTORY DESTINATION "data")
install(DIRECTORY DESTINATION "log")
