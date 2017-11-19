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

#ifndef coPluginList_H
#define coPluginList_H


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


#include "coPlugin.h"
#include "coCoreConfig.h"
#include "coMessageQueue.h"
#include "coMessageQueue2.h"


class coPluginList {

	private:
	// plugins
        lockID				pluginListLock;
		etList*             pluginList;
		void*				pluginListIterator;

        pthread_t           broadcastThread_i;
        int                 broadcastThreadRun = 0;
        bool                boradcastThreadPing = false;

    public:
        coMessageQueue2*     messageQueue;



	public:
		coPluginList();
		~coPluginList();

	// add / remove
		bool        	    append( coPlugin* plugin );
		bool       	 	    remove( coPlugin* plugin );

	// list
		bool       			iterate();
		bool    	        next( coPlugin** plugin );
        bool                iterateFinish();


    // API
        void                boradcastThreadStart();
        static void*        broadcastThread( void* userdata );
        void                broadcastWatchdogThreadStart();
        static void*        broadcastWatchdogThread( void* userdata );

        //void                broadcast( coPlugin* source, coMessage* message );
        void                setupAll();
        void                executeAll();

};



#endif