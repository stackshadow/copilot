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

#ifndef doDBDPlugin_C
#define doDBDPlugin_C


#include "coPlugin.h"


coPlugin::                          coPlugin( const char* name, const char* onlyTargetNodeName, const char* listenGroup ){

// alloc
    etStringAllocLen( this->pluginName, 128 );
    etStringAllocLen( this->pluginInfo, 128 );

// alloc private stuff
    etStringAllocLen( this->targetNode, 128 );
    etStringAllocLen( this->targetGroup, 128 );

// save
    etStringCharSet( this->pluginName, name, -1 );
	etStringCharSet( this->targetNode, onlyTargetNodeName, -1 );
	etStringCharSet( this->targetGroup, listenGroup, -1 );
}


coPlugin::                          ~coPlugin(){
    etStringFree( this->pluginName );
    etStringFree( this->pluginInfo );

    etStringFree( this->targetNode );
    etStringFree( this->targetGroup );
}



const char* coPlugin::              name(){
    const char *tempCharArray = NULL;
    etStringCharGet( this->pluginName, tempCharArray );
    return tempCharArray;
}


void coPlugin::						setName( const char* name ){
    etStringCharSet( this->pluginName, name, -1 );
}


bool coPlugin::                     info( const char* shortInfo ){
    etStringCharSet( this->pluginInfo, shortInfo, -1 );
}


const char* coPlugin::              info(){
    const char* returnChar = NULL;
    etStringCharGet( this->pluginInfo, returnChar );
    return returnChar;
}


bool coPlugin::						filterCheck( const char* hostName, const char* group ){

// vars
	const char* 	tempCharArray = NULL;
	size_t			tempLen = 0;
	int				tempCmpResult = -1;

// hostname
	if( hostName == NULL ) return false;
	if( group == NULL ) return false;

// compare node
	etStringCharGet( this->targetNode, tempCharArray );
	tempLen = strlen(tempCharArray);
	tempCmpResult = strncmp( hostName, tempCharArray, tempLen );
	if( tempCmpResult != 0 && tempLen > 0 ){
		return false;
	}

// group
	etStringCharGet( this->targetGroup, tempCharArray );
	tempLen = strlen(tempCharArray);
	tempCmpResult = strncmp( group, tempCharArray, tempLen );
	if( tempCmpResult != 0 && tempLen > 0 ){
		return false;
	}

// return
	return true;
}






#endif
