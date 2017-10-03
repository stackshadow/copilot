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

#ifndef sshService_H
#define sshService_H


#include <stdio.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include "memory/etList.h"

#include "coPlugin.h"
#include "coCoreConfig.h"

#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>

#define sshServerKeyPath baseFilePath "sshd_server_keys/"
#define sshKeyReqPath baseFilePath "sshd_req_keys/"
#define sshAcceptedKeyPath baseFilePath "sshd_accepted_keys/"
#define sshClientKeyPath baseFilePath "sshd_client_keys/"

class sshService : public coPlugin {

    typedef enum {
        in_connected,
        in_disconnected,
        out_connected,
        out_disconnected,
    } sessionState;

    private:
        lockID				sessionStateLock = 0;
		unsigned int		maxConnections = 1;
		unsigned int		curConnections = 0;

		pthread_t			threadWaitForNewClients;


// public functions
	public:
							sshService();
							~sshService();
// API
		coPlugin::t_state	onBroadcastMessage( coMessage* message );
		bool        		onBroadcastReply( coMessage* message );
		bool 				onSetup();
		bool				onExecute();

// helper
		static bool 		cmpToAllLokalKeys( ssh_key clientKey );
		static bool			askForSaveClientKey( ssh_key clientKey );
		static bool			verify_knownhost( ssh_session session );


// requested keys
        static unsigned int reqKeysCount();
        static bool         reqKeyAdd( ssh_key clientKey );
        static bool         reqKeysGet( json_t* jsonObject );
        static bool         reqKeysRemove( const char* fingerprint );
        static bool         reqKeysAccept( const char* fingerprint );
        static bool         acceptedKeysGet( json_t* jsonObject );
        static bool         acceptedKeysRemove( const char* fingerprint );


// server ( copilotd-devices )
		static bool			checkAndCreateServerKeys();
		void				serve();
		static void*		serveThread( void* void_service );

// client ( we connect to servers )
        void                connect( const char* hostname, int port );
		void				connectAll();
		static void*		connectToClientThread( void* void_service );


};







#endif







