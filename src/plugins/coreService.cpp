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

#ifndef authService_C
#define authService_C

#include "coCore.h"
#include "plugins/coreService.h"
#include "version.h"

#include <string>

coreService::                   coreService() : coPlugin( "core" ) {



// register this plugin
    coCore::ptr->registerPlugin( this, "", "co" );

}


coreService::                   ~coreService(){

}


bool coreService::              onBroadcastMessage( coMessage* message ){

// vars
	const char*			msgHostName = message->hostName();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
	const char*			msgPayload = message->payload();

// to all hosts
    if( strncmp( (char*)msgHostName, "all", 3 ) == 0 ){

    // ping
        if( strncmp(msgCommand,"ping",4) == 0 ){

			message->hostName( coCore::ptr->hostInfo.nodename );
			message->replyCommand( "pong" );

            return true;
        }
    }


// to "localhost" or to the nodename-host
    if( strncmp("localhost",msgHostName,9) != 0 &&
	strncmp(coCore::ptr->hostInfo.nodename,msgHostName,coCore::ptr->hostNodeNameLen) != 0 ){
        return true;
    }


    if( strncmp( (char*)msgCommand, "getVersion", 10 ) == 0 ){

		message->replyCommand( "version" );
		message->replyPayload( copilotVersion );

        return true;
    }


    if( strncmp( (char*)msgCommand, "getServices", 11 ) == 0 ){
        return true;
    }


    return true;
}






#endif // ldapService_C
