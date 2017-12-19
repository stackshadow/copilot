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

#ifndef coMessageQueue2_C
#define coMessageQueue2_C


#include "coMessageQueue2.h"


coMessageQueue2::                coMessageQueue2(){

    this->messageFiFoLock = 0;

    etListAlloc( this->list );
}


coMessageQueue2::                ~coMessageQueue2(){

// vars
    coMessage*      message = NULL;
    void*           listIterator = NULL;

// iterate from beginning
    etListIterate( this->list, listIterator );
    if( etListIterateNext( listIterator, message ) != etID_YES ){
        delete message;
    }

    etListFree( this->list );

}




bool coMessageQueue2::           add( coPlugin* sourcePlugin, coMessage* message ){

// Lock
    lockPthread( this->messageFiFoLock );

// add it to the list
    etListAppend( this->list, (void*)message );

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen,
    "[APPEND] [%s -> %s] [%s - %s]",
    message->nodeNameSource(), message->nodeNameTarget(),
    message->group(), message->command() );
    etDebugMessage( etID_LEVEL_DETAIL_NET, etDebugTempMessage );

// UnLock
    unlockPthread( this->messageFiFoLock );
    return true;
}


bool coMessageQueue2::           add(   coPlugin*   sourcePlugin,
                                        const char* nodeNameSource,
                                        const char* nodeNameTarget,
                                        const char* group,
                                        const char* command,
                                        const char* payload ){


// vars
    coMessage*      message = NULL;

// set message
    message = new coMessage();
    message->source( sourcePlugin );
    message->nodeNameSource( nodeNameSource );
    message->nodeNameTarget( nodeNameTarget );
    message->group( group );
    message->command( command );
    message->payload( payload );

// add it to the list
    return this->add( sourcePlugin, message );
}


bool coMessageQueue2::           get( coMessage** p_message ){

// vars
    coMessage*      message = NULL;
    void*           listIterator = NULL;

// lock
    lockPthread( this->messageFiFoLock );

// iterate from beginning
    etListIterate( this->list, listIterator );
    if( listIterator == NULL ){
        unlockPthread( this->messageFiFoLock );
        return false;
    }

// try to get first element in the list
    if( etListIterateGet( listIterator, message ) != etID_YES ){
        unlockPthread( this->messageFiFoLock );
        return false;
    }

// get message
    *p_message = message;

// remove it from the list
    etListIterateRemove( this->list, listIterator );

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "%p: message aviable", message );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

    unlockPthread( this->messageFiFoLock );
    return true;
}

/**
@brief Release the actual message and move the read index to the next message
@return always true
*/
bool coMessageQueue2::           release(){
    return true;
}











#endif