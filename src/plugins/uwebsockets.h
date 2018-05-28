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

#ifndef DISABLE_UWEBSOCKET
#ifndef uwebsocket_H
#define uwebsocket_H

#include "string/etString.h"
#include "string/etStringChar.h"

#include "evillib-extra_depends.h"
#include "db/etDBObject.h"

//#include "coPlugin.h"

#include "uWS.h"
#include "WebSocket.h"

class uwebsocket {


private:
    int								port;
    pthread_t                   	thread;
	uWS::WebSocket<uWS::SERVER>*	wsServer;
	
public:
                                uwebsocket( int wsPort );
                                ~uwebsocket();
	static 	uwebsocket*			inst;


public:
    static void*                wsThread( void* data );

	void						onMessage( uWS::WebSocket<uWS::SERVER> *server, char *message, size_t messageSize, uWS::OpCode opCode );
	//void                        wsReply( const char* message );
	

	static int					onSubscriberMessage( const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, void* userdata );
	static int					onSubscriberJsonMessage( json_t* jsonObject, void* userdata );

};













#endif // websocket_H

#endif // DISABLE_WEBSOCKET