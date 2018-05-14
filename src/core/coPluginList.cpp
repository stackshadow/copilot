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

#ifndef coPluginList_C
#define coPluginList_C

#include "coCore.h"
#include "coPluginList.h"

/** @greoupdef pluginlist coPluginList - handling all plugins
*/


coPluginList::				coPluginList(){
// plugin-list
    this->pluginListLock = 0;
    etListAlloc( this->pluginList );
    this->pluginListIterator = NULL;

// fifo
    this->messageQueue = new coMessageQueue2();



// test: add some messages
    this->messageQueue->add( NULL, "test", "test", "test", "test1", "" );
    this->messageQueue->add( NULL, "test", "test", "test", "test2", "" );
    this->messageQueue->add( NULL, "test", "test", "test", "test3", "" );




// start thread
    this->boradcastThreadStart();
    //this->broadcastWatchdogThreadStart();

}


coPluginList::				~coPluginList(){

// vars
    coPlugin*       source = NULL;

// remove all plugins
    this->iterate();
    while( this->next(&source) == true ){
        delete source;
    }
    this->iterateFinish();
// remove the list
    etListFree( this->pluginList );

// stop thread
    this->broadcastThreadRun = -1;
    while( broadcastThreadRun != 0 ){
        usleep( 500000 );
    }





}




bool coPluginList::      	append( coPlugin* plugin ){
    lockPthread( this->pluginListLock );

// check
    if( this->pluginList == NULL ) return false;

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Register Plugin: %s", plugin->name() );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// add it to the list
    etListAppend( this->pluginList, (void*)plugin );
    unlockPthread( this->pluginListLock );
	return true;
}


bool coPluginList::       	remove( coPlugin* plugin ){
    lockPthread( this->pluginListLock );

    if( this->pluginList == NULL ) return false;

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Remove Plugin: %s", plugin->name() );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

    etListDataRemove( this->pluginList, (void*)plugin, etID_TRUE );
    this->pluginListIterator = NULL;

    unlockPthread( this->pluginListLock );
	return true;
}




bool coPluginList::			iterate(){
    lockPthread( this->pluginListLock );

	this->pluginListIterator = NULL;
	if( etListIterate( this->pluginList, this->pluginListIterator ) == etID_YES ){
		return true;
	}
	
	return false;
}


bool coPluginList::			next( coPlugin** plugin ){

//
	if( this->pluginListIterator == NULL ) return false;

// get plugin
	if( __etListIterateNext( &this->pluginListIterator, (void**)plugin ) == etID_YES ){
		return true;
	}

	return false;
}


bool coPluginList::         iterateFinish(){
    unlockPthread( this->pluginListLock );
	return true;
}





void coPluginList::         boradcastThreadStart(){

// set thread to running
    this->broadcastThreadRun = 1;

// start the thread which wait for clients
    pthread_create( &this->broadcastThread_i, NULL, coPluginList::broadcastThread, this );
    pthread_setname_np( this->broadcastThread_i, "broadcast\0" );
    pthread_detach( this->broadcastThread_i );


}

/**
 * @brief Handle all open messages
 * This function handles all open broadcast messages
 * @param userdata void-pointer to an coPluginList pointer
 */
void* coPluginList::        broadcastThread( void* userdata ){

// vars
    coPluginList*       pluginList = (coPluginList*)userdata;
    coMessage*          message = NULL;
	coPlugin*		    tempPlugin;
    const char*         msgNodeNameSource = NULL;
	const char*		    msgNodeNameTarget = NULL;
	const char*		    msgGroup = NULL;
	const char*		    msgCommand = NULL;

    while( pluginList->broadcastThreadRun == 1 ){

        if( pluginList->messageQueue->get( &message ) == true ){
            msgNodeNameSource = message->nodeNameSource();
            msgNodeNameTarget = message->nodeNameTarget();
            msgGroup = message->group();
            msgCommand = message->command();

        // iterate
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "MSG FROM %s TO %s/%s/%s", msgNodeNameSource, msgNodeNameTarget, msgGroup, msgCommand );
            etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );


        // iterate through the list
            pluginList->iterate();
            while( pluginList->next(&tempPlugin) == true ){

            // is null ?
                if( tempPlugin == NULL ) continue;

            // we dont ask the source
                if( tempPlugin == message->source() ) continue;

            // check if plugin accepts the hostname/group
                if( tempPlugin->filterCheck( msgNodeNameTarget, msgGroup ) == false ) continue;

            // call plugin-function
                coPlugin::t_state returnState = tempPlugin->onBroadcastMessage( message );

            // ping if needed
                if( pluginList->boradcastThreadPing == true ){ pluginList->boradcastThreadPing = false; }

                if( returnState == coPlugin::MESSAGE_FINISHED ) break;

                usleep( 1000L );
            }
            pluginList->iterateFinish();
            pluginList->messageQueue->release();

        // free message memory
            delete message;

        } else {
        // ping if needed
            if( pluginList->boradcastThreadPing == true ){ pluginList->boradcastThreadPing = false; }
            usleep( 50000L );
        }

    }

// set to 0
    pluginList->broadcastThreadRun = 0;
    return NULL;
}


void coPluginList::         broadcastWatchdogThreadStart(){


// start the watchdog
    pthread_t threadWatchdog;
    pthread_create( &threadWatchdog, NULL, coPluginList::broadcastWatchdogThread, this );
    pthread_setname_np( threadWatchdog, "broadcastWD\0" );
    pthread_detach( threadWatchdog );

}


void* coPluginList::        broadcastWatchdogThread( void* userdata ){

    // vars
    coPluginList*       pluginList = (coPluginList*)userdata;

    while( pluginList->broadcastThreadRun == 1 ){

    // we set the ping and wait for response
        pluginList->boradcastThreadPing = true;
        sleep(5);

    // now the broadcast thread should set the ping to false
        if( pluginList->boradcastThreadPing == true ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "broadcast thread seems to hang... kill and restart it" );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

        // exit the running thread
            pthread_cancel( pluginList->broadcastThread_i );

        // start the thread which wait for clients
            pthread_create( &pluginList->broadcastThread_i, NULL, coPluginList::broadcastThread, pluginList );
            pthread_setname_np( pluginList->broadcastThread_i, "broadcastWD\0" );
            pthread_detach( pluginList->broadcastThread_i );


        }

    }

	return NULL;
}


/*

void coPluginList::         broadcast( coPlugin* source, coMessage* message ){
// from here the plugins start
    if( source == NULL ) return;
    if( message == NULL ) return;


// vars
	coPlugin*				tempPlugin;
    const char*             pluginHostName = NULL;
    int                     pluginHostNameLen = 0;
    const char*             pluginGroup = NULL;
    int                     pluginGroupLen = 0;
	coPlugin::t_state		pluginState = coPlugin::BREAK;
    int                     cmpResult = -1;

	const char*				msgHostName = message->hostName();
	const char*				msgGroup = message->group();
	const char*				msgCommand = message->command();


// iterate through the list
    this->iterate();
    while( this->next(&tempPlugin) == true ){

	// is null ?
		if( tempPlugin == NULL ) continue;

    // we dont ask the source
        if( tempPlugin == source ) continue;

	// check if plugin accepts the hostname/group
		if( tempPlugin->filterCheck( msgHostName, msgGroup ) == false ) continue;

    // iterate
		snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s/%s/%s", msgHostName, msgGroup, msgCommand );
		etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

	// clear the reply
		message->clearReply();

	// send it to the plugin and wait for response
		snprintf( etDebugTempMessage, etDebugTempMessageLen, "CALL %s::onBroadcastMessage()", tempPlugin->name() );
		etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

	// call plugin-function
		pluginState = tempPlugin->onBroadcastMessage( message );

	// plugin dont reply something
		if( pluginState == coPlugin::NO_REPLY ) continue;

	// plugin want to break
		if( pluginState == coPlugin::BREAK ) {
			snprintf( etDebugTempMessage, etDebugTempMessageLen, "Plugin '%s' requests break", tempPlugin->name() );
			etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
			break;
		}

	// reply back to plugin
		source->onBroadcastReply( message );

    }
    this->iterateFinish();

}
*/

void coPluginList::         setupAll(){
// from here the plugins start
	if( coCore::setupMode != true ) return;

// vars
	coPlugin*				tempPlugin;

// iterate through the list
    this->iterate();
    while( this->next(&tempPlugin) == true ){

    // iterate
        if( tempPlugin != NULL ){
			tempPlugin->onSetup();
		}

    }
    this->iterateFinish();


}


void coPluginList::         executeAll(){

// from here the plugins start
	if( coCore::setupMode == true ) return;

// vars
	coPlugin*				tempPlugin;

// iterate through the list
    this->iterate();
    while( this->next(&tempPlugin) == true ){

    // iterate
        if( tempPlugin != NULL ){
			tempPlugin->onExecute();
		}

    }
    this->iterateFinish();


}





#endif