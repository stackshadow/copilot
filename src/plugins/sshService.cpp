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

#include "uuid.h"
#include "limits.h"     // for UINT_MAX


sshService::               			sshService() : coPlugin( "sshService", "", "cocom" ){

// create path if needed
	if( access( sshServerKeyPath, F_OK ) != 0 ){
		system( "mkdir -p " sshServerKeyPath );
	}
	if( access( sshKeyReqPath, F_OK ) != 0 ){
		system( "mkdir -p " sshKeyReqPath );
	}
	if( access( sshAcceptedKeyPath, F_OK ) != 0 ){
		system( "mkdir -p " sshAcceptedKeyPath );
	}
	if( access( sshClientKeyPath, F_OK ) != 0 ){
		system( "mkdir -p " sshClientKeyPath );
	}

// register plugin
	coCore::ptr->plugins->append( this );
}


sshService::               			~sshService(){

}




coPlugin::t_state sshService::		onBroadcastMessage( coMessage* message ){

// vars
    const char*		            msgGroup = message->group();
	const char*		            msgCommand = message->command();
	const char*		            msgPayload = message->payload();
	json_t*			            jsonPayload = NULL;
	json_t*                     jsonTempValue;
    const char*                 jsonTempString = NULL;
	const char*                 hostName = NULL;
	int                         hostPort = 0;
    int                         serverEnabled = 0;
	json_error_t	            jsonError;
    coCoreConfig::nodeType      type = coCoreConfig::UNKNOWN;



	if( strncmp(msgCommand,"serverConfigGet",10) == 0 ){
        serverConfigGet:

    // lock
        coCore::ptr->config->nodesIterate();

    // build the respond-object
        jsonPayload = json_object();

    // read infos
        if( coCore::ptr->config->nodeSelectByHostName(coCore::ptr->hostNameGet()) == true ){
            coCore::ptr->config->nodeInfo( NULL, &type );
            coCore::ptr->config->nodeConnInfo( &hostName, &hostPort );
            json_object_set_new( jsonPayload, "enabled", json_integer(1) );
            json_object_set_new( jsonPayload, "host", json_string(hostName) );
            json_object_set_new( jsonPayload, "port", json_integer(hostPort) );
        } else {
            coCore::ptr->config->nodeAppend( coCore::ptr->hostNameGet() );
            json_object_set_new( jsonPayload, "enabled", json_integer(0) );
            json_object_set_new( jsonPayload, "host", json_string(coCore::ptr->hostNameGet()) );
            json_object_set_new( jsonPayload, "port", json_integer(4567) );
            coCore::ptr->config->save();
        }

    // unlock
        coCore::ptr->config->nodesIterateFinish();


    // set the message
        msgPayload = json_dumps( jsonPayload, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // add the message to list
        coCore::ptr->plugins->messageAdd( this,
        coCore::ptr->hostNameGet(), msgGroup, "serverConfig", msgPayload );


    // cleanup and return
        free((void*)msgPayload);
        json_decref(jsonPayload);
        return coPlugin::REPLY;
    }


	if( strncmp(msgCommand,"serverSave",10) == 0 ){

	// parse json
		jsonPayload = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
		if( jsonPayload == NULL || jsonError.line > -1 ){
			return coPlugin::NO_REPLY;
		}

    // get host / port
		json_t* jsonHostName = json_object_get( jsonPayload, "host" );
		json_t* jsonHostPort = json_object_get( jsonPayload, "port" );
        json_t* jsonHostEnabled = json_object_get( jsonPayload, "enabled" );

        hostName = json_string_value( jsonHostName );
		jsonTempString = json_string_value( jsonHostPort );
        hostPort = atoi( jsonTempString );
		jsonTempString = json_string_value( jsonHostEnabled );
        serverEnabled = atoi( jsonTempString );

    // lock
        coCore::ptr->config->nodesIterate();

    // read infos
        coCore::ptr->config->nodeSelectByHostName(coCore::ptr->hostNameGet());
        if( serverEnabled == 1 ){
            type = coCoreConfig::SERVER;
        } else {
            type = coCoreConfig::UNKNOWN;
        }
        coCore::ptr->config->nodeInfo( NULL, &type, true );
        coCore::ptr->config->nodeConnInfo( &hostName, &hostPort, true );
        coCore::ptr->config->save();

    // unlock
        coCore::ptr->config->nodesIterateFinish();

	// cleanup
		json_decref(jsonPayload);
        goto serverConfigGet;
		return coPlugin::NO_REPLY;
	}


    if( strncmp(msgCommand,"requestKeysGet",15) == 0 ){
        requestKeysGet:
    // vars
        json_t*         jsonKeys = json_object();

    // get the requested keys as json
        sshService::reqKeysGet( jsonKeys );

    // set the message
        msgPayload = json_dumps( jsonKeys, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // add the message to list
        coCore::ptr->plugins->messageAdd( this,
        coCore::ptr->hostNameGet(), msgGroup, "requestKeys", msgPayload );


    // cleanup and return
        free((void*)msgPayload);
        json_decref(jsonKeys);
        return coPlugin::REPLY;
    }


	if( strncmp(msgCommand,"requestKeyAccept",9) == 0 ){

	// parse json
		sshService::reqKeysAccept( msgPayload );

        goto requestKeysGet;
        return coPlugin::NO_REPLY;
    }


	if( strncmp(msgCommand,"requestKeyRemove",11) == 0 ){

	// parse json
		sshService::reqKeysRemove( msgPayload );

        goto requestKeysGet;
        return coPlugin::NO_REPLY;
    }


    if( strncmp(msgCommand,"acceptedKeysGet",15) == 0 ){
        acceptedKeysGet:
    // vars
        json_t*         jsonKeys = json_object();

    // get the requested keys as json
        sshService::acceptedKeysGet( jsonKeys );

    // set the message
        msgPayload = json_dumps( jsonKeys, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // add the message to list
        coCore::ptr->plugins->messageAdd( this,
        coCore::ptr->hostNameGet(), msgGroup, "acceptedKeys", msgPayload );

    // cleanup and return
        free((void*)msgPayload);
        json_decref(jsonKeys);
        return coPlugin::REPLY;
    }


	if( strncmp(msgCommand,"acceptedKeyRemove",11) == 0 ){

	// parse json
		sshService::acceptedKeysRemove( msgPayload );

        goto acceptedKeysGet;
        return coPlugin::NO_REPLY;
    }



    return coPlugin::NO_REPLY;
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
		coCore::ptr->config->nodeSelect(coCore::ptr->hostNameGet());
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
	keyDirectory = opendir( sshAcceptedKeyPath );
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
		etStringCharSet( fullPath, sshAcceptedKeyPath, -1 );
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

// message "no key found"
    if( returnValue == false ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "No valid key found, move the key to signing request folder..." );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );

    // add the key to requested keys
        sshService::reqKeyAdd( clientKey );

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




unsigned int sshService::           reqKeysCount(){

// vars
    DIR*			    keyDir = NULL;
    dirent*			    keyDirEntry = NULL;
    unsigned int        keyDirCounter = 0;

// open directory
    keyDir = opendir( sshAcceptedKeyPath );
    if( keyDir == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "Could not open key-directory" );
        return UINT_MAX;
    }

// read every key
    keyDirEntry = readdir( keyDir );
    while( keyDirEntry != NULL ){
    // ignore "." and ".."
        if( keyDirEntry->d_name[0] != '.' ){
            keyDirCounter++;
        }
        keyDirEntry = readdir( keyDir );
    }
    closedir(keyDir);


    return keyDirCounter;
}

/**
@brief save public-key to requests-folder

Copilotd save unknown public ssh-keys to an specific folder.
This function export clientKey key to the requests folder

@param clientKey The ssh-key
@return \
 - false if to many keys are inside the request-folder
 - false if clientKey contains no public key
 - false if hash functions fail
 - false if key already exist
 - true if key was successfuly saved
*/
bool sshService::					reqKeyAdd( ssh_key clientKey ){
// vars
	unsigned char* 		hash;
	size_t				hlen;
	char 				answer[512] = { '0' };

// create path if needed
    if( access( sshKeyReqPath, F_OK ) != 0 ){
        system( "mkdir -p " sshKeyReqPath );
    }


// maximum amount of keys reched ?
    if( sshService::reqKeysCount() > 10 ){
        etDebugMessage( etID_LEVEL_ERR, "Maximum amount of requested keys are reached." );
        return false;
    }

// is the key public?
	if( ssh_key_is_public(clientKey) != 1 ){
		return false;
	}

    int test = ssh_key_is_private(clientKey);

// get key-hash
	if( ssh_get_publickey_hash( clientKey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen ) != SSH_OK ){
		return false;
	}
	char* str = ssh_get_hexa( hash, hlen );

// we build the full path
    etString* fullKeyPath; const char* keyPath;
    etStringAllocLen( fullKeyPath, 128 );
    etStringCharSet( fullKeyPath, sshKeyReqPath, -1 );
    etStringCharAdd( fullKeyPath, (const char*)str );
    etStringCharGet( fullKeyPath, keyPath );

// check if the file already exist
    if( access( keyPath, F_OK ) == 0 ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Key %s already exist, do nothing !", keyPath );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

//
    ssh_pki_export_pubkey_file( clientKey, keyPath );
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Key %s saved", keyPath );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// cleanup
    etStringFree( fullKeyPath );
    return true;
}


bool sshService::					reqKeysGet( json_t* jsonObject ){
    if( jsonObject == NULL ) return false;

// vars
    DIR*			keyDir = NULL;
    dirent*			keyDirEntry = NULL;
    json_t*         jsonKey = NULL;
    const char*     msgPayload = NULL;

// open directory
    keyDir = opendir( sshKeyReqPath );
    if( keyDir == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "Could not open key-directory" );
        return coPlugin::NO_REPLY;
    }

// read every key
    keyDirEntry = readdir( keyDir );
    while( keyDirEntry != NULL ){

    // ignore "." and ".."
        if( keyDirEntry->d_name[0] == '.' ) goto nextKey;

    //
        jsonKey = json_object();
        json_object_set_new( jsonObject, keyDirEntry->d_name, jsonKey );

    nextKey:
        keyDirEntry = readdir( keyDir );
    }
    closedir(keyDir);

    return true;
}


bool sshService::					reqKeysRemove( const char* fingerprint ){

// we build the full path
    etString* fullKeyPath; const char* keyPath;
    etStringAllocLen( fullKeyPath, 128 );
    etStringCharSet( fullKeyPath, sshKeyReqPath, -1 );
    etStringCharAdd( fullKeyPath, fingerprint );
    etStringCharGet( fullKeyPath, keyPath );

// remove key
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Remove '%s'", keyPath );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    unlink( keyPath );

// clean
    etStringFree( fullKeyPath );
    return true;
}


bool sshService::					reqKeysAccept( const char* fingerprint ){

// vars
    etString*       sourceKeyPath = NULL;
    etString*       targetKeyPath = NULL;
    const char*     sourcePath = NULL;
    const char*     targetPath = NULL;

// source
    etStringAllocLen( sourceKeyPath, 128 );
    etStringCharSet( sourceKeyPath, sshKeyReqPath, -1 );
    etStringCharAdd( sourceKeyPath, fingerprint );
    etStringCharGet( sourceKeyPath, sourcePath );

// target
    etStringAllocLen( targetKeyPath, 128 );
    etStringCharSet( targetKeyPath, sshAcceptedKeyPath, -1 );
    etStringCharAdd( targetKeyPath, fingerprint );
    etStringCharGet( targetKeyPath, targetPath );

// move
    if( rename( sourcePath, targetPath ) == 0 ){
        return true;
    }

    return false;
}


bool sshService::					acceptedKeysGet( json_t* jsonObject ){
    if( jsonObject == NULL ) return false;

// vars
    DIR*			keyDir = NULL;
    dirent*			keyDirEntry = NULL;
    json_t*         jsonKey = NULL;
    const char*     msgPayload = NULL;

// open directory
    keyDir = opendir( sshAcceptedKeyPath );
    if( keyDir == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "Could not open key-directory" );
        return coPlugin::NO_REPLY;
    }

// read every key
    keyDirEntry = readdir( keyDir );
    while( keyDirEntry != NULL ){

    // ignore "." and ".."
        if( keyDirEntry->d_name[0] == '.' ) goto nextKey;

    //
        jsonKey = json_object();
        json_object_set_new( jsonObject, keyDirEntry->d_name, jsonKey );

    nextKey:
        keyDirEntry = readdir( keyDir );
    }
    closedir(keyDir);

    return true;
}


bool sshService::					acceptedKeysRemove( const char* fingerprint ){

// we build the full path
    etString* fullKeyPath; const char* keyPath;
    etStringAllocLen( fullKeyPath, 128 );
    etStringCharSet( fullKeyPath, sshAcceptedKeyPath, -1 );
    etStringCharAdd( fullKeyPath, fingerprint );
    etStringCharGet( fullKeyPath, keyPath );

// remove key
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Remove '%s'", keyPath );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    unlink( keyPath );

// clean
    etStringFree( fullKeyPath );
    return true;
}



/*
string getClientIp(ssh_session session) {

    struct sockaddr_storage     tmp;
    struct sockaddr_in*         sock;
    unsigned int                len = 100;
    char                        ip[100] = "\0";

    getpeername( ssh_get_fd(session), (struct sockaddr*)&tmp, &len );
    sock = (struct sockaddr_in *)&tmp;
    inet_ntop(AF_INET, &sock->sin_addr, ip, len);

    string ip_str = ip;

    return ip_str;
}
*/

bool sshService::					checkAndCreateServerKeys(){


	FILE*		keyFile;
	ssh_key		sshKey;
	struct 		stat s;
	int 		err;



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

	if( sshService::checkAndCreateServerKeys() == false ){
		return;
	}

// start the thread which wait for clients
	pthread_t thread;
	pthread_create( &thread, NULL, sshService::serveThread, this );
	pthread_detach( thread );

}


void* sshService::					serveThread( void* void_service ){

// vars
	sshService*     service = (sshService*)void_service;
    const char*     serverHost = NULL;
    int             serverPort = 0;
	sshSession*     session = NULL;
    uuid_t          UUID;
    char            UUIDString[37];

// wait for connection limit
	while( service->curConnections >= service->maxConnections ){
		sleep(2);
	}
	service->curConnections++;

// create a new Session
	session = new sshSession();

// create uuid
    uuid_generate( UUID );
    uuid_unparse( UUID, UUIDString );

// get the server infos
    coCore::ptr->config->nodesIterate();
    if( coCore::ptr->config->nodeSelectByHostName(coCore::ptr->hostNameGet()) != true ){
    // debugging message
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "No Server Configuration found, do nothing." );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

        coCore::ptr->config->nodesIterateFinish();
		return NULL;
	}
    coCore::ptr->config->nodeConnInfo( &serverHost, &serverPort );
    coCore::ptr->config->nodesIterateFinish();

// wait for incoming connection
	if( session->waitForClient( serverHost, serverPort ) == false ) return NULL;

// exchange keys and init crypto
	if( session->keyExchange() == false ){
		service->curConnections--;
		return NULL;
	}

// we create an event-loop
	ssh_event mainloop = ssh_event_new();

// poll all events
	session->pollUntilShell( mainloop, 10 );

// node state
    coCoreConfig::nodeStates newState = coCoreConfig::CONNECTED;
    coCore::ptr->config->nodesIterate();
    coCore::ptr->config->nodeSelectByHostName(coCore::ptr->hostNameGet());
    coCore::ptr->config->nodeState( &newState, true );
    coCore::ptr->config->nodesIterateFinish();

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




void sshService::					connect( const char* hostname, int port ){
	// create new session
		sshSession* session = NULL;
		session = new sshSession();
		session->setConnection( port, hostname );

	// start the thread which wait for clients
		pthread_t thread;
		pthread_create( &thread, NULL, sshService::connectToClientThread, session );
		pthread_detach( thread );
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

    // connect to it
        this->connect( clientHost, clientPort );

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
