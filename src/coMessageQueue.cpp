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

#ifndef coMessageQueue_C
#define coMessageQueue_C


#include "coMessageQueue.h"


coMessageQueue::                coMessageQueue(){

    this->messageFiFoLock = 0;
    int index = 0;
    for( index = 0; index < queueLen; index++ ){
        this->messageFiFoUsed[index] = false;
        this->messageFiFo[index] = new coMessage;
    }

}


coMessageQueue::                ~coMessageQueue(){

// remove all fifo-stuff
    coMessage* message = NULL;
    int index = 0;
    for( index = 0; index < queueLen; index++ ){
        message = this->messageFiFo[index];
        delete message;
    }

}





bool coMessageQueue::           add(    coPlugin*   sourcePlugin,
                                        const char* nodeNameSource,
                                        const char* nodeNameTarget,
                                        const char* group,
                                        const char* command,
                                        const char* payload ){


// vars
    coMessage*      message = NULL;
    int             messageIndex = 0;
    int             messageNextIndex = 0;

// Lock

// Lock
    lockPthread( this->messageFiFoLock );

// try to find an written message
    while(1){

    // check if element is free
        if( this->messageFiFoUsed[messageIndex] == false ){
            break;
        }

    // iterate
        messageIndex++;

    // end of the fifo
        if( messageIndex >= queueLen ){
            messageIndex = 0;
        }

    // did we reach our start-element ?
        if( messageIndex == this->messageFiFoIndexReaded ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "FIFO FULL ( this should't happen ... ) please restart thist application." );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            unlockPthread( this->messageFiFoLock );
            return false;
        }

    }




// set message
    message = this->messageFiFo[messageIndex];
    message->source( sourcePlugin );
    message->nodeNameSource( nodeNameSource );
    message->nodeNameTarget( nodeNameTarget );
    message->group( group );
    message->command( command );
    message->payload( payload );
    this->messageFiFoUsed[messageIndex] = true;


    this->messageFiFoIndexWritten = messageIndex;

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "FIFO %i: append message '%s'", messageIndex, command );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// UnLock
    unlockPthread( this->messageFiFoLock );
    return true;
}


bool coMessageQueue::           get( coMessage** p_message ){

// vars
    coMessage*      message = NULL;
    int             messageIndex = this->messageFiFoIndexReaded;

// Lock
    lockPthread( this->messageFiFoLock );

// try to find an written message
    while(1){

    // check if element is free
        if( this->messageFiFoUsed[messageIndex] == true ){
            break;
        }

    // iterate
        messageIndex++;

    // end of the fifo
        if( messageIndex >= queueLen ){
            messageIndex = 0;
        }

    // did we reach our start-element ?
        if( messageIndex == this->messageFiFoIndexReaded ){
            unlockPthread( this->messageFiFoLock );
            return false;
        }

    }



// get message
    message = this->messageFiFo[messageIndex];
    *p_message = message;

// save readed position
    this->messageFiFoIndexReaded = messageIndex;


// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "FIFO %i: get", this->messageFiFoIndexReaded );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

    unlockPthread( this->messageFiFoLock );
    return true;
}

/**
@brief Release the actual message and move the read index to the next message
@return always true
*/
bool coMessageQueue::           release(){

// vars
    int             indexReadNext = this->messageFiFoIndexReaded+1;
    coMessage*      message = NULL;

// Lock
    lockPthread( this->messageFiFoLock );

// release actual mesage
    this->messageFiFoUsed[this->messageFiFoIndexReaded] = false;

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "FIFO %i: release message", this->messageFiFoIndexReaded );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// iterate
    this->messageFiFoIndexReaded++;
    if( this->messageFiFoIndexReaded >= queueLen ){
        this->messageFiFoIndexReaded = 0;
    }

// UnLock
    unlockPthread( this->messageFiFoLock );
    return true;
}











#endif