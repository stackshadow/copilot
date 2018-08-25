/*
Copyright (C) 2018 by Martin Langlotz

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

#ifndef pubsub_H
#define pubsub_H

#include "evillib_depends.h"
#include "memory/etList.h"
#include "jansson.h"


// callbackfunction
typedef int (*psSubscriberMessage)( void* objectSource, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, void* userdata );
typedef int (*psSubscriberJsonMessage)( void* objectSource, json_t* jsonObject, void* userdata );




class psBus {

private:
    void**          psSubscriberArray;
    size_t          psSubscriberArrayLen;
    int             psSubscriberArrayLock;
    const char*     localNodeName;

public:
    static psBus*	inst;
    typedef enum e_state {
        NEXT_SUBSCRIBER = 1,     // Message finished, next subscriber can have it
        END = 2                  // Message finished, nobody else need that
    } t_state;


public:
    psBus();
    ~psBus();

    static void     toJson( json_t** jsonObject, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload );
    static bool     fromJson( json_t* jsonObject, const char** id, const char** nodeSource, const char** nodeTarget, const char** group, const char** command, const char** payload );

    void            _subscribe( void* objectSource, const char* nodeTarget, const char* group, void* userdata, psSubscriberMessage callback, psSubscriberJsonMessage jsonCallback );
    void            _publish( void* objectSource, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload );
    void            _publish( void* objectSource, json_t* jsonObject );
    void            _publishOrSubscribe( void* objectSource, json_t* jsonObject, void* userdata, psSubscriberMessage callback, psSubscriberJsonMessage jsonCallback );

// function types
    typedef void    (*toJson_tf)( json_t** jsonObject, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload );
    typedef bool    (*fromJson_tf)( json_t* jsonObject, const char** id, const char** nodeSource, const char** nodeTarget, const char** group, const char** command, const char** payload );

    typedef void    (*subscribe_tf)( void* objectSource, const char* nodeTarget, const char* group, void* userdata, psSubscriberMessage callback, psSubscriberJsonMessage jsonCallback );
    typedef void    (*publish_tf)( void* objectSource, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload );
    typedef void    (*publishJson_tf)( void* objectSource, json_t* jsonObject );

// static
    static void     subscribe( void* objectSource, const char* nodeTarget, const char* group, void* userdata, psSubscriberMessage callback, psSubscriberJsonMessage jsonCallback ){
        if( psBus::inst == NULL ) return;
        psBus::inst->_subscribe( objectSource, nodeTarget, group, userdata, callback, jsonCallback );
    }
    static void     publish( void* objectSource, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload ){
        if( psBus::inst == NULL ) return;
        psBus::inst->_publish( objectSource, id, nodeSource, nodeTarget, group, command, payload );
    }
    static void     publishJson( void* objectSource, json_t* jsonObject ){
        if( psBus::inst == NULL ) return;
        psBus::inst->_publish( objectSource, jsonObject );
    }


};






#endif