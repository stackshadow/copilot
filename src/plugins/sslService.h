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

#ifndef sslService_H
#define sslService_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "coPlugin.h"
#include "coCoreConfig.h"
#include "plugins/sslSession.h"

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>




class sslService : public coPlugin {

    typedef enum {
        in_connected,
        in_disconnected,
        out_connected,
        out_disconnected,
    } sessionState;

    private:
        lockID				sessionStateLock = 0;
		unsigned int		maxConnections = 5;
		unsigned int		curConnections = 0;

		pthread_t			threadWaitForNewClients;




// public functions
	public:
							sslService();
							~sslService();

// API
		coPlugin::t_state	onBroadcastMessage( coMessage* message );
		bool 				onSetup();
		bool				onExecute();

// configuration
        int                 maxConnection( int* setConnections );


// requested keys
        static unsigned int reqKeysCount();
        static bool         reqKeysGet( json_t* jsonObject );
        static bool         reqKeyRemove( const char* hostname );
        static bool         reqKeyAccept( const char* hostname );
        static bool         acceptedKeysGet( json_t* jsonObject );
        static bool         acceptedKeyRemove( const char* hostname );


// server ( copilotd-devices )
		void				serve();
		static void*		serveThread( void* void_service );
		static void*		serverHandleClientThread( void* void_sslSession );

		static int          serveOnNewPeer( void* userdata );

// client ( we connect to servers )
		void				connectAll();
		static void*		connectToClientThread( void* void_session );


};







#endif







