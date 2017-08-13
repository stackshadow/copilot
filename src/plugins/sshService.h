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

#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>

#define sshClientKeyPath "/etc/copilot/services/sshd_client_keys/"
#define sshServerKeyPath "/etc/copilot/services/sshd_server_keys/"

class sshService : public coPlugin {

	private:
		unsigned int		maxConnections = 1;
		unsigned int		curConnections = 0;

		json_t*				jsonConfig = NULL;
		json_t*				jsonArrayServer = NULL;
		void*				jsonServerIterator = NULL;
		json_t*				jsonArrayClient = NULL;
		json_t*				jsonClientIterator = NULL;

		pthread_t			threadWaitForNewClients;

		etList*				sessions;


// public functions
	public:
							sshService();
							~sshService();

// load json
		void				loadConfig();
		bool				nextServerConfig( const char** host, int* port );
		bool				nextClientConfig( const char** host, int* port );

// helper
		static bool 		cmpToAllLokalKeys( ssh_key clientKey );
		static bool			verify_knownhost( ssh_session session );

// server ( copilotd-devices )
		bool				checkAndCreateServerKeys();
		void				serve();
		static void*		serveThread( void* void_service );

// client ( we connect to servers )
		void				connectAll();
		static void*		connectToClientThread( void* void_service );
		void				appendClient( const char* ipv6 );
		void				appendClientv4( const char* ipv4 );

// callbacks
	public:
		//bool				onBroadcastMessage( coMessage* message );
		//bool        		onBroadcastReply( coMessage* message );


};







#endif







