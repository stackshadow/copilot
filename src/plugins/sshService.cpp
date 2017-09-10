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


sshService::               			sshService() : coPlugin( "sshService", "", "cocom" ){
	etListAlloc( this->sessions );

// register plugin
	coCore::ptr->plugins->append( this );
}


sshService::               			~sshService(){

}




coPlugin::t_state sshService::		onBroadcastMessage( coMessage* message ){

// vars
	const char*		msgCommand = message->command();
	const char*		msgPayload = message->payload();
	json_t*			jsonPayload;
	json_error_t	jsonError;


	if( strncmp(msgCommand,"serverSave",10) == 0 ){

	// parse json
		jsonPayload = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
		if( jsonPayload == NULL || jsonError.line > -1 ){
			return coPlugin::NO_REPLY;
		}


		json_object_get( jsonPayload, "host" );
		json_object_get( jsonPayload, "port" );

	// cleanup
		json_decref(jsonPayload);
	}


}


bool sshService::        			onBroadcastReply( coMessage* message ){

}


bool sshService:: 					onSetup(){

// check if we have an server connection

// get the server infos
	bool 					serverFound = false;
	coCoreConfig::nodeType 	serverType;
	const char* 			serverHost;
	int 					serverPort;
	char 					answer[512] = { '0' };
	int						answerLen = 0;
	const char* 			pAnswer = (const char*)&answer;

	fprintf( stdout, "Copilot Node-Connection-Setup \n" );

ask:
	fprintf( stdout, "1.) Accept Connection from another node ( aka training-mode ) \n" );
	fprintf( stdout, "2.) Connect to another node \n" );
	fprintf( stdout, "What would you like to do ? (1/2) \n" );
	fflush( stdout );
	read( STDIN_FILENO, answer, 512 );
	fprintf( stdout, "\n" );

	if( answer[0] != '1' && answer[0] != '2' ) goto ask;

	if( answer[0] == '1' ){

		coCore::ptr->config->nodesIterate();

	// default server / port
		answer[0] = ':'; answer[1] = ':'; answer[2] = '\0';
		serverType = coCoreConfig::SERVER;
		serverPort = 4567;

	// read hostname
		fprintf( stdout, "Enter host/IP to bind. ( user :: for IPv6 and IPv4 ) " );
		fflush( stdout );
		read( STDIN_FILENO, answer, 512 );
		fprintf( stdout, "\n" );
	// remove 0
		answerLen = strlen(answer);
		if( answerLen > 0 ) answerLen--;
		answer[answerLen] = '\0';

	// save
		coCore::ptr->config->nodeAppend("nodeServer");
		coCore::ptr->config->nodeInfo( NULL, &serverType, true );
		coCore::ptr->config->nodeConnInfo( (const char**)&pAnswer, &serverPort, true );
		coCore::ptr->config->nodesIterateFinish();

	// save it
		coCore::ptr->config->save();

	// serve connection
		this->serve();

	}


}


bool sshService:: 					onExecute(){
	if( coCore::setupMode == true ) return true;

	this->serve();
	this->connectAll();
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


bool sshService::					askForSaveClientKey( ssh_key clientKey ){

// vars
	unsigned char* 		hash;
	size_t				hlen;
	char 				answer[512] = { '0' };

// is the key public?
	if( ssh_key_is_public(clientKey) != 1 ){
		return false;
	}


// get key-hash
	if( ssh_get_publickey_hash( clientKey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen ) != SSH_OK ){
		return false;
	}

	char* str = ssh_get_hexa( hash, hlen );

	fprintf( stdout, "Accept Key? %s \n", str );
	fflush( stdout );
	read( STDIN_FILENO, answer, 512 );


	if( answer[0] == 'y' ){

	keyName:
		fprintf( stdout, "Enter a key name:" );
		fflush( stdout );
		read( STDIN_FILENO, answer, 512 );
		fprintf( stdout, "\n" );
		if( strlen(answer) <= 0 ) goto keyName;
		if( answer[0] == '.' ) goto keyName;
		if( answer[0] == '/' ) goto keyName;


	// remove 0
		int answerLen = strlen(answer);
		if( answerLen > 0 ) answerLen--;
		answer[answerLen] = '\0';

	// we build the full path
		etString* fullKeyPath; const char* keyPath;
		etStringAllocLen( fullKeyPath, 128 );
		etStringCharSet( fullKeyPath, sshClientKeyPath, -1 );
		etStringCharAdd( fullKeyPath, answer );
		etStringCharGet( fullKeyPath, keyPath );


		ssh_pki_export_pubkey_file( clientKey, keyPath );
		return true;
	}




	return false;
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
	if( access( sshServerKeyPath "rsa", F_OK ) != 0 ) {
		system("ssh-keygen -t rsa -q -N \"\" -b 2048 -f " sshServerKeyPath "rsa" );
	}

// create DSA
	if( access( sshServerKeyPath "dsa", F_OK ) != 0 ) {
		system("ssh-keygen -t dsa -q -N \"\" -b 1024 -f " sshServerKeyPath "dsa" );
	}

// create ED25519
	if( access( sshServerKeyPath "ed25519", F_OK ) != 0 ) {
		system("ssh-keygen -t ed25519 -q -N \"\" -b 521 -f " sshServerKeyPath "ed25519" );
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

// get the server infos
	bool serverFound = false; coCoreConfig::nodeType serverType; const char* serverHost; int serverPort;
	coCore::ptr->config->nodesIterate();
	while( coCore::ptr->config->nodeNext() == true ){
		coCore::ptr->config->nodeInfo(NULL, &serverType);
		if( serverType == coCoreConfig::SERVER ){
			coCore::ptr->config->nodeConnInfo( &serverHost, &serverPort );
			serverFound = true;
		}
	}
	coCore::ptr->config->nodesIterateFinish();
// no server found
	if( serverFound == false ){

		return NULL;
	}

// wait for incoming connection
	session->waitForClient( serverHost, serverPort );

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

// vars
	const char*					clientHost;
	int							clientPort;
	coCoreConfig::nodeType		nodeType;

// start iteration ( and lock core )
	coCore::ptr->config->nodesIterate();

	while( coCore::ptr->config->nodeNext() == true ){

		coCore::ptr->config->nodeInfo(&clientHost,&nodeType);

	// only clients
		if( nodeType != coCoreConfig::CLIENT ){
			continue;
		}

	// get host/port
		if( coCore::ptr->config->nodeConnInfo( &clientHost, &clientPort ) != true ){
			continue;
		}

	// create new session
		sshSession* session = NULL;
		session = new sshSession();
		session->setConnection( clientPort, clientHost );

	// start the thread which wait for clients
		pthread_t thread;
		pthread_create( &thread, NULL, sshService::connectToClientThread, session );
		pthread_detach( thread );

	}

// finish
	coCore::ptr->config->nodesIterateFinish();
}


void* sshService::					connectToClientThread( void* void_service ){

//vars
	const char 		bindAddr[] = "0.0.0.0";
	int 			bindPort = 8989;
// vars
	sshService* service = (sshService*)void_service;
	sshSession* session = (sshSession*)void_service;

wait:
	session->connectToClient();
	//session->connectToClient( "localhost", 22 );



}





#endif