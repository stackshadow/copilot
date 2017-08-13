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

#ifndef sshSession_C
#define sshSession_C

// {"id":"","h":"all","g":"co","c":"ping","v":""}


#include "plugins/sshService.h"
#include "coCore.h"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>

#include "plugins/sshSession.h"

sshSession::               			sshSession() : coPlugin( "sshSession" ){
	this->session = ssh_new();
	this->channelShell = NULL;

// connect infos
	this->port = 8989;
	etStringAllocLen( this->host, 32 );

// register plugin
	coCore::ptr->registerPlugin( this, "", "" );
}


sshSession::						~sshSession(){

// close the channel
	if( ssh_channel_is_open( this->channelShell ) != 0 ){
		ssh_channel_close( this->channelShell );
	}

// close the session
	ssh_disconnect( this->session );
	ssh_free( this->session );
	this->session = NULL;

// free memory
	this->port = 0;
	etStringFree( this->host );

// deregister plugin
	coCore::ptr->removePlugin( this );

}



void sshSession::					setConnection( int port, const char* hostname ){
	this->port = port;
	etStringCharSet( this->host, hostname, -1 );
}


bool sshSession:: 					isAuthenticated(){
	if( this->authState == SSH_AUTH_SUCCESS ){
		return true;
	}
	return false;
}


bool sshSession:: 					isActive(){
	if( this->channelShell == NULL ) return false;
	return true;
}


bool sshSession:: 					send( coMessage* message, ssh_channel sshChannel, bool useReply ){

// vars
	json_error_t	jsonError;
	json_t*			jsonMessage = NULL;
	char*			jsonChar;

// check if channel is open
	if( ssh_channel_is_open( sshChannel ) == 0 ) return false;

// create json from message
	jsonMessage = json_object();
	if( message->toJson( jsonMessage, useReply ) == false ){
		return false;
	}

// dump / send json
	jsonChar = json_dumps( jsonMessage, JSON_PRESERVE_ORDER | JSON_COMPACT );
	if( jsonChar != NULL ){
		ssh_channel_write( sshChannel, jsonChar, strlen(jsonChar) );
		free(jsonChar);
		json_decref(jsonMessage);
		return true;
	}

// free
	json_decref(jsonMessage);
	return false;
}


// ############################################################################################
// ################################# CALLBACKS FOR SSH-SERVER #################################
// ############################################################################################

void sshSession:: 					cbLog( ssh_session session, int priority, const char *message, void *userdata ){

	fprintf( stdout, "SSH %p: %s\n", session, message );
	fflush( stdout );


}


int sshSession::					cbAuthPubkey( ssh_session session, const char *user, struct ssh_key_struct *pubkey, char signature_state, void *userdata){

	sshSession* psession = (sshSession*)userdata;

	switch( signature_state ){

		case SSH_PUBLICKEY_STATE_NONE:
			if( sshService::cmpToAllLokalKeys(pubkey) == true ){
				return SSH_AUTH_SUCCESS;
			}
			break;

		case SSH_PUBLICKEY_STATE_VALID:
			if( sshService::cmpToAllLokalKeys(pubkey) == true ){
				psession->authState = SSH_AUTH_SUCCESS;
				return SSH_AUTH_SUCCESS;
			}

		default:
			psession->authState = SSH_AUTH_DENIED;
			break;

	}

	return SSH_AUTH_DENIED;
}


ssh_channel sshSession::			cbReqChannelOpen(ssh_session session, void *userdata){

	sshSession* psession = (sshSession*)userdata;
	if( psession->isAuthenticated() == false ) return NULL;

	ssh_channel newChannel = ssh_channel_new( session );


// set callback for server-channel
	ssh_set_channel_callbacks( newChannel, &psession->serverChannelCallbacks );

	return newChannel;
}


int sshSession::					cbReqService( ssh_session session, const char *service, void *userdata ){

	sshSession* psession = (sshSession*)userdata;

	if( strncmp("ssh-userauth",service,12) == 0 ){
		return 0;
	}

	return -1;
}


int sshSession::					cbServerShellRequest( ssh_session session, ssh_channel channel, void *userdata ){


// vars
	sshSession* 	psession = (sshSession*)userdata;
	if( psession->isAuthenticated() == false ) return -1;

// close old shell
	if( psession->channelShell != NULL ){
		if( ssh_channel_is_open( psession->channelShell ) != 0 ){
			ssh_channel_close( psession->channelShell );
		}
		ssh_channel_free( psession->channelShell );
	}

// save new shell
	psession->channelShell = channel;

	return 0;
}


int sshSession:: 					cbServerChannelData(	ssh_session 	session,
															ssh_channel 	channel,
															void*			data,
															uint32_t 		len,
															int 			is_stderr,
															void*			userdata ){

	sshSession* 	psession = (sshSession*)userdata;
	if( psession->isAuthenticated() == false ) return -1;

// vars
	json_error_t	jsonError;
	json_t*			jsonMessage = NULL;


// try to parse the data into json
	jsonMessage = json_loads( (const char*)data, JSON_PRESERVE_ORDER, &jsonError );
	if( jsonMessage == NULL || jsonError.line >= 0 ){
		snprintf( etDebugTempMessage, etDebugTempMessageLen, "JSON ERROR: %s \n", jsonError.text );
		etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

		psession->tempMessage.hostName( "" );
		psession->tempMessage.group( "ssh" );
		psession->tempMessage.replyCommand( "error" );
		psession->tempMessage.replyPayload( jsonError.text );

	// send it back
		psession->send( &psession->tempMessage, channel, true );

		return len;
	}

// parse the message
	if( psession->tempMessage.fromJson( jsonMessage ) == false ){
		json_decref( jsonMessage );

		psession->tempMessage.hostName( "" );
		psession->tempMessage.group( "ssh" );
		psession->tempMessage.replyCommand( "error" );
		psession->tempMessage.replyPayload( "Error on parsing..." );

	// send it back
		psession->send( &psession->tempMessage, channel, true );

		return len;
	}

// broadcast it to all plugins
	coCore::ptr->broadcast( psession, 	psession->tempMessage.hostName(),
										psession->tempMessage.group(),
										psession->tempMessage.command(),
										psession->tempMessage.payload() );


	return len;
}


int sshSession:: 					cbServerChannelExec( 	ssh_session session,
															ssh_channel channel,
															const char *command,
															void *userdata ){
	//sshSession::cbServerChannelData( session, channel, (void*)command, 0, 0, userdata );
	return -1;
}




// ############################################################################################
// ####################################### SERVER #############################################
// ############################################################################################


bool sshSession::					waitForClient(){

// vars
	const char 		bindAddr[] = "::";
	int 			bindPort = 8989;

// setup server
	this->sshServer = ssh_bind_new();
// debug
	int verbository = SSH_LOG_PROTOCOL;
	ssh_options_set( this->session, SSH_OPTIONS_LOG_VERBOSITY, &verbository );

// config
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_BINDADDR, &bindAddr );
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_BINDPORT, &bindPort );
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_RSAKEY, "/etc/copilot/services/sshd_server_keys/rsa" );
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_DSAKEY, "/etc/copilot/services/sshd_server_keys/dsa" );
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_ECDSAKEY, "/etc/copilot/services/sshd_server_keys/ed25519" );

// register listener
	if( ssh_bind_listen(this->sshServer) < 0 ){
        printf("Error listening to socket: %s\n",ssh_get_error(this->sshServer));
        return NULL;
    }

// setup provided services
    ssh_set_auth_methods(this->session,SSH_AUTH_METHOD_PUBLICKEY);

// setup server-callbacks
	this->serverCallbacks.auth_pubkey_function = sshSession::cbAuthPubkey;
	this->serverCallbacks.channel_open_request_session_function = sshSession::cbReqChannelOpen;
	this->serverCallbacks.service_request_function = sshSession::cbReqService;
	this->serverCallbacks.userdata = this;
	ssh_callbacks_init( &this->serverCallbacks );
	ssh_set_server_callbacks( this->session, &this->serverCallbacks );

// setup channel-callbacks
	this->serverChannelCallbacks.channel_shell_request_function = sshSession::cbServerShellRequest;
	this->serverChannelCallbacks.channel_data_function = sshSession::cbServerChannelData;
	this->serverChannelCallbacks.channel_exec_request_function = sshSession::cbServerChannelExec;
	this->serverChannelCallbacks.userdata = this;
	ssh_callbacks_init( &this->serverChannelCallbacks );

// wait for a clinet
	ssh_bind_accept( this->sshServer, this->session );

// there is a new client, we remove the listener
	ssh_bind_free( this->sshServer );

	return true;
}


bool sshSession::					keyExchange(){

// here a new client arrives
    if( ssh_handle_key_exchange( this->session ) != SSH_OK ) {
        printf("ssh_handle_key_exchange: %s\n", ssh_get_error( this->session ) );
        return false;
    }


	return true;
}


bool sshSession::					pollUntilShell( ssh_event mainLoop ){
    ssh_event_add_session( mainLoop, this->session);

	while( this->channelShell == NULL ){
		if( ssh_event_dopoll( mainLoop, -1 ) != SSH_OK ){
			break;
		}
		usleep( 50000L );
	}

	return true;
}


bool sshSession::					pollEvents( ssh_event mainLoop ){

// do the rest
	while( 1 ){
		if( ssh_event_dopoll( mainLoop, -1 ) != SSH_OK ){
			break;
		}
		usleep( 50000L );
	}

// connection lost
	ssh_event_remove_session( mainLoop, this->session );

}





// ############################################################################################
// ####################################### CLIENT #############################################
// ############################################################################################


bool sshSession::					connectToClient(){

	/*
	int verbository = SSH_LOG_PROTOCOL;
	ssh_options_set( this->session, SSH_OPTIONS_LOG_VERBOSITY, &verbository );
	 */

// vars
	const char*		hostNameChar = NULL;
	etStringCharGet( this->host, hostNameChar );


	ssh_options_set( this->session, SSH_OPTIONS_HOST, hostNameChar );
	ssh_options_set( this->session, SSH_OPTIONS_PORT, &this->port );
	if( ssh_connect( this->session ) != SSH_OK ){
		snprintf( etDebugTempMessage, etDebugTempMessageLen,
		"Error connecting to %s Port %i: %s\n", host, port, ssh_get_error(this->session) );
		etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
	}

// Verify the server's identity
	if( sshService::verify_knownhost( this->session ) == false ){
		return false;
	}

// we need a temprary string
	etString*		privateKeyFile = NULL;
	const char*		privateKeyFileChar = NULL;
	ssh_key			privateKey = NULL;
	int 			authResult = -1;

	etStringAllocLen( privateKeyFile, 64 );
	etStringCharSet( privateKeyFile, sshClientKeyPath, -1 );
	etStringCharAdd( privateKeyFile, hostNameChar );
	etStringCharGet( privateKeyFile, privateKeyFileChar );
	if( ssh_pki_import_privkey_file( privateKeyFileChar, NULL, NULL, NULL, &privateKey ) != SSH_OK ){
		snprintf( etDebugTempMessage, etDebugTempMessageLen,
		"Error with key %s : %s\n", privateKeyFileChar, ssh_get_error(this->session) );
		etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
		return false;
	}



	authResult = ssh_userauth_publickey( this->session, NULL, privateKey );


	this->channelShell = ssh_channel_new(this->session);
	if( this->channelShell == NULL ){
		return false;
	}

	if( ssh_channel_open_session( this->channelShell ) != SSH_OK ){
		ssh_channel_free(this->channelShell);
		this->channelShell = NULL;
		return false;
	}

	if( ssh_channel_request_shell( this->channelShell ) ){
		return false;
	}

	ssh_channel_close( this->channelShell );
	ssh_channel_send_eof( this->channelShell );
	ssh_channel_free( this->channelShell );
	this->channelShell = NULL;

}









coPlugin::t_state	sshSession::	onBroadcastMessage( coMessage* message ){
	if( this->isActive() == false ) return coPlugin::NO_REPLY;

// vars
	const char*		messageHostName = message->hostName();
	json_t*			jsonMessageObject = json_object();
	char*			jsonString = NULL;

// we only accept message for other hosts
	if( strncmp("localhost",messageHostName,9) == 0 ) return coPlugin::NO_REPLY;
	if( strncmp(coCore::ptr->hostInfo.nodename,messageHostName,coCore::ptr->hostNodeNameLen) == 0 ) return coPlugin::NO_REPLY;

// send out the message
	this->send( message, this->channelShell, false );

// we dont have anything to reply, because the answer comes asynchron
	return coPlugin::NO_REPLY;
}


bool sshSession::       			onBroadcastReply( coMessage* message ){
	this->send( message, this->channelShell, true );
}



#endif