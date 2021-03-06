/*
Copyright (C) 2018 by Martin Langlotz aka stackshadow

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

#ifndef DISABLE_WEBSOCKET
#ifndef websocket_H
#define websocket_H

#include "string/etString.h"
#include "string/etStringChar.h"

#include "evillib-extra_depends.h"
#include "db/etDBObject.h"

#include "coPlugin.h"

#include <libwebsockets.h>




class websocket : public coPlugin {


private:

    struct clientSessionData {
        struct lws*         wsi;
        bool                authenthicated;
    };

    struct lws_protocols protocols[3] = {
    /* first protocol must always be HTTP handler */
    {
        "http-only",                                /* name */
        websocket::wsCallbackHttp,                  /* callback */
        sizeof( struct clientSessionData ),         /* per_session_data_size */
        0,                                          /* max frame size / rx buffer */
    },
    {
        "copilot",
        websocket::wsCallbackCopilot,
        sizeof( struct clientSessionData),
        65536,
    },/*
    {
        "lws-mirror-protocol",
        callback_lws_mirror,
        sizeof(struct per_session_data__lws_mirror),
        128,
    },*/
    { NULL, NULL, 0, 0 } /* terminator */
    };

    struct lws_context*         wsContext;
    pthread_t                   thread;
    int                         wsThreadRun = 0;


    struct clientSessionData*   actualClientSession = NULL;
    etString*                   actualClientReply = NULL;

public:
                                websocket( int wsPort );
                                ~websocket();
    static websocket*           ptr;

public:
    static void*                wsThread( void* data );
    static int                  wsCallbackHttp( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len );
    static int                  wsCallbackCopilot( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len );
    void                        wsOnMessage( const char* message, int messageLen );
    void                        wsReply( const char* message );

// auth

    bool                        isAuth();
    void                        setAuth( bool authenticated );

// handlers
	coPlugin::t_state			onBroadcastMessage( coMessage* message );
	bool        				onBroadcastReply( coMessage* message );

/*
public slots:
    void                        onNewConnection();
*/

};

#endif // websocket_H

#endif // DISABLE_WEBSOCKET