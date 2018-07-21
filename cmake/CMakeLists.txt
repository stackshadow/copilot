cmake_minimum_required(VERSION 2.6)
project(copilotd)


# some variables we need
execute_process( COMMAND nproc OUTPUT_VARIABLE NCPUS
)


# sources - evillib
file( GLOB srcEvillib   "../src/evillib.c" )

# sources - copilotd
file( GLOB srcCopilotd  
"../src/plugins/coreService.cpp"
"../src/plugins/pubsub.cpp"
"../src/core/*.c" 
"../src/core/*.cpp" )

# sources - uwebsocket
file( GLOB srcUWebSockets
    "../libs/uWebSockets/src/*.cpp"
)



# This we need
LIST( APPEND LIBRARYS sodium )
LIST( APPEND LIBRARYS jansson )
LIST( APPEND LIBRARYS pthread )






option( PLUGIN_SSL "Enable SSL" OFF )
if(PLUGIN_SSL)
    file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/sslService.cpp" "../src/plugins/sslSession.cpp" )
    LIST( APPEND LIBRARYS gnutls )
else()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_SSL" )
endif()

# libretls
option( PLUGIN_LTLS "Enable SSL" ON )
if(PLUGIN_LTLS)

    add_custom_command( 
        COMMAND git clone --depth 1 https://github.com/libressl-portable/portable.git ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src 
        OUTPUT ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src/autogen.sh 
    )
    add_custom_command( 
        DEPENDS ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src/autogen.sh 
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src
        COMMAND bash autogen.sh
        OUTPUT ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src/configure
    )
    add_custom_command( 
        DEPENDS ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src/configure
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src
        COMMAND ./configure --prefix ${CMAKE_SOURCE_DIR}/../libs/libressl
        OUTPUT ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src/Makefile
    )
    add_custom_command( 
        DEPENDS ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src/Makefile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../libs/libressl-portable-src
        COMMAND make -j${NCPUS}
        COMMAND make install
        OUTPUT ${CMAKE_SOURCE_DIR}/../libs/libressl/lib/libtls.so
    )
    
    add_custom_target( libressl
        DEPENDS ${CMAKE_SOURCE_DIR}/../libs/libressl/lib/libtls.so
    )
    
# install
    file( GLOB libreSSLSharedObjects "${CMAKE_SOURCE_DIR}/../libs/libressl/lib/*.so*" )
    install( FILES ${libreSSLSharedObjects} DESTINATION share/copilot/lib COMPONENT runtime)

    
# 
    file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/lsslService.cpp" )
    
    
    
# link copilot against this lib
    LIST( APPEND LIBRARYS ${CMAKE_SOURCE_DIR}/../libs/libressl/lib/libssl.so )
    LIST( APPEND LIBRARYS ${CMAKE_SOURCE_DIR}/../libs/libressl/lib/libtls.so )
    LIST( APPEND LIBRARYS ${CMAKE_SOURCE_DIR}/../libs/libressl/lib/libcrypto.so )

 
# runtime search paths
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath=${CMAKE_SOURCE_DIR}/../libs/libressl/lib" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath=." )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath=${CMAKE_INSTALL_PREFIX}/share/copilot/lib" )

else()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_LTLS" )
endif()


option( PLUGIN_WS "Enable Websocket" OFF )
if(PLUGIN_WS)
	file( GLOB srcCopilotd ${srcCopilotd} ${srcUWebSockets} "../src/plugins/uwebsockets.cpp" )
	include_directories( "/usr/include/uWS" )
#	LIST( APPEND LIBRARYS uWS )
#	LIST( APPEND LIBRARYS ssl )
	#LIST( APPEND LIBRARYS crypto )
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
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dsyslogd_C" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dsyslogd_H" )
endif()

option( PLUGIN_DB "Enable Systemd" ON )
if(PLUGIN_DB)
    file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/eDB.cpp" )
    LIST( APPEND LIBRARYS sqlite3 )
else()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_EDB" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DeDB_C" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DeDB_H" )
    LIST( APPEND LIBRARYS sqlite3 )
endif()


option( PLUGIN_MDB "Enable MongoDB" ON )
if(PLUGIN_MDB)
    find_package( libmongoc-1.0 1.7 REQUIRED )
    
    message ("-- mongoc found version \"${MONGOC_VERSION}\"")
    message ("-- mongoc include path \"${MONGOC_INCLUDE_DIRS}\"")
    message ("-- mongoc libraries \"${MONGOC_LIBRARIES}\"")


    file( GLOB srcCopilotd ${srcCopilotd} "../src/plugins/mongodb.cpp" )
    LIST( APPEND INCDIRS ${MONGOC_INCLUDE_DIRS} )
    LIST( APPEND LIBRARYS ${MONGOC_LIBRARIES} )
else()
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dmongodb_H" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_MDB" )
endif()

# add main.cpp
file( GLOB srcCopilotd ${srcCopilotd}  "../src/main.cpp" )



include_directories( "../libs/uWebSockets/src" ) 
include_directories( "../libs/evillib/core" ) 
include_directories( "../libs/evillib/extra" )
include_directories( "../libs/util-linux/libuuid/src" )
include_directories( "../libs/portable-2.7.4/include" )


include_directories( "../src" )
include_directories( "../src/core" )


add_executable(copilotd ${srcEvillib} ${srcCopilotd} ${srclibressl} )
target_link_libraries( copilotd ${LIBRARYS} )
target_include_directories( copilotd PRIVATE ${INCDIRS} )
install( TARGETS copilotd RUNTIME DESTINATION bin )



execute_process( COMMAND git log -n 1 --pretty=format:"%h %ai"
                 OUTPUT_VARIABLE GITVERSION
)

FILE(WRITE ../src/gitversion.h
    "#define gitversion ${GITVERSION}"
)



set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pie -fPIE -fPIC -fstack-protector -Wformat -Wformat-security -DPACKAGE='\"copilotd\"' -DLOCALEDIR='\"/usr/share/locale\"' -DDISABLE_SSH")


add_dependencies( copilotd libressl )





target_link_libraries( copilotd z )
target_link_libraries( copilotd uuid )
target_link_libraries( copilotd dl )













