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

#ifndef doCore_H
#define doCore_H

#include <unistd.h>
#include <sys/utsname.h>
#include <string>
#include <pthread.h>
#include "semaphore.h"

#include "evillib_depends.h"
#include "memory/etList.h"
#include "string/etString.h"
#include "string/etStringChar.h"


#include "coMessage.h"
#include "coPluginElement.h"



#define baseFilePath /etc/copilot/services/

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define configFile(a) STR(baseFilePath) a




class coCore {

// types
	public:
		typedef enum {
			UNKNOWN = 0,
			SERVER = 1,
			CLIENT = 10
		} nodeType;

// public values
	private:
		struct utsname      hostInfo;
		int                 hostNodeNameLen;

	// settings
		sem_t 				mutex;
		json_t*				jsonConfig = NULL;
		json_t*				jsonNodes = NULL;
		void*				jsonNodesIterator;
		json_t*				jsonNode = NULL;

	// plugins
		etList*             pluginList;
		void*               iterator;
		json_t*             jsonAnswerArray;

	// lock
		pthread_mutex_t 	broadcastRunning = PTHREAD_MUTEX_INITIALIZER;
		coMessage*			broadcastMessage;

	public:
							coCore();
							~coCore();
		static coCore*  	ptr;

	public:

	// set / get
		void            	setHostName( const char* hostname );
		bool				hostNameGet( const char** hostName, int* hostNameChars );
		bool				hostNameAppend( etString* string );
		bool				isHostName( const char* hostNameToCheck );

	// config
		bool				configLoad();
		bool				configSave( const char* jsonString );
	// nodes
		bool				nodesGet( json_t** jsonObject );
		bool				nodesArrayGet( json_t* jsonArray );
		bool				nodesIterate();
		bool				nodeNext( const char** name, coCore::nodeType* type, bool set = false );
		bool				nodeConnInfo( const char** host, int* port );
		bool				nodesIterateFinish();


	// functions to work with the list
		bool        	    registerPlugin( coPlugin* plugin, const char *hostName, const char *group );
		bool       	 	    removePlugin( coPlugin* plugin );
		void        	    listPlugins( json_t* pluginNameArray );


	// helper functions
		static bool  		jsonValue( json_t* jsonObject, const char* key, char* value, int valueMax, const char* defaultValue, bool toJson );
		static bool  		jsonValue( json_t* jsonObject, const char* key, std::string* value, const char* defaultValue, bool toJson );

	// login / auth / permissions
		static bool  		passwordCheck( const char* user, const char* pass );
		static bool			passwordChange( const char* user, const char* oldpw, const char* newpw );

	// the main loop
		void				mainLoop();

	private:
		bool       			pluginsIterate();
		bool    	        pluginNext( coPluginElement** pluginElement );
		bool     	       	pluginNext( coPlugin** plugin );
		bool     	       	pluginElementGet( coPlugin* plugin, coPluginElement** pluginElement );
		bool     	       	nextAviable();

	// some helper stuff
		static bool     	setTopic( coPluginElement* pluginElement, json_t* jsonAnswerObject, const char* msgGroup );


	public:
		void            	broadcast(
								coPlugin*       source,
								const char* 	msgID,
								const char*     msgHostName,
								const char*     msgGroup,
								const char*     msgCommand,
								const char*     msgPayload
							);






};



#endif

