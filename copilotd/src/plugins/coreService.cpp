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
                                            json_t*         jsonData, 
                                            json_t*         jsonAnswerObject ){




    if( strncmp( (char*)msgCommand, "login", 5 ) == 0 ){

        json_object_set_new( jsonAnswerObject, "topic", json_string("loginok") );
        json_object_set_new( jsonAnswerObject, "payload", json_string("") );

        return true;
    }


    if( strncmp( (char*)msgCommand, "plistget", 8 ) == 0 ){

        json_t* jsonArray = json_array();
        coCore::ptr->listPlugins( jsonArray );


        json_object_set_new( jsonAnswerObject, "topic", json_string("plist") );
        json_object_set_new( jsonAnswerObject, "payload", jsonArray );


        return true;
    }



}






#endif // ldapService_C
