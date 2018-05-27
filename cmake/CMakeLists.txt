cmake_minimum_required(VERSION 2.6)
project(copilotd)


file( GLOB srcEvillib   "../src/evillib.c" )

file( GLOB srcCopilotd  
"../src/plugins/coreService.cpp"
"../src/plugins/pubsub.cpp"
"../src/plugins/eDB.cpp" 
"../src/main.cpp" 
"../src/core/*.c" 
"../src/core/*.cpp" )

# This we need
LIST( APPEND LIBRARYS sodium )
LIST( APPEND LIBRARYS jansson )
LIST( APPEND LIBRARYS pthread )

option( PLUGIN_SSL "Enable SSL" ON )
if(PLUGIN_SSL)
	file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/sslService.cpp" "../src/plugins/sslSession.cpp" )
	LIST( APPEND LIBRARYS gnutls )
endif()


option( PLUGIN_WS "Enable Websocket" OFF )
if(PLUGIN_WS)
	file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/uwebsockets.cpp" )
	include_directories( "/usr/include/uWS" )
	LIST( APPEND LIBRARYS uWS )
	LIST( APPEND LIBRARYS ssl )
	LIST( APPEND LIBRARYS crypto )
	LIST( APPEND LIBRARYS z )
	LIST( APPEND LIBRARYS uv )
endif()


# -DUSE_MY_LIB=ON
option( PLUGIN_LDAP "Enable LDAP" ON )
if(PLUGIN_LDAP)
	file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/ldapService.cpp" )
	LIST( APPEND LIBRARYS ldap )
else()
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_LDAP" )
endif()

option( PLUGIN_NFT "Enable NFT" ON )
if(PLUGIN_NFT)
	file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/nftService.cpp" )
else()
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_NFT" )
endif()


option( PLUGIN_STS "Enable System State" ON )
if(PLUGIN_STS)
	file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/sysState.cpp" "../src/plugins/sysStateCmd.cpp" )
else()
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_SYSSTATE" )
endif()

option( PLUGIN_SYSTEMD "Enable Systemd" ON )
if(PLUGIN_SYSTEMD)
	file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/syslogd.cpp" )
	LIST( APPEND LIBRARYS systemd )
else()
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_SYSTEMD" )
endif()



include_directories( "../libs/evillib/core" ) 
include_directories( "../libs/evillib/extra" )
include_directories( "../libs/util-linux/libuuid/src" )
include_directories( "../src" )
include_directories( "../src/core" )


add_executable(copilotd ${srcEvillib} ${srcCopilotd})
target_link_libraries( copilotd ${LIBRARYS} )



execute_process( COMMAND git log -n 1 --pretty=format:"%h %ai"
                 OUTPUT_VARIABLE GITVERSION
)

FILE(WRITE ../src/gitversion.h
    "#define gitversion ${GITVERSION}"
)



set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pie -fPIE -fPIC -fstack-protector -Wformat -Wformat-security -DPACKAGE='\"copilotd\"' -DLOCALEDIR='\"/usr/share/locale\"' -DDISABLE_SSH")




target_link_libraries( copilotd z )
target_link_libraries( copilotd uuid )
target_link_libraries( copilotd dl )






target_link_libraries( copilotd sqlite3 )





