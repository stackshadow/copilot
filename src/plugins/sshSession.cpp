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
// ssh -T -p 4567 copilotd@localhost -i /tmp/etc/copilotd/sshd_server_keys/ed25519

#include "coCore.h"
#include <unistd.h>             // usleep
#include <arpa/inet.h>          // inet_ntop
#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>

#include "plugins/sshService.h" // For static helper functions
#include "plugins/sshSession.h"


sshSession::               			sshSession() : coPlugin( "sshSession", "", "" ){
	this->session = ssh_new();
	this->channelShell = NULL;



// connect infos
	this->port = 8989;
	etStringAllocLen( this->host, 32 );
//	etStringAllocLen( this->peerName, 32 );

// register plugin
	coCore::ptr->plugins->append( this );
}


sshSession::						~sshSession(){

// close the channel
	if( ssh_channel_is_open( this->channelShell ) != 0 ){
        ssh_channel_send_eof( this->channelShell );
		ssh_channel_close( this->channelShell );
	}

// close the session
	ssh_disconnect( this->session );
	ssh_free( this->session );
	this->session = NULL;

// free memory
	this->port = 0;
	etStringFree( this->host );
//	etStringFree( this->peerName );

// free bind if present
    if( this->sshServer != NULL )
	ssh_bind_free( this->sshServer );

// deregister plugin
	coCore::ptr->plugins->remove( this );

}



// API

coPlugin::t_state	sshSession::	onBroadcastMessage( coMessage* message ){
	if( this->isActive() == false ) return coPlugin::NO_REPLY;

// vars
    const char*     msgSource = message->hostNameSource();
	json_t*			jsonMessageObject = json_object();
	char*			jsonString = NULL;

/// @todo limit outgoing messages ?

// send out the message
	this->send( message, this->channelShell, false );

// we dont have anything to reply, because the answer comes asynchron
	return coPlugin::NO_REPLY;
}


bool sshSession::       			onBroadcastReply( coMessage* message ){
	this->send( message, this->channelShell, true );
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
    const char*     myHostName = coCore::ptr->hostNameGet();
    const char*     msgTarget = message->hostNameTarget();
	json_error_t	jsonError;
	json_t*			jsonMessage = NULL;
	char*			jsonString = NULL;
    size_t          jsonStringSize = 0;
    char*           messageString = NULL;
    size_t          messageStringSize = 0;
    int             uintSize = sizeof(unsigned int);


// check if channel is open
	if( ssh_channel_is_open( sshChannel ) == 0 ) return false;

// if there is a message for myHost we dont need to send it out to the world
    if( strncmp(msgTarget,myHostName,strlen(myHostName)) == 0 ){
        return false;
    }

// create json from message
	jsonMessage = json_object();
	if( message->toJson( jsonMessage, useReply ) == false ){
		return false;
	}


// dump / send json
	jsonString = json_dumps( jsonMessage, JSON_PRESERVE_ORDER | JSON_COMPACT );
	if( jsonString != NULL ){

	// debugging message
		snprintf( etDebugTempMessage, etDebugTempMessageLen, "Send %s", jsonString );
		etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

    // allocate bigger message and add seperator ( '\n' )
        jsonStringSize = (strlen(jsonString)+1) * sizeof(char);

	// write to ssh-channel
		int writtenBytes = ssh_channel_write( sshChannel, jsonString, jsonStringSize );
        int flushrc = ssh_blocking_flush( this->session, 5000 );
        while( flushrc != SSH_OK ){

            switch( flushrc ){

                case SSH_AGAIN:
                    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Not all bytes are written, need to send again", jsonString );
                    etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
                    break;
                case SSH_ERROR:
                    snprintf( etDebugTempMessage, etDebugTempMessageLen, "SSH Error %s", ssh_get_error(this->sshServer) );
                    etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

                default:
                    break;
            }

        }


	// cleanup
        free(jsonString);
		json_decref(jsonMessage);
		return true;
	}

// free
onfail:
    if( jsonString != NULL ) free(jsonString);
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
            // we dont allow clients which only has an public key
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



// get the host
    struct sockaddr_storage     sockStorage;
    struct sockaddr_in*         sockIn;
    unsigned int                len = 100;
    char                        ip[100] = "\0";
    char                        node[NI_MAXHOST];
    char*                       nodeHostName = NULL;

// get peer
    getpeername( ssh_get_fd(session), (struct sockaddr*)&sockStorage, &len );
    sockIn = (struct sockaddr_in *)&sockStorage;
    sockIn->sin_family = AF_INET;


// try to resolve the hostname
    int res = getnameinfo((struct sockaddr*)sockIn, sizeof(*sockIn), node, sizeof(node), NULL, 0, 0);
    if( res == 0 ){
        nodeHostName = node;
    } else {
    // could not get hostname, get the ip
        inet_ntop(AF_INET, &sockIn->sin_addr, ip, len);
        nodeHostName = ip;
    }
    etStringCharSet( psession->host, nodeHostName, NI_MAXHOST );

// try to find and add node
    coCore::ptr->config->nodesIterate();
    if( coCore::ptr->config->nodeSelectByHostName(nodeHostName) == false ){
        coCore::ptr->config->nodeAppend( nodeHostName );
        coCore::ptr->config->nodeConnInfo( (const char**)&nodeHostName, NULL, true );
    }
    coCore::ptr->config->nodesIterateFinish();


// debugging message
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "New channel created" );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );


    psession->channelShell = newChannel;

	return newChannel;
}


int sshSession::					cbReqService( ssh_session session, const char *service, void *userdata ){

	sshSession* psession = (sshSession*)userdata;

	if( strncmp("ssh-userauth",service,12) == 0 ){
    // debugging message
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Requested service ssh-userauth accepted" );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
		return 0;
	}

// debugging
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Requested service '%S' not accepted", service );
    etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
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


// get the host
    struct sockaddr_storage     tmp;
    struct sockaddr_in*         sock;
    unsigned int                len = 100;
    char                        ip[100] = "\0";
    char                        node[NI_MAXHOST];
    char*                       ipName = NULL;
    char*                       nodeHostName = NULL;

// get peer
    getpeername( ssh_get_fd(session), (struct sockaddr*)&tmp, &len );
    sock = (struct sockaddr_in *)&tmp;
    sock->sin_family = AF_INET;


// try to resolve the hostname
    int res = getnameinfo((struct sockaddr*)sock, sizeof(*sock), node, sizeof(node), NULL, 0, 0);
    if( res == 0 ){
        nodeHostName = node;
    } else {
    // could not get hostname, get the ip
        inet_ntop(AF_INET, &sock->sin_addr, ip, len);
        nodeHostName = ip;
    }
    etStringCharSet( psession->host, nodeHostName, NI_MAXHOST );

// try to find and add node
    coCore::ptr->config->nodesIterate();
    if( coCore::ptr->config->nodeSelectByHostName(nodeHostName) == false ){
        coCore::ptr->config->nodeAppend( nodeHostName );
        coCore::ptr->config->nodeConnInfo( (const char**)&nodeHostName, NULL, true );
    }
    coCore::ptr->config->nodesIterateFinish();

// debugging message
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Accept requested shell for %s", nodeHostName );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );


// save new shell
	psession->channelShell = channel;

// return
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
    const char*     myHostName = coCore::ptr->hostNameGet();

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s \n", data );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );


// try to parse the data into json
	jsonMessage = json_loads( (const char*)data, JSON_PRESERVE_ORDER, &jsonError );
	if( jsonMessage == NULL || jsonError.line >= 0 ){

		snprintf( etDebugTempMessage, etDebugTempMessageLen, "JSON ERROR: %s \n", jsonError.text );
		etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

        //coCore::ptr->plugins->messageAdd( psession, "", "ssh", "error", jsonError.text );

		return len;
	}

// parse the message
	if( psession->tempMessage.fromJson( jsonMessage ) == false ){
		json_decref( jsonMessage );

		snprintf( etDebugTempMessage, etDebugTempMessageLen, "JSON ERROR \n" );
		etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

//        coCore::ptr->plugins->messageAdd( psession, "", "ssh", "error", "Error parsing json..." );

		return len;
	}
    json_decref(jsonMessage);



    coCore::ptr->plugins->messageAdd(   psession,
                                        psession->tempMessage.hostNameSource(),
                                        psession->tempMessage.hostNameTarget(),
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


bool sshSession::					waitForClient( const char* bindAddr, int bindPort ){
    if( bindAddr == NULL ) return false;
    if( bindPort == 0 ) return false;

// vars
//	const char 		bindAddr[] = "::";
//	int 			bindPort = 8989;

// setup server
	this->sshServer = ssh_bind_new();
// debug
	int verbository = SSH_LOG_PROTOCOL;
	ssh_options_set( this->session, SSH_OPTIONS_LOG_VERBOSITY, &verbository );

// config
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_BINDADDR, (const char*)bindAddr );
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_BINDPORT, &bindPort );
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_RSAKEY, "/etc/copilot/services/sshd_server_keys/rsa" );
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_DSAKEY, "/etc/copilot/services/sshd_server_keys/dsa" );
	ssh_bind_options_set( this->sshServer, SSH_BIND_OPTIONS_ECDSAKEY, "/etc/copilot/services/sshd_server_keys/ed25519" );

// debugging message
	snprintf( etDebugTempMessage, etDebugTempMessageLen, "Start Server on %s on port %i", bindAddr, bindPort );
	etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

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
	ssh_bind_free( this->sshServer ); this->sshServer = NULL;

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


bool sshSession::					pollUntilShell( ssh_event mainLoop, unsigned int timeoutSeconds ){

// vars
    unsigned int timeoutCounter = timeoutSeconds * 20;

// add this session to the main loop
    ssh_event_add_session( mainLoop, this->session);

    while( timeoutCounter >= 0 ){

		if( ssh_event_dopoll( mainLoop, -1 ) != SSH_OK ){
			break;
		}

		if( this->channelShell != NULL ){
            return true;
		}

        usleep( 50000L );
        timeoutCounter--;
    }

    etDebugMessage( etID_LEVEL_ERR, "Timeout" );

	return false;
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

// on first start, we dont have an delay
    goto nodelay;

trydelayed:
    sleep(10);
nodelay:

// vars
	const char*		hostNameChar = NULL;
	etString*		privateKeyFile = NULL;
	const char*		privateKeyFileChar = NULL;
	ssh_key			privateKey = NULL;
	int 			authResult = -1;
    ssh_event       mainloop;
    char            buffer[2048];
    int             bufferReadedBytes;
    const char*     message = NULL;


// connect to server
	etStringCharGet( this->host, hostNameChar );
	ssh_options_set( this->session, SSH_OPTIONS_HOST, hostNameChar );
	ssh_options_set( this->session, SSH_OPTIONS_PORT, &this->port );
	if( ssh_connect( this->session ) != SSH_OK ){
		snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error connecting to %s Port %i: %s\n", host, port, ssh_get_error(this->session) );
		etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        goto close;
	}

// Verify the server's identity
	if( sshService::verify_knownhost( this->session ) == false ){
		return false;
	}


// build the full key-path
	etStringAllocLen( privateKeyFile, 64 );
	etStringCharSet( privateKeyFile, sshClientKeyPath, -1 );
	etStringCharAdd( privateKeyFile, hostNameChar );
	etStringCharGet( privateKeyFile, privateKeyFileChar );

// create key if missing
	if( access( privateKeyFileChar, F_OK ) != 0 ) {
        etStringCharSet( privateKeyFile, "ssh-keygen -t ed25519 -q -N \"\" -b 521 -f ", -1);
        etStringCharAdd( privateKeyFile, sshClientKeyPath );
        etStringCharAdd( privateKeyFile, hostNameChar );
        etStringCharGet( privateKeyFile, privateKeyFileChar );
		system( privateKeyFileChar );

        etStringCharSet( privateKeyFile, sshClientKeyPath, -1 );
        etStringCharAdd( privateKeyFile, hostNameChar );
        etStringCharGet( privateKeyFile, privateKeyFileChar );
	}

	if( ssh_pki_import_privkey_file( privateKeyFileChar, NULL, NULL, NULL, &privateKey ) != SSH_OK ){
		snprintf( etDebugTempMessage, etDebugTempMessageLen,
		"Error with key %s : %s\n", privateKeyFileChar, ssh_get_error(this->session) );
		etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
		return false;
	}


// try to authenticate
	authResult = ssh_userauth_publickey( this->session, NULL, privateKey );
    if( authResult != 0 ) goto close;

// set authenticated
    this->authState = SSH_AUTH_SUCCESS;

// create channel memory
	this->channelShell = ssh_channel_new(this->session);
	if( this->channelShell == NULL ){
		return false;
	}

// request a new channel
	if( ssh_channel_open_session( this->channelShell ) != SSH_OK ){
		ssh_channel_free(this->channelShell);
		this->channelShell = NULL;
		return false;
	}



// request shell
/*
	if( ssh_channel_request_shell( this->channelShell ) ){
		return false;
	}
*/


// Test: Send a ping
/*
    this->tempMessage.hostName( "all" );
    this->tempMessage.group( "co" );
    this->tempMessage.command( "ping" );
    this->tempMessage.payload( "");

    this->send( &this->tempMessage, this->channelShell, false );
*/


// loop
    memset( buffer, 0, sizeof(buffer) ); // clear buffer
    while( ssh_is_connected(this->session) == 1 && ssh_channel_is_open(this->channelShell) != 0 ){

        // eof is reached
            if( ssh_channel_is_eof(this->channelShell) != 0 ) break;


            bufferReadedBytes = ssh_channel_read_timeout( this->channelShell, buffer, sizeof(buffer), 0, 3000 );
            if( bufferReadedBytes > 0 ){

                message = strtok( buffer, "\n" );
                while( message != NULL ){
                    sshSession::cbServerChannelData( this->session, this->channelShell, buffer, bufferReadedBytes, 0, this );
                    message = strtok( NULL, "\n" );
                }


            // clear buffer
                memset( buffer, 0, sizeof(buffer) );
            }


            //sleep(1);
    }

close:
	ssh_channel_close( this->channelShell );
	ssh_channel_send_eof( this->channelShell );
	ssh_channel_free( this->channelShell );
	this->channelShell = NULL;
    ssh_disconnect( this->session );

    ssh_free( this->session );
    this->session = ssh_new();

    goto trydelayed;
}











#endif
