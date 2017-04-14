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

#ifndef mqttService_H
#define mqttService_H



#include <stdio.h>
#include <mosquitto.h>

#include "coPlugin.h"

class mqttService : public coPlugin {


private:
    char                hostName[128];
    int                 hostPort;
    struct mosquitto*   mosq = NULL;
    bool                connected = false;
    char                lastPubTopic[128];


public:
                    mqttService( char* host, int port = 1883 );
                    ~mqttService();
    static mqttService* ptr;


// callbacks of mosquitto
public:
    static void     cb_onConnect( struct mosquitto* mosq, void* userdata, int result );
    static void     cb_onDisConnect( struct mosquitto* mosq, void* userdata, int result );
    static void     cb_onMessage( struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

// callbacks
public:
    bool            broadcastReply( json_t* jsonAnswerArray );
    bool            onMessage(  const char*     msgHostName, 
                                const char*     msgGroup, 
                                const char*     msgCommand, 
                                json_t*         jsonData, 
                                json_t*         jsonAnswerObject );

};
















#endif