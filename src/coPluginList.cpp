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


coPluginList::				coPluginList(){
// plugin-list
    this->threadLock = 0;
    etListAlloc( this->pluginList );
    this->pluginListIterator = NULL;
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

}




bool coPluginList::      	append( coPlugin* plugin ){
    lockMyPthread();

// check
    if( this->pluginList == NULL ) return false;

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Register Plugin: %s", plugin->name() );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// add it to the list
    etListAppend( this->pluginList, (void*)plugin );

    unlockMyPthread();
	return true;
}


bool coPluginList::       	remove( coPlugin* plugin ){


}




bool coPluginList::			iterate(){
    lockMyPthread();

	this->pluginListIterator = NULL;
	etListIterate( this->pluginList, this->pluginListIterator );
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
    unlockMyPthread();
}




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