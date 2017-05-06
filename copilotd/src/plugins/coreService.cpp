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


bool coreService::              onMessage(  const char*     msgHostName, 
                                            const char*     msgGroup, 
                                            const char*     msgCommand, 
                                            const char*     msgPayload, 
                                            json_t*         jsonAnswerObject ){




// to all hosts
    if( strncmp( (char*)msgHostName, "all", 3 ) == 0 ){
        
    // ping
        if( strncmp(msgCommand,"ping",4) == 0 ){
            
            std::string tempString = "nodes/";
            tempString += coCore::ptr->hostInfo.nodename;
            tempString += "/co/pong";

            json_object_set_new( jsonAnswerObject, "topic", json_string("pong") );
            json_object_set_new( jsonAnswerObject, "payload", json_string("") );
            
            return true;
        }
    }

// to "localhost" or to the nodename-host
    if( strncmp("localhost",msgHostName,9) != 0 &&
    strncmp(coCore::ptr->hostInfo.nodename,msgHostName,coCore::ptr->hostNodeNameLen) != 0 ){
        return true;
    }


    if( strncmp( (char*)msgCommand, "getVersion", 10 ) == 0 ){
        
        json_object_set_new( jsonAnswerObject, "topic", json_string("version") );
        json_object_set_new( jsonAnswerObject, "payload", json_string( copilotVersion ) );
        
        return true;
    }


    if( strncmp( (char*)msgCommand, "getServices", 11 ) == 0 ){

        json_t* jsonArray = json_array();
        coCore::ptr->listPlugins( jsonArray );


        json_object_set_new( jsonAnswerObject, "topic", json_string("services") );
        json_object_set_new( jsonAnswerObject, "payload", jsonArray );


        return true;
    }


    return true;
}






#endif // ldapService_C
