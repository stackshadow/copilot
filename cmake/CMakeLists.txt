cmake_minimum_required(VERSION 2.6)
project(copilotd)


# some variables we need
execute_process( COMMAND nproc OUTPUT_VARIABLE NCPUS )
set( COPILOTD_SRC "${CMAKE_SOURCE_DIR}/../src" )
set( COPILOTD_SRC_PLUGINS "${COPILOTD_SRC}/plugins" )
set( COPILOTD_LIBS "${CMAKE_SOURCE_DIR}/../libs" )



include_directories( "../libs/uWebSockets/src" )
include_directories( "../libs/evillib/core" )
include_directories( "../libs/evillib/extra" )
include_directories( "../libs/util-linux/libuuid/src" )
include_directories( "../libs/portable-2.7.4/include" )


include_directories( "${COPILOTD_SRC}" )
include_directories( "${COPILOTD_SRC}/core" )
include_directories( "${COPILOTD_SRC}/plugins" )

# plugins
add_subdirectory( "${COPILOTD_SRC_PLUGINS}/libretls" "${COPILOTD_SRC_PLUGINS}/libretls" )
add_subdirectory( "${COPILOTD_SRC_PLUGINS}/sysState" "${COPILOTD_SRC_PLUGINS}/sysState" )
add_subdirectory( "${COPILOTD_SRC_PLUGINS}/websocket" "${COPILOTD_SRC_PLUGINS}/websocket" )

# EXCLUDE_FROM_ALL



# submodules
add_custom_command(
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    COMMAND git submodule update
    OUTPUT
    ${CMAKE_SOURCE_DIR}/../libs/evillib/make/Makefile
    ${CMAKE_SOURCE_DIR}/../libs/util-linux/README
    ${CMAKE_SOURCE_DIR}/../libs/uWebSockets/Makefile
)
add_custom_target( submodules
    DEPENDS
    ${CMAKE_SOURCE_DIR}/../libs/evillib/make/Makefile
    ${CMAKE_SOURCE_DIR}/../libs/util-linux/README
    ${CMAKE_SOURCE_DIR}/../libs/uWebSockets/Makefile
)
add_custom_target( clean_submodules )
add_custom_command(
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    COMMAND git submodule foreach git clean -d -x -f
    TARGET clean_submodules
)

# sources - evillib
file( GLOB srcEvillib   "../src/evillib.c" )

# sources - copilotd
file( GLOB srcCopilotd
    "../src/plugins/coreService.cpp"
    "../src/core/*.c"
    "../src/core/*.cpp"
    "../src/main.cpp"
)

# sources - uwebsocket
file( GLOB srcUWebSockets
    "../libs/uWebSockets/src/*.cpp"
)

# create gitversion
execute_process( COMMAND git log -n 1 --pretty=format:"%h %ai" OUTPUT_VARIABLE GITVERSION )
FILE( WRITE ../src/gitversion.h "#define gitversion ${GITVERSION}" )


# This we need
LIST( APPEND LIBRARYS sodium )
LIST( APPEND LIBRARYS jansson )
LIST( APPEND LIBRARYS pthread )


# the target
add_executable( copilotd ${srcEvillib} ${srcCopilotd} )
add_dependencies( copilotd libressl submodules )


# compile
if(DEVELOP)
    target_compile_options( copilotd
        PUBLIC "-Wl,-rpath=."
        PUBLIC "-Wl,-rpath=${CMAKE_INSTALL_PREFIX}/share/copilot/lib"
        PUBLIC "-g"
        PUBLIC "-pie"
        PUBLIC "-fPIE"
        PUBLIC "-fPIC"
        PUBLIC "-fstack-protector"
        PUBLIC "-Wformat"
        PUBLIC "-DPACKAGE=copilotd"
        PUBLIC "-DLOCALEDIR=/usr/share/locale"
        PUBLIC "-DPLUGINPATH=\"${COPILOTD_SRC}/../develop_runtime\""
        PUBLIC "-DDEVELOP=1"
    )
else()
    target_compile_options( copilotd
        PUBLIC "-Wl,-rpath=."
        PUBLIC "-Wl,-rpath=${CMAKE_INSTALL_PREFIX}/share/copilot/lib"
        PUBLIC "-pie"
        PUBLIC "-fPIE"
        PUBLIC "-fPIC"
        PUBLIC "-fstack-protector"
        PUBLIC "-Wformat"
        PUBLIC "-DPACKAGE=copilotd"
        PUBLIC "-DLOCALEDIR=/usr/share/locale"
        PUBLIC "-DPLUGINPATH=\"${CMAKE_INSTALL_PREFIX}/share/copilot/plugins\""
    )
endif()

# link
target_link_libraries( copilotd ${LIBRARYS} )
target_link_libraries( copilotd z )
target_link_libraries( copilotd uuid )
target_link_libraries( copilotd dl )

target_include_directories( copilotd PRIVATE ${INCDIRS} )

# install
install( TARGETS copilotd RUNTIME DESTINATION bin )







add_custom_target( clean_cmake )
add_custom_command(
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    COMMAND find . -type f -wholename "*/CMakeFiles/*" -delete
    TARGET clean_cmake
)








