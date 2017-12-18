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

#ifndef coCoreConfig_H
#define coCoreConfig_H


#include <unistd.h>
#include <sys/utsname.h>
#include <string>
#include <pthread.h>
#include "semaphore.h"

#include "evillib_depends.h"
#include "memory/etList.h"
#include "string/etString.h"
#include "string/etStringChar.h"

#include "jansson.h"

#include "lockPthread.h"




class coCoreConfig {


// types
	public:
		typedef enum {
			UNKNOWN = 0,
			SERVER = 1,
			CLIENT = 10,
            CLIENT_IN = 11,
		} nodeType;



// public values
	private:
		lockID				threadLock;

	// settings
        etString*           configBasePath = NULL;
		json_t*				jsonConfig = NULL;
		json_t*				jsonNodes = NULL;
		void*				jsonNodesIterator = NULL;
		json_t*				jsonNode = NULL;

public:
		coCoreConfig();
		~coCoreConfig();

	// config
		bool				load( const char* myNodeName );
		bool				save( const char* jsonString = NULL );
        json_t*             section( const char* sectionName );

    // core config
        bool                configPath( const char** path );

	// nodes
		bool				nodesGet( json_t** jsonObject );
		bool				nodesGetAsArray( json_t* jsonArray );
        void                nodeStatesRemove();

	// iterate nodes / get node-infos
		bool				nodesIterate();
		bool				nodeAppend( const char* name );
        bool                nodeRemove( const char* name );
		bool				nodeSelect( const char* name );
        bool                nodeSelectByHostName( const char* hostName );
		bool				nodeNext();
		bool				nodeInfo( const char** name, coCoreConfig::nodeType* type, bool set = false );
		bool				nodeConnInfo( const char** host, int* port, bool set = false );
		bool				nodesIterateFinish();

    // user / password
        bool                authMethode();
        bool                userAdd( const char* username );
        bool                userCheck( const char* username, const char* password );





};


#endif
