cmake_minimum_required(VERSION 3.0) 

set(webfiles src/main.js)

add_custom_target(web ALL DEPENDS index.html index_debug.html) 


add_custom_command(OUTPUT index.html COMMAND src/spamake packed src/main.js index.html
	DEPENDS ${webfiles})
add_custom_command(OUTPUT index_debug.html COMMAND src/spamake devel src/main.js index_debug.html
	DEPENDS ${webfiles})
	
