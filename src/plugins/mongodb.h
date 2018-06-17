/*
Copyright (C) 2018 by Martin Langlotz

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


#ifndef mongodb_H
#define mongodb_H

#include <iostream>
#include <string>

#include <mongoc.h>



class mdbClient {
    
    private:
        mongoc_client_t*        client = NULL;
        mongoc_database_t*      database = NULL;
        mongoc_collection_t*    collTemplate = NULL;
        bool                    connected = false;

// public functions
	public:
                                mdbClient( std::string hostname, int port, const char* dbname, std::string* errorMessage );
                                ~mdbClient();
        bool                    isConnected();    
        bool                    jsonAdd( const char* jsonString, std::string* errorMessage );
        bool                    queryToJson( bson_t* filter, bson_t* opts, json_t* jsonArray );
        
};

class mongodb {

    private:
        json_t*                 jsonConfig = NULL;
        mdbClient*              client;

// public functions
	public:
                                mongodb();
                                ~mongodb();
        void                    config( const char** hostName, int* port, const char** db );

    static int                  onSubscriberMessage( 
                                    const char* id, 
                                    const char* nodeSource, 
                                    const char* nodeTarget, 
                                    const char* group, 
                                    const char* command, 
                                    const char* payload, 
                                    void* userdata 
                                );

};





#endif