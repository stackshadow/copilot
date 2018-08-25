cmake_minimum_required(VERSION 2.6)

# clean
set( LIBRARY_NAME "" )
set( srcPlugin "" )
set( libPlugin "" )


set( LIBRARY_NAME lssl )


# libretls
option( PLUGIN_LTLS "Enable SSL" ON )
if(PLUGIN_LTLS)

    add_custom_command(
        COMMAND git clone --depth 1 https://github.com/libressl-portable/portable.git ${COPILOTD_LIBS}/libressl-portable-src
        OUTPUT ${COPILOTD_LIBS}/libressl-portable-src/autogen.sh
    )
    add_custom_command(
        DEPENDS ${COPILOTD_LIBS}/libressl-portable-src/autogen.sh
        WORKING_DIRECTORY ${COPILOTD_LIBS}/libressl-portable-src
        COMMAND bash autogen.sh
        OUTPUT ${COPILOTD_LIBS}/libressl-portable-src/configure
    )
    add_custom_command(
        DEPENDS ${COPILOTD_LIBS}/libressl-portable-src/configure
        WORKING_DIRECTORY ${COPILOTD_LIBS}/libressl-portable-src
        COMMAND ./configure --prefix ${COPILOTD_LIBS}/libressl
        OUTPUT ${COPILOTD_LIBS}/libressl-portable-src/Makefile
    )
    add_custom_command(
        DEPENDS ${COPILOTD_LIBS}/libressl-portable-src/Makefile
        WORKING_DIRECTORY ${COPILOTD_LIBS}/libressl-portable-src
        COMMAND make -j${NCPUS}
        COMMAND make install
        OUTPUT ${COPILOTD_LIBS}/libressl/lib/libtls.so
    )

    add_custom_target( libressl
        DEPENDS ${COPILOTD_LIBS}/libressl/lib/libtls.so
    )

# install
    file( GLOB libreSSLSharedObjects "${COPILOTD_LIBS}/libressl/lib/*.so*" )
    install( FILES ${libreSSLSharedObjects} DESTINATION share/copilot/lib COMPONENT runtime)


# sources
    LIST( APPEND srcPlugin "${COPILOTD_SRC_PLUGINS}/libretls/lsslSession.cpp" )
    LIST( APPEND srcPlugin "${COPILOTD_SRC_PLUGINS}/libretls/lsslService.cpp" )




# link copilot against this lib
    LIST( APPEND libPlugin argon2 )
    LIST( APPEND libPlugin ${COPILOTD_LIBS}/libressl/lib/libssl.so )
    LIST( APPEND libPlugin ${COPILOTD_LIBS}/libressl/lib/libtls.so )
    LIST( APPEND libPlugin ${COPILOTD_LIBS}/libressl/lib/libcrypto.so )
    LIST( APPEND libPlugin jansson )






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
        COMMAND ln -fs ${COPILOTD_SRC_PLUGINS}/libretls/liblssl.so ${COPILOTD_SRC}/../develop_runtime/lssl.so
        TARGET ${LIBRARY_NAME}
    )
endif()

# add libressl dependencie
    add_dependencies( ${LIBRARY_NAME} libressl  )

# compile
    target_compile_options( ${LIBRARY_NAME}
        PUBLIC "-Wl,-rpath=${COPILOTD_LIBS}/libressl/lib"
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
    target_link_libraries( ${LIBRARY_NAME} ${libPlugin} )


# install
    install( TARGETS ${LIBRARY_NAME}
        LIBRARY DESTINATION share/copilot/plugins
    )


endif()




