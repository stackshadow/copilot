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

#include "coCore.h"
#include "coPlugin.h"


coPlugin::                          coPlugin( const char* name ){

// alloc
    etStringAllocLen( this->pluginName, 128 );
    etStringAllocLen( this->pluginInfo, 128 );

// save
    etStringCharSet( this->pluginName, name, -1 );
}


coPlugin::                          ~coPlugin(){
    etStringFree( this->pluginName );
    etStringFree( this->pluginInfo );
}



const char* coPlugin::              name(){
    const char *tempCharArray = NULL;
    etStringCharGet( this->pluginName, tempCharArray );
    return tempCharArray;
}


bool coPlugin::                     info( const char *shortInfo ){
    etStringCharSet( this->pluginInfo, shortInfo, -1 );
}


const char* coPlugin::              info(){
    const char* returnChar = NULL;
    etStringCharGet( this->pluginInfo, returnChar );
    return returnChar;
}



#endif
