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



#include "jansson.h"
//#include "doDBDws.h"
//#include "wsPlugins/doDBDPluginList.h"
//#include "wsPlugins/doDBDConnections.h"

#include "core/etInit.h"

#include "coCore.h"

// plugins
//#include "plugins/qwebsocket.h"
#include "plugins/websocket.h"
#include "plugins/coreService.h"
#include "plugins/nftService.h"
#include "plugins/ldapService.h"
#include "plugins/mqttService.h"
#include "plugins/ldapService.h"

//#include <QtCore/QCoreApplication>

int main( int argc, char *argv[] ){

    etInit(argc,(const char**)argv);
//    QCoreApplication a(argc, argv);

// create the core which contains all services
    coCore*         newcoCore = new coCore();
    
// create services
    coreService*    newCoreService = new coreService();

/** @todo Here are memory leaks ! */
#ifndef DISABLE_MQTT
    mqttService*    mqqtPlugin = new mqttService();
#endif
    
#ifndef DISABLE_WEBSOCKET
    websocket*      wsPlugin = new websocket( 3000 );
#endif

#ifndef DISABLE_NFT
    nftService*     newNftService = new nftService();
#endif

#ifndef DISABLE_LDAP
    ldapService*    newLdapService = new ldapService();
#endif


    //return a.exec();
    newcoCore->mainLoop();

// cleanup
    delete newcoCore;

    etDebugMessage( etID_LEVEL_WARNING, "Test" );
}



