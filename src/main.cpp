/*
Copyright (C) 2017 by Martin Langlotz

This file is part of copilot.

copilot is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, version 3 of this License

copilot is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with copilot.  If not, see <http://www.gnu.org/licenses/>.

*/

/* test:
gnutls-cli --no-ca-verification \
--x509keyfile=/etc/copilot/services/ssl_my_keys/hacktop7.mnet.local-key.pem \
--x509certfile=/etc/copilot/services/ssl_my_keys/hacktop7.mnet.local-cert.pem  \
localhost:4567

{ "id":"", "s":"testnode", "t":"all", "g":"cocom", "c":"ping", "v": "" }

*/

#include "intl.h"

#include "jansson.h"
//#include "doDBDws.h"
//#include "wsPlugins/doDBDPluginList.h"
//#include "wsPlugins/doDBDConnections.h"
#include <getopt.h>

#include "core/etInit.h"

#include "core/pubsub.h"
#include "core/coCore.h"

// plugins
#include "core/plugin.h"
//#include "plugins/qwebsocket.h"


#include "plugins/coreService.h"







int main( int argc, char *argv[] ){

// locale
    setlocale (LC_ALL, "");
    bindtextdomain( "PACKAGE", "LOCALEDIR" );
    textdomain( "PACKAGE" );


    etInit(argc,(const char**)argv);
    etDebugLevelSet( etID_LEVEL_DETAIL_APP );




// create the core which contains all services
    coCore*         core = new coCore();

// we init our config
    coConfig*       config = new coConfig();

// we need the pubsub-stuff
	psBus*          bus = new psBus();


// parse options
    core->parseOpt( argc, argv );
    config->parseOpt( argc, argv );




// we load node name from config
    const char* myNodeName = NULL;
    config->load();
    myNodeName = config->nodeName();
// no node-name in config, we grab it from core
    if( myNodeName == NULL ){
        myNodeName = core->nodeName();
        config->nodeName( myNodeName );
        config->save();
    } else {
        core->nodeName( myNodeName );
    }
// because ssh-configuration is based on our hostname, we print it out for debugging
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "My nodename: %s", myNodeName );
    etDebugMessage( etID_LEVEL_INFO, etDebugTempMessage );




// load plugins
    loadPlugin( "lssl" );
    loadPlugin( "websocket" );

// init all plugins
    initAllPlugins( argc, argv );

// run all plugins
    runAllPlugins();

    while(1){
        sleep(10);
    }
    exit(1);


// because the most plugins run own threads we need a "busy" thread
    core->mainLoop();

// cleanup
    delete core;




    etDebugLevelSet( etID_LEVEL_CRITICAL );
    etDebugMessage( etID_LEVEL_WARNING, "Test" );
}



