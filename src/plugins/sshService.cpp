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

#ifndef sshService_C
#define sshService_C

#include "plugins/sshService.h"
#include "coCore.h"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>

#include "plugins/sshSession.h"

sshService::               			sshService() : coPlugin( "sshService" ){
	this->loadConfig();
}


sshService::               			~sshService(){

}




void sshService:: 					loadConfig(){

// vars
    json_error_t    jsonError;
	json_t*			jsonValue;

// clear
	this->jsonConfig = NULL;
	this->jsonArrayServer = NULL;
	this->jsonServerIterator = NULL;
	this->jsonArrayClient = NULL;
	this->jsonClientIterator = NULL;

// open the file
    this->jsonConfig = json_load_file( configFile("ssh.json"), JSON_PRESERVE_ORDER, &jsonError );
    if( this->jsonConfig == NULL ){
        this->jsonConfig = json_object();
    }

// servers
	this->jsonArrayServer = json_object_get( this->jsonConfig, "server" );
	if( this->jsonArrayServer == NULL ){
		this->jsonArrayServer = json_array();
		json_object_set_new( this->jsonConfig, "server", this->jsonArrayServer );
		jsonValue = json_object();
		json_object_set_new( jsonValue, "host", json_string("::") );
		json_object_set_new( jsonValue, "port", json_integer(8989) );
		json_array_append_new( this->jsonArrayServer, jsonValue );
	}

// clients
	this->jsonArrayClient = json_object_get( this->jsonConfig, "client" );
	if( this->jsonArrayClient == NULL ){
		this->jsonArrayClient = json_array();
		json_object_set_new( this->jsonConfig, "client", this->jsonArrayClient );
	}

// save
	json_dump_file( this->jsonConfig, configFile("ssh.json"), JSON_PRESERVE_ORDER | JSON_INDENT(4) );

}


bool sshService:: 					nextServerConfig( const char** host, int* port ){

// vars
    json_error_t    jsonError;
	json_t*			jsonValue;

	if( this->jsonConfig == NULL ) return false;

// get the server configs
	jsonValue = json_object_get( this->jsonConfig, "server" );
	if( jsonValue == NULL ) return false;



}


bool sshService:: 					nextClientConfig( const char** host, int* port ){

}




bool sshService:: 					cmpToAllLokalKeys( ssh_key clientKey ){


// vars
	DIR*			keyDirectory = NULL;
	dirent*			keyEntry = NULL;
	ssh_key			acceptedKey;
	etString*		fullPath = NULL;
	const char*		fullKeyFilename = NULL;
	bool			returnValue = false;

// alloc
	etStringAllocLen( fullPath, 128 );

// open directory
	keyDirectory = opendir( sshClientKeyPath );
	if( keyDirectory == NULL ){
		etDebugMessage( etID_LEVEL_ERR, "Could not open key-directory" );
		return returnValue;
	}

// try every key
	keyEntry = readdir( keyDirectory );
	while( keyEntry != NULL ){

	// ignore "." and ".."
		if( keyEntry->d_name[0] == '.' ) goto nextKey;

	// build full path
		etStringCharSet( fullPath, sshClientKeyPath, -1 );
		etStringCharAdd( fullPath, keyEntry->d_name );
		etStringCharGet( fullPath, fullKeyFilename );

	// read public-key from file
		acceptedKey = NULL;
		if( ssh_pki_import_pubkey_file( fullKeyFilename, &acceptedKey ) != SSH_OK ){
			snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: is not a public key or not accessible.", fullKeyFilename );
			etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
			goto nextKey;
		}

	// check it
		if( ssh_key_cmp( clientKey, acceptedKey, SSH_KEY_CMP_PUBLIC ) == 0 ){
			snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: accepted", fullKeyFilename );
			etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );

			ssh_key_free( acceptedKey );
			returnValue = true;
			break;
		}
		ssh_key_free( acceptedKey );

nextKey:
		keyEntry = readdir( keyDirectory );
	}

// clean and return
	closedir( keyDirectory );
	etStringFree( fullPath );
	return returnValue;
}


bool sshService:: 					verify_knownhost( ssh_session session ){

// vars
	ssh_key			publicKey;
	int 			state;
	size_t 			hlen;
	unsigned char*	hash = NULL;
	char*			hexa;
	char			buf[10];

// check is server is known
	state = ssh_is_server_known( session );

// get the hash
	ssh_get_publickey( session, &publicKey );
	ssh_get_publickey_hash( publicKey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen );

	if (hlen < 0){
		return false;
	}

	switch( state ){
		case SSH_SERVER_KNOWN_OK:
			break; /* ok */

		case SSH_SERVER_KNOWN_CHANGED:
			fprintf(stderr, "Host key for server changed: it is now:\n");
			ssh_print_hexa("Public key hash", hash, hlen);
			fprintf(stderr, "For security reasons, connection will be stopped\n");
			free(hash);
			return false;

		case SSH_SERVER_FOUND_OTHER:
			fprintf(stderr, "The host key for this server was not found but an other"
			"type of key exists.\n");
			fprintf(stderr, "An attacker might change the default server key to"
			"confuse your client into thinking the key does not exist\n");
			free(hash);
			return false;

		case SSH_SERVER_FILE_NOT_FOUND:
			fprintf(stderr, "Could not find known host file.\n");
			fprintf(stderr, "If you accept the host key here, the file will be"
			"automatically created.\n");
			/* fallback to SSH_SERVER_NOT_KNOWN behavior */

		case SSH_SERVER_NOT_KNOWN:
			hexa = ssh_get_hexa(hash, hlen);
			fprintf(stderr,"The server is unknown. Do you trust the host key?\n");
			fprintf(stderr, "Public key hash: %s\n", hexa);
			free(hexa);
			if (fgets(buf, sizeof(buf), stdin) == NULL){
				free(hash);
				return false;
			}
			if (strncasecmp(buf, "yes", 3) != 0){
				free(hash);
				return false;
			}
			if (ssh_write_knownhost(session) < 0){
				fprintf(stderr, "Error %s\n", strerror(errno));
				free(hash);
				return false;
			}
			break;

		case SSH_SERVER_ERROR:
			fprintf(stderr, "Error %s", ssh_get_error(session));
			free(hash);
			return false;
	}

	free(hash);
	return true;
}




bool sshService::					checkAndCreateServerKeys(){


	FILE*		keyFile;
	ssh_key		sshKey;
	struct 		stat s;
	int 		err;

// create path if needed
	if( access( sshClientKeyPath, F_OK ) != 0 ){
		system( "mkdir -p " sshClientKeyPath );
	}
	if( access( sshServerKeyPath, F_OK ) != 0 ){
		system( "mkdir -p " sshServerKeyPath );
	}

// create RSA
	keyFile = fopen( "/etc/copilot/services/sshd_server_keys/rsa", "r" );
	if( keyFile == NULL ){
		system("ssh-keygen -t rsa -q -N \"\" -b 2048 -f /etc/copilot/services/sshd_server_keys/rsa" );
	}

// create DSA
	keyFile = fopen( "/etc/copilot/services/sshd_server_keys/dsa", "r" );
	if( keyFile == NULL ){
		system("ssh-keygen -t dsa -q -N \"\" -b 1024 -f /etc/copilot/services/sshd_server_keys/dsa" );
	}

// create ED25519
	keyFile = fopen( "/etc/copilot/services/sshd_server_keys/ed25519", "r" );
	if( keyFile == NULL ){
		system("ssh-keygen -t ed25519 -q -N \"\" -b 521 -f /etc/copilot/services/sshd_server_keys/ed25519" );
	}

	return true;
}


void sshService::					serve(){

	if( this->checkAndCreateServerKeys() == false ){
		return;
	}

// start the thread which wait for clients
	pthread_t thread;
	pthread_create( &thread, NULL, sshService::serveThread, this );
	pthread_detach( thread );

}


void* sshService::					serveThread( void* void_service ){

// vars
	sshService* service = (sshService*)void_service;
	sshSession* session = NULL;


// wait for connection limit
	while( service->curConnections >= service->maxConnections ){
		sleep(2);
	}
	service->curConnections++;

// create a new Session
	session = new sshSession();

// wait for incoming connection
	session->waitForClient();

// exchange keys and init crypto
	if( session->keyExchange() == false ){
		service->curConnections--;
		return NULL;
	}

// we create an event-loop
	ssh_event mainloop = ssh_event_new();

// poll all events
	session->pollUntilShell( mainloop );

// now a shell is established, so we TRY to serve a new connection
	pthread_t thread;
	pthread_create( &thread, NULL, sshService::serveThread, service );
	pthread_detach( thread );

// and we handle all other actions during the client close the connection
	session->pollEvents( mainloop );

// sleanup
	ssh_event_free( mainloop );
	delete session;
	service->curConnections--;
}




void sshService::					connectAll(){

// check if there is a possible client
	if( this->jsonArrayClient == NULL ) return;
	if( json_array_size( this->jsonArrayClient ) <= 0 ) return;

// start the thread which wait for clients
	pthread_t thread;
	pthread_create( &thread, NULL, sshService::connectToClientThread, this );


}


void* sshService::					connectToClientThread( void* void_service ){

//vars
	const char 		bindAddr[] = "0.0.0.0";
	int 			bindPort = 8989;
// vars
	sshService* service = (sshService*)void_service;
	sshSession* session = NULL;

wait:

	session = new sshSession();
	session->setConnection( 8989, "localhost" );
	session->connectToClient();
	//session->connectToClient( "localhost", 22 );



}







#endif