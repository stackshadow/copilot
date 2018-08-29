cmake_minimum_required(VERSION 2.6)

# clean
set( LIBRARY_NAME websocket )
set( srcPlugin "" )
set( libPlugin "" )



# libretls
option( PLUGIN_WEBSOCKET "Enable Websocket" OFF )
if(PLUGIN_WEBSOCKET)

# websocketlib
    file( GLOB srcWebsockets "${COPILOTD_LIBS}/uWebSockets/src/*.cpp" )

# sources
    LIST( APPEND srcPlugin "${COPILOTD_SRC_PLUGINS}/websocket/uwebsockets.cpp" )
    LIST( APPEND srcPlugin ${srcWebsockets} )

# target
    add_library(
        ${LIBRARY_NAME}
        MODULE
        ${srcPlugin}
    )

# link runtime dir for testing
if(DEVELOP)
    add_custom_command(
        COMMAND mkdir -p ${COPILOTD_SRC}/../develop_runtime
        COMMAND ln -fs ${COPILOTD_SRC_PLUGINS}/websocket/lib${LIBRARY_NAME}.so ${COPILOTD_SRC}/../develop_runtime/${LIBRARY_NAME}.so
        TARGET ${LIBRARY_NAME}
    )
endif()

# compile
    target_compile_options( ${LIBRARY_NAME}
        PUBLIC "-Wl,-rpath=."
        PUBLIC "-Wl,-rpath=${CMAKE_INSTALL_PREFIX}/share/copilot/lib"
        PUBLIC "-g"
        PUBLIC "-pie"
        PUBLIC "-fPIE"
        PUBLIC "-fPIC"
        PUBLIC "-fstack-protector"
        PUBLIC "-Wformat"
    )

# link
    LIST( APPEND libPlugin ${COPILOTD_LIBS}/libressl/lib/libssl.so )
    LIST( APPEND libPlugin ${COPILOTD_LIBS}/libressl/lib/libtls.so )
    LIST( APPEND libPlugin ${COPILOTD_LIBS}/libressl/lib/libcrypto.so )
    LIST( APPEND libPlugin jansson )
    target_link_libraries( ${LIBRARY_NAME} ${libPlugin} )



endif()




