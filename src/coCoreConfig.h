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

#include <sys/syscall.h>
#include <sys/types.h>

#ifndef baseFilePath
#define baseFilePath "/etc/copilot/services/"
#endif

#define lockID pid_t

#define lockMyPthread() \
	pid_t myThreadTIDLock = syscall(SYS_gettid); \
	while( this->threadLock != myThreadTIDLock && this->threadLock != 0 ){ \
		usleep( 10000 ); \
	} \
	this->threadLock = myThreadTIDLock; \

#define unlockMyPthread() \
	pid_t myThreadTIDUnLock = syscall(SYS_gettid); \
	while( this->threadLock != myThreadTIDUnLock && this->threadLock != 0 ){ \
		usleep( 10000 ); \
	} \
	this->threadLock = 0; \



#define lockPthread( threadLock ) \
	pid_t myThreadTIDLock = syscall(SYS_gettid); \
	while( threadLock != myThreadTIDLock && threadLock != 0 ){ \
		usleep( 10000 ); \
	} \
	threadLock = myThreadTIDLock; \

#define unlockPthread( threadLock ) \
	pid_t myThreadTIDUnLock = syscall(SYS_gettid); \
	while( threadLock != myThreadTIDUnLock && threadLock != 0 ){ \
		usleep( 10000 ); \
	} \
	threadLock = 0; \


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
		json_t*				jsonConfig = NULL;
		json_t*				jsonNodes = NULL;
		void*				jsonNodesIterator;
		json_t*				jsonNode = NULL;

public:
		coCoreConfig();
		~coCoreConfig();

	// config
		bool				load();
		bool				save( const char* jsonString = NULL );

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
