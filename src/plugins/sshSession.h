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

#ifndef sshSession_H
#define sshSession_H

#include "sshService.h"

class sshSession : public coPlugin {



	public:
		// states
		struct ssh_server_callbacks_struct 		serverCallbacks;
		struct ssh_channel_callbacks_struct		serverChannelCallbacks;

	private:
		int					port = 8989;
		etString*			host = NULL;

		int					authState = SSH_AUTH_AGAIN;
		ssh_bind 			sshServer;
		ssh_session 		session;
		ssh_event			sessionLoop;
		ssh_channel			channelShell;
		coMessage			tempMessage;

	public:
							sshSession();
							~sshSession();

// common stuff
		void				setConnection( int port, const char* hostname );
		bool				isAuthenticated();
		bool				isActive();
		bool				send( coMessage* message, ssh_channel sshChannel, bool useReply );


// server-side
		static void			cbLog( ssh_session session, int priority, const char *message, void *userdata );
		static int			cbAuthPubkey( ssh_session session, const char *user, struct ssh_key_struct *pubkey, char signature_state, void *userdata );
		static ssh_channel	cbReqChannelOpen(ssh_session session, void *userdata);
		static int			cbReqService( ssh_session session, const char *service, void *userdata );

// server-channel-callbacks
		static int			cbServerShellRequest( ssh_session session, ssh_channel channel, void *userdata );
		static int 			cbServerChannelData(	ssh_session 	session,
													ssh_channel 	channel,
													void*			data,
													uint32_t 		len,
													int 			is_stderr,
													void*			userdata );
		static int			cbServerChannelExec( 	ssh_session session,
													ssh_channel channel,
													const char *command,
													void *userdata );

		bool				waitForClient();
		bool				keyExchange();
		bool				pollUntilShell( ssh_event mainLoop );
		bool				pollEvents( ssh_event mainLoop );


// client-side
		bool				connectToClient();


// API
		coPlugin::t_state	onBroadcastMessage( coMessage* message );
		bool        		onBroadcastReply( coMessage* message );


};




#endif