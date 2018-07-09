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

#ifndef sslSession_C
#define sslSession_C

#include "plugins/sslSession.h"
#include "coCore.h"
#include <sys/socket.h>
#include <netdb.h>
#include "pubsub.h"
#include "uuid/uuid.h"

#define SERV_PORT 11111
#define MAX_LINE 4096
#define MAX_BUF 20480


// static stuff
gnutls_priority_t                   sslSession::priorityCache;
gnutls_certificate_credentials_t    sslSession::myCerts;
gnutls_certificate_credentials_t    sslSession::clientCerts;
etString*                           sslSession::pathMyKeys = NULL;
etString*                           sslSession::pathAcceptedKeys = NULL;
etString*                           sslSession::pathRequestedKeys = NULL;

//gnutls_global_deinit ();

int print_info(gnutls_session_t session) {
        const char *tmp;
        gnutls_credentials_type_t cred;
        gnutls_kx_algorithm_t kx;
        int dhe, ecdh;

        dhe = ecdh = 0;

        /* print the key exchange's algorithm name
         */
        kx = gnutls_kx_get(session);
        tmp = gnutls_kx_get_name(kx);
        printf("- Key Exchange: %s\n", tmp);

        /* Check the authentication type used and switch
         * to the appropriate.
         */
        cred = gnutls_auth_get_type(session);
        switch (cred) {
        case GNUTLS_CRD_IA:
                printf("- TLS/IA session\n");
                break;


#ifdef ENABLE_SRP
        case GNUTLS_CRD_SRP:
                printf("- SRP session with username %s\n",
                       gnutls_srp_server_get_username(session));
                break;
#endif

        case GNUTLS_CRD_PSK:
                /* This returns NULL in server side.
                 */
                if (gnutls_psk_client_get_hint(session) != NULL)
                        printf("- PSK authentication. PSK hint '%s'\n",
                               gnutls_psk_client_get_hint(session));
                /* This returns NULL in client side.
                 */
                if (gnutls_psk_server_get_username(session) != NULL)
                        printf("- PSK authentication. Connected as '%s'\n",
                               gnutls_psk_server_get_username(session));

                if (kx == GNUTLS_KX_ECDHE_PSK)
                        ecdh = 1;
                else if (kx == GNUTLS_KX_DHE_PSK)
                        dhe = 1;
                break;

        case GNUTLS_CRD_ANON:  /* anonymous authentication */

                printf("- Anonymous authentication.\n");
                if (kx == GNUTLS_KX_ANON_ECDH)
                        ecdh = 1;
                else if (kx == GNUTLS_KX_ANON_DH)
                        dhe = 1;
                break;

        case GNUTLS_CRD_CERTIFICATE:   /* certificate authentication */

                /* Check if we have been using ephemeral Diffie-Hellman.
                 */
                if (kx == GNUTLS_KX_DHE_RSA || kx == GNUTLS_KX_DHE_DSS)
                        dhe = 1;
                else if (kx == GNUTLS_KX_ECDHE_RSA
                         || kx == GNUTLS_KX_ECDHE_ECDSA)
                        ecdh = 1;

                /* if the certificate list is available, then
                 * print some information about it.
                 */
                //print_x509_certificate_info(session);

        }                       /* switch */

        if (ecdh != 0)
                printf("- Ephemeral ECDH using curve %s\n",
                       gnutls_ecc_curve_get_name(gnutls_ecc_curve_get
                                                 (session)));
        else if (dhe != 0)
                printf("- Ephemeral DH using prime of %d bits\n",
                       gnutls_dh_get_prime_bits(session));

        /* print the protocol's name (ie TLS 1.0)
         */
        tmp =
            gnutls_protocol_get_name(gnutls_protocol_get_version(session));
        printf("- Protocol: %s\n", tmp);

        /* print the certificate type of the peer.
         * ie X.509
         */
        tmp =
            gnutls_certificate_type_get_name(gnutls_certificate_type_get
                                             (session));

        printf("- Certificate Type: %s\n", tmp);

        /* print the compression algorithm (if any)
         */
        tmp = gnutls_compression_get_name(gnutls_compression_get(session));
        printf("- Compression: %s\n", tmp);

        /* print the name of the cipher used.
         * ie 3DES.
         */
        tmp = gnutls_cipher_get_name(gnutls_cipher_get(session));
        printf("- Cipher: %s\n", tmp);

        /* Print the MAC algorithms name.
         * ie SHA1
         */
        tmp = gnutls_mac_get_name(gnutls_mac_get(session));
        printf("- MAC: %s\n", tmp);

        return 0;
}




sslSession::                sslSession() {

// allocate memory
    etStringAlloc( this->sessionHost );


// we use an global priority-cache
// this cache selects the cipher in tls

    this->tlsSession = NULL;


    snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%p]: sslSession", this );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    
}


sslSession::                ~sslSession(){

// remove string
    etStringFree( this->sessionHost );

// close and deinit
    if( this->tlsSession != NULL ){
        gnutls_bye (this->tlsSession, GNUTLS_SHUT_WR);
        close( this->socketChannel );
        this->tlsSession = NULL;
    }
    //gnutls_deinit (this->tlsSession);


}




bool sslSession::           globalInit( const char* myNodeName ){

//
    const char*         configPath = NULL;
    std::string         mkdirCommand;
	int					ret = -1;
    coCore::ptr->config->configPath( &configPath );


// init tls
    gnutls_global_init();
    gnutls_global_set_log_level( 1 );
    
    //gnutls_crypto_init();

// alloc priority cache
    gnutls_priority_init( &sslSession::priorityCache, "PFS", NULL);


// allocate paths
    if( sslSession::pathMyKeys == NULL ){
        etStringAllocLen( sslSession::pathMyKeys, 32 );
        etStringCharSet( sslSession::pathMyKeys, configPath, -1 );
        etStringCharAdd( sslSession::pathMyKeys, "/ssl_private" );
    }
    if( sslSession::pathAcceptedKeys == NULL ){
        etStringAllocLen( sslSession::pathAcceptedKeys, 32 );
        etStringCharSet( sslSession::pathAcceptedKeys, configPath, -1 );
        etStringCharAdd( sslSession::pathAcceptedKeys, "/ssl_accepted" );
    }
    if( sslSession::pathRequestedKeys == NULL ){
        etStringAllocLen( sslSession::pathRequestedKeys, 32 );
        etStringCharSet( sslSession::pathRequestedKeys, configPath, -1 );
        etStringCharAdd( sslSession::pathRequestedKeys, "/ssl_requested" );
    }

// my private keys
    etStringCharGet( sslSession::pathMyKeys, configPath );
	if( access( configPath, F_OK ) != 0 ){
        mkdirCommand  = "mkdir -p ";
        mkdirCommand += configPath;
		ret = system( mkdirCommand.c_str() );
	}
// accepted keys
    etStringCharGet( sslSession::pathAcceptedKeys, configPath );
	if( access( configPath, F_OK ) != 0 ){
        mkdirCommand  = "mkdir -p ";
        mkdirCommand += configPath;
		ret = system( mkdirCommand.c_str() );
	}
// requested keys
    etStringCharGet( sslSession::pathRequestedKeys, configPath );
	if( access( configPath, F_OK ) != 0 ){
        mkdirCommand  = "mkdir -p ";
        mkdirCommand += configPath;
		ret = system( mkdirCommand.c_str() );
	}

// Hint: We use the same key, if we are server or if we are an client, we ( as node ) only have one !
// generate keypair if needed
    const char* sslMyKeyPath = NULL; etStringCharGet( sslSession::pathMyKeys, sslMyKeyPath );
    if( sslSession::generateKeyPair( myNodeName, sslMyKeyPath ) != true ){
        return false;
    }

// load credentials ( keys )
    gnutls_certificate_allocate_credentials( &sslSession::myCerts );
    gnutls_certificate_allocate_credentials( &sslSession::clientCerts );
    sslSession::credCreate( &sslSession::myCerts, myNodeName, sslMyKeyPath, sslSession::verifyPublikKeyOnServerCallback );
    sslSession::credCreate( &sslSession::clientCerts, myNodeName, sslMyKeyPath, sslSession::verifyPublikKeyOnClientCallback );


    return true;
}


bool sslSession::           globalServerInit( const char* serverName ){


    ///@todo deinit functions
    //gnutls_certificate_free_credentials( xcred );
// remember it
    return true;
}




bool sslSession::           import( const char* fileName, gnutls_privkey_t privateKey ){

// vars
    int                     rc;
    FILE*                   fileTemp;
    long int                fileSize;
    gnutls_datum_t          privateKeyData;

// open file
    fileTemp = fopen( fileName, "r" );
    if( fileTemp == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not open File '%s'", fileName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

// read file-size
    fseek( fileTemp, 0L, SEEK_END );
    fileSize = ftell( fileTemp );
    fseek( fileTemp, 0L, SEEK_SET );

// read file into buffer
    char certBuffer[fileSize]; memset( certBuffer, 0, fileSize );
    size_t readedBytes = fread( certBuffer, 1, fileSize, fileTemp );
	privateKeyData.data = (unsigned char*)certBuffer;
	privateKeyData.size = fileSize;


// import to key
    rc = gnutls_privkey_import_x509_raw( privateKey, &privateKeyData, GNUTLS_X509_FMT_PEM, NULL, 0 );
    if( rc != GNUTLS_E_SUCCESS ){
        etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        return false;
    }

    return true;
}


bool sslSession::           import( const char* fileName, gnutls_pubkey_t  publicKey ){

// vars
    int                     rc;
    FILE*                   fileTemp;
    long int                fileSize;
    gnutls_datum_t          publicKeyData;

// open file
    fileTemp = fopen( fileName, "r" );
    if( fileTemp == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not open File '%s'", fileName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

// read file-size
    fseek( fileTemp, 0L, SEEK_END );
    fileSize = ftell( fileTemp );
    fseek( fileTemp, 0L, SEEK_SET );

// read file into buffer
    char certBuffer[fileSize]; memset( certBuffer, 0, fileSize );
    size_t readedBytes = fread( certBuffer, 1, fileSize, fileTemp );
	publicKeyData.data = (unsigned char*)certBuffer;
	publicKeyData.size = fileSize;


// import to key
    rc = gnutls_pubkey_import( publicKey, &publicKeyData, GNUTLS_X509_FMT_PEM );
    if( rc != GNUTLS_E_SUCCESS ){
        etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        return false;
    }


    return true;
}




bool sslSession::           generateKeyPair( const char* name, const char* folder ){

// vars
    int                     rc;
    unsigned int            bits;
    gnutls_privkey_t        privKey;
    gnutls_pubkey_t         pubKey;
    gnutls_x509_privkey_t   x509KeyPrivate;

    FILE*                   fileTemp;
    size_t                  bufferSize = 10240;
    size_t                  bufferSizeOut;
    char                    buffer[bufferSize];
    const char*             errorString = NULL;
    std::string             tempFilePath;
    std::string             tempString;

// get the amount of bits according to security level
    bits = gnutls_sec_param_to_pk_bits( GNUTLS_PK_ECDSA, GNUTLS_SEC_PARAM_HIGH );

// ######################################### Pivate key #########################################
    tempFilePath  = folder;
    tempFilePath += "/";
    tempFilePath += name;
    tempFilePath += "-key.pem";
    if( access( tempFilePath.c_str(), F_OK ) != 0 ){

        gnutls_privkey_init( &privKey );
        gnutls_privkey_generate( privKey, GNUTLS_PK_ECDSA, bits, 0 );
        rc = gnutls_privkey_export_x509( privKey, &x509KeyPrivate );
        if( rc != GNUTLS_E_SUCCESS ){
            etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        }

        memset( buffer, 0, sizeof(buffer) );
        bufferSizeOut = bufferSize;
        rc = gnutls_x509_privkey_export( x509KeyPrivate, GNUTLS_X509_FMT_PEM, buffer, &bufferSizeOut );
        if( rc != GNUTLS_E_SUCCESS ){
            etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        }

    // write it to file
        fileTemp = fopen( tempFilePath.c_str(), "w+" );
        if( fileTemp == NULL ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not create key '%s'", tempFilePath.c_str() );
            etDebugMessage( etID_LEVEL_CRITICAL, etDebugTempMessage );
            exit(-1);
        }
        fprintf( fileTemp, "%s", buffer );
        fflush( fileTemp );
        fclose( fileTemp );
    } else {

        gnutls_privkey_init( &privKey );

        sslSession::import( tempFilePath.c_str(), privKey );

        /*
        rc = gnutls_privkey_export_x509( privKey, &x509KeyPrivate );
        if( rc != GNUTLS_E_SUCCESS ){
            etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        }


        memset( buffer, 0, sizeof(buffer) );
        bufferSizeOut = bufferSize;
        rc = gnutls_x509_privkey_export( x509KeyPrivate, GNUTLS_X509_FMT_PEM, buffer, &bufferSizeOut );
        if( rc != GNUTLS_E_SUCCESS ){
            etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        }
        */

    }


// ######################################### public key #########################################
    tempFilePath  = folder;
    tempFilePath += "/";
    tempFilePath += name;
    tempFilePath += "-pub.pem";
    if( access( tempFilePath.c_str(), F_OK ) != 0 ){

    // get public key
        gnutls_pubkey_init( &pubKey );
        rc = gnutls_pubkey_import_privkey( pubKey, privKey, 0, 0 );
        if( rc != GNUTLS_E_SUCCESS ){
            etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        }

    // read public-key to buffer
        memset( buffer, 0, sizeof(buffer) );
        bufferSizeOut = bufferSize;
        rc = gnutls_pubkey_export( pubKey, GNUTLS_X509_FMT_PEM, buffer, &bufferSizeOut );
        if( rc != GNUTLS_E_SUCCESS ){
            etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        }

    // write it to file
        fileTemp = fopen( tempFilePath.c_str(), "w+" );
        if( fileTemp == NULL ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not create key '%s'", tempFilePath.c_str() );
            etDebugMessage( etID_LEVEL_CRITICAL, etDebugTempMessage );
            exit(-1);
        }
        fprintf( fileTemp, "%s", buffer );
        fflush( fileTemp );
        fclose( fileTemp );

    // cleanup
        gnutls_pubkey_deinit( pubKey );
        gnutls_privkey_deinit( privKey );
    }

// ######################################### Template #########################################
    tempFilePath  = folder;
    tempFilePath += "/";
    tempFilePath += name;
    tempFilePath += ".cfg";
    if( access( tempFilePath.c_str(), F_OK ) != 0 ){
        tempString  = "# The organization of the subject.\n";
        tempString += "# organization = \"Copilot Org\"\n\n";

        tempString += "# The organizational unit of the subject.\n";
        tempString += "# unit = \"\"\n\n";

        tempString += "# The state of the certificate owner.\n";
        tempString += "# state = \"\"\n\n";

        tempString += "# The country of the subject. Two letter code.\n";
        tempString += "# country = DE\n\n";

        tempString += "# # The common name of the certificate owner.\n";
        tempString += "cn = \"";
        tempString += name;
        tempString += "\"\n\n";

        tempString += "expiration_days = -1";

        fileTemp = fopen( tempFilePath.c_str(), "w+" );
        if( fileTemp == NULL ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not create key '%s'", tempFilePath.c_str() );
            etDebugMessage( etID_LEVEL_CRITICAL, etDebugTempMessage );
            exit(-1);
        }
        fwrite( tempString.c_str(), 1, tempString.length(), fileTemp );
        fflush( fileTemp );
        fclose( fileTemp );
    }



// ######################################### Certificate #########################################
    tempFilePath  = folder;
    tempFilePath += "/";
    tempFilePath += name;
    tempFilePath += "-cert.pem";
    if( access( tempFilePath.c_str(), F_OK ) != 0 ){
    // create command
        tempString  = "/usr/bin/certtool ";
        tempString += "--generate-self-signed ";

        tempString += "--load-privkey ";
        tempString += folder;
        tempString += "/";
        tempString += name;
        tempString += "-key.pem ";

        tempString += "--template ";
        tempString += folder;
        tempString += "/";
        tempString += name;
        tempString += ".cfg ";

        tempString += "--outfile ";
        tempString += tempFilePath;

    // run command
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s", tempString.c_str() );
        etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );
        rc = system( etDebugTempMessage );

        if( rc != 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Failed to create certificate." );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }
    }

    return true;
}




bool sslSession::           credCreate( gnutls_certificate_credentials_t* xcred, const char* name, const char* folder, gnutls_certificate_verify_function* func ){

// vars
    int             rc;
    std::string     certFilePath;
    std::string     privKeyFilePath;


    certFilePath  = folder;
    certFilePath += "/";
    certFilePath += name;
    certFilePath += "-cert.pem";

    privKeyFilePath  = folder;
    privKeyFilePath += "/";
    privKeyFilePath += name;
    privKeyFilePath += "-key.pem";


// set key / cert
    rc = gnutls_certificate_set_x509_key_file( *xcred, certFilePath.c_str(), privKeyFilePath.c_str(), GNUTLS_X509_FMT_PEM );
    if( rc != GNUTLS_E_SUCCESS ){
        gnutls_strerror( rc );
        return false;
    }

// set diffie hellman key exchange
    gnutls_certificate_set_known_dh_params( *xcred, GNUTLS_SEC_PARAM_MEDIUM );

// setup callback function
    if( func != NULL ){
        gnutls_certificate_set_verify_function( *xcred, func );
    }


    return true;
}


bool sslSession::           pubKeyGetId( gnutls_pubkey_t publicKey, char* outBuffer, size_t* outBufferSize ){

// vars
    size_t              rawKeyIDSize = 21;
    unsigned char       rawKeyID[rawKeyIDSize];
    int                 ret;

// clean
    memset( rawKeyID, 0, rawKeyIDSize * sizeof(unsigned char) );
    memset( outBuffer, 0, *outBufferSize * sizeof(char) );

// get raw size
    gnutls_pubkey_get_key_id( publicKey, GNUTLS_KEYID_USE_SHA1, rawKeyID, &rawKeyIDSize );


    gnutls_datum_t tmp;
    tmp.data = rawKeyID;
    tmp.size = rawKeyIDSize;

    ret = gnutls_hex_encode( &tmp, outBuffer, outBufferSize );
	if( ret == 0 ){
		return true;
	}
	
	return false;
}


bool sslSession::           checkAcceptedKey( gnutls_pubkey_t publicKey, const char* peerHostName, bool pinning ){

// vars
    int             rc;
    std::string     fileName;
    FILE*           file;
    long int        fileSize;
    size_t          fileSizeReaded;
    size_t          publicKeyBufferSize = 1024;
    size_t          publicKeyBufferSizeOut = publicKeyBufferSize;
    char            publicKeyBuffer[publicKeyBufferSize]; memset( publicKeyBuffer, 0, publicKeyBufferSize );
    size_t          acceptedKeyBufferSize = 1024;
    char            acceptedKeyBuffer[acceptedKeyBufferSize]; memset( acceptedKeyBuffer, 0, acceptedKeyBufferSize );
    size_t          bufferIndex = 0;
    const char*     sslKeyPath = NULL;

// clean
    memset( publicKeyBuffer, 0, publicKeyBufferSize );
    memset( acceptedKeyBuffer, 0, acceptedKeyBufferSize );

// get the key as pem
    rc = gnutls_pubkey_export( publicKey, GNUTLS_X509_FMT_PEM, publicKeyBuffer, &publicKeyBufferSizeOut );
    if( rc != GNUTLS_E_SUCCESS ){
        etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        return false;
    }


// get key id
    size_t keyIDHexSize = 50;
    char keyIDHex[keyIDHexSize];
    sslSession::pubKeyGetId( publicKey, keyIDHex, &keyIDHexSize );

// generate full filename
// get accepted key path
    etStringCharGet( sslSession::pathAcceptedKeys, sslKeyPath );
    fileName  = sslKeyPath;
    fileName += "/";
    fileName += keyIDHex;

// first check if the key exist
    if( access( fileName.c_str(), F_OK ) != 0 ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] File '%s' dont exist.", keyIDHex, fileName.c_str() );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
        goto pinkey;
    }

// open file
    file = fopen( fileName.c_str(), "r" );
    if( file == NULL ){
        return false;
    }

// get file size
    fseek( file, 0L, SEEK_END );
    fileSize = ftell( file );
    fseek( file, 0L, SEEK_SET );

// read file into buffer
    fileSizeReaded = fread( acceptedKeyBuffer, 1, fileSize, file );
    fclose( file );

// print file size
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] %s fileSize %ld", keyIDHex, fileName.c_str(), fileSize );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// compare buffer size
    if( fileSizeReaded != publicKeyBufferSizeOut ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] %s keysize is different: %ld <> %ld", keyIDHex, peerHostName, fileSizeReaded, publicKeyBufferSizeOut );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

// compare buffer contents
    for( bufferIndex = 0; bufferIndex < publicKeyBufferSizeOut; bufferIndex++ ){
        if( publicKeyBuffer[bufferIndex] != acceptedKeyBuffer[bufferIndex] ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] %s key content differ", keyIDHex, peerHostName );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }
    }

    snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] accepted", keyIDHex );
    etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
    return true;

pinkey:
    if( pinning == true ){

		file = fopen( fileName.c_str(), "w+" );
		if( file == NULL ){
			snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not create file %s", fileName.c_str() );
			etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
			return false;
		}

        fwrite( publicKeyBuffer, 1, publicKeyBufferSizeOut, file );
        fflush(file);
        fclose(file);

        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] key pinned", keyIDHex );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
        return true;
    } else {
        etStringCharGet( sslSession::pathRequestedKeys, sslKeyPath );
        fileName  = sslKeyPath;
        fileName += "/";
        fileName += keyIDHex;

		file = fopen( fileName.c_str(), "w+" );
		if( file == NULL ){
			snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] Could not create file %s", keyIDHex, fileName.c_str() );
			etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
			return false;
		}

		fwrite( publicKeyBuffer, 1, publicKeyBufferSizeOut, file );
        fflush(file);
        fclose(file);

        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] %s: key requested for acceptence", keyIDHex, peerHostName );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
        return false;
    }

    return false;
}




int sslSession::            verifyPublikKeyCallback( gnutls_session_t session, bool pinning ){


    unsigned int                certStatus;
    const gnutls_datum_t*       certList;
    unsigned int                certListSize;
    gnutls_pubkey_t             publicKey;
    int ret;
    gnutls_x509_crt_t cert;
    const char *hostname;

/* read hostname */
    hostname = (const char*)gnutls_session_get_ptr( session );
    if( hostname == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "No hostname was provided" );
        return GNUTLS_E_CERTIFICATE_ERROR;
    }
//
    certList = gnutls_certificate_get_peers( session, &certListSize );
    if( certList == NULL ){
            printf("No certificate was found!\n");
            return GNUTLS_E_CERTIFICATE_ERROR;
    }

// get the public key
    gnutls_pubkey_init( &publicKey );
    ret = gnutls_pubkey_import_x509_raw( publicKey, &certList[0], GNUTLS_X509_FMT_DER, 0 );

// get key id
    size_t keyIDHexSize = 100;
    char keyIDHex[keyIDHexSize];
    sslSession::pubKeyGetId( publicKey, keyIDHex, &keyIDHexSize );

    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Key ID of %s: %s", hostname, keyIDHex );
    etDebugMessage( etID_LEVEL_INFO, etDebugTempMessage );

// check if key is accepted
    if( sslSession::checkAcceptedKey( publicKey, hostname, pinning ) == true ){
        return 0;
    }

// key was not accepted
    return GNUTLS_E_CERTIFICATE_ERROR;

}



bool sslSession::           hashMessage( const char* message, char* outputBuffer, size_t outputBufferSize ){
    
    size_t              hashHexSize = 30;
    unsigned char       hashHexBuffer[hashHexSize];
    memset( hashHexBuffer, 0, hashHexSize );
    
    
    gnutls_hash_fast( GNUTLS_DIG_SHA1, message, strlen(message), hashHexBuffer );

    gnutls_datum_t tmp;
    tmp.data = hashHexBuffer;
    tmp.size = 20;

    int ret = gnutls_hex_encode( &tmp, outputBuffer, &outputBufferSize );
    if( ret != GNUTLS_E_SUCCESS ){
        etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( ret ) );
        return false;
    }
    
    return true;
}






int sslSession::            port( int port ){
    if( port > 0 ){
        this->sessionPort = port;
    }

    return this->sessionPort;
}


const char* sslSession::    host( const char* hostName ){

// vars
    const char* hostNameToReturn = NULL;

    if( hostName != NULL ){
        etStringCharSet( this->sessionHost, hostName, -1 );
    }

    etStringCharGet( this->sessionHost, hostNameToReturn );

    return hostNameToReturn;
}



bool sslSession::           certInfo( const char* fileName ){

// vars
    int                     rc;
    FILE*                   fileTemp;
    long int                fileSize;



// open file
    fileTemp = fopen( fileName, "r" );
    if( fileTemp == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not open File '%s'", fileName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

// read file-size
    fseek( fileTemp, 0L, SEEK_END );
    fileSize = ftell( fileTemp );
    fseek( fileTemp, 0L, SEEK_SET );

// read file into buffer
    char certBuffer[fileSize]; memset( certBuffer, 0, fileSize );
    size_t readedBytes = fread( certBuffer, 1, fileSize, fileTemp );

    gnutls_datum_t pem;
	pem.data = (unsigned char*)certBuffer;
	pem.size = fileSize;

// close file
    fclose( fileTemp );

// create certificate
    gnutls_x509_crt_t crt;
    rc = gnutls_x509_crt_init (&crt);

// import crt from filebuffer
    rc = gnutls_x509_crt_import( crt, &pem, GNUTLS_X509_FMT_PEM );
    if( rc != GNUTLS_E_SUCCESS ){
        etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
        return false;
    }

// just a test
    size_t cnBufferSiue = 1024;
    char cnBuffer[cnBufferSiue]; memset( cnBuffer, 0, cnBufferSiue );
    rc = gnutls_x509_crt_get_dn_by_oid( crt, GNUTLS_OID_X520_COMMON_NAME, 0, 0, cnBuffer, &cnBufferSiue );
    if( rc != GNUTLS_E_SUCCESS ){
        etDebugMessage( etID_LEVEL_ERR, gnutls_strerror( rc ) );
    }

    return true;
}


bool sslSession::           sendJson( json_t* jsonObject ){

// vars
    const char*     myNodeName = coCore::ptr->nodeName();
    const char*     msgID = NULL;
    const char*     msgSource = NULL;
    const char*     msgTarget = NULL;
    const char*     msgGroup = NULL;
    const char*     msgCommand = NULL;
    char*           jsonString = NULL;

// connected ?
    if( this->sessionState < sslSession::CONNECTED ) return false;

// get source/target from json
    psBus::fromJson( jsonObject, &msgID, &msgSource, &msgTarget, &msgGroup, &msgCommand, NULL );

// if there is a message for myHost we dont need to send it out to the world
    if( strncmp(msgTarget,myNodeName,strlen(myNodeName)) == 0 ){
        return false;
    }


// make sure we are the source
    json_object_set_new( jsonObject, "s", json_string( coCore::ptr->nodeName() ) );

// dump / send json
    jsonString = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
    if( jsonString != NULL ){

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen,
        "[SEND] [%s -> %s] [%s - %s]",
        msgSource, msgTarget,
        msgGroup, msgCommand );
        etDebugMessage( etID_LEVEL_DETAIL_NET, etDebugTempMessage );

    // send it out
        gnutls_record_send( this->tlsSession, jsonString, strlen(jsonString) );

    // cleanup 
        free( jsonString );

    }

// we dont have anything to reply, because the answer comes asynchron
    return true;
}




bool sslSession::           handleClient(){

// vars
    int                     sd, ret;
    char                    buffer[MAX_BUF + 1];
    const char*             clientHostName;


// init out tls-connection
    gnutls_init( &this->tlsSession, GNUTLS_SERVER );
    gnutls_priority_set( this->tlsSession, sslSession::priorityCache );
    gnutls_credentials_set( this->tlsSession, GNUTLS_CRD_CERTIFICATE, sslSession::myCerts );

// we request the client certificate to verify it
    gnutls_certificate_server_set_request( this->tlsSession, GNUTLS_CERT_REQUEST );

// call
    if( this->newPeerCallbackFunct != NULL ){
        this->newPeerCallbackFunct( this->userdata );
    }

 // set pointer for session to hostname
    clientHostName = this->host(NULL);
    gnutls_session_set_ptr( this->tlsSession, (void*)clientHostName );


// gnu tls use the socket
    gnutls_transport_set_int( this->tlsSession, this->socketChannel );


// communication
    this->sessionState = sslSession::CONNECTED_INC;
    this->communicate();
    this->sessionState = sslSession::DISCONNECTED;


    gnutls_bye (this->tlsSession, GNUTLS_SHUT_WR);
    close( this->socketChannel );
    gnutls_deinit (this->tlsSession);
    this->tlsSession = NULL;

onerror:
    close( this->socketChannel );


    return 0;
}


bool sslSession::           client(){

// vars
    int ret, ii;
    char                                    buffer[MAX_BUF + 1];
    const char*                             err;
    struct sockaddr_in                      clientSocketAddress;
    const char*                             clientHostName;

// init gnutls  with client certificates
    gnutls_init( &this->tlsSession, GNUTLS_CLIENT );
    gnutls_priority_set( this->tlsSession, sslSession::priorityCache );
    gnutls_credentials_set( this->tlsSession, GNUTLS_CRD_CERTIFICATE, sslSession::clientCerts );

// save info
    clientHostName = this->host(NULL);
    gnutls_server_name_set( this->tlsSession, GNUTLS_NAME_DNS, clientHostName, strlen(clientHostName) );


// save info
    clientHostName = this->host(NULL);
    gnutls_session_set_ptr( this->tlsSession, (void*)clientHostName );

// gnu tls use the socket
    gnutls_transport_set_int( this->tlsSession, this->socketChannel );


// communication
    this->sessionState = sslSession::CONNECTED_OUT;
    this->communicate();
    this->sessionState = sslSession::DISCONNECTED;


    gnutls_bye (this->tlsSession, GNUTLS_SHUT_RDWR);

onerror:


    gnutls_deinit (this->tlsSession);
    this->tlsSession = NULL;



    return 0;
}


bool sslSession::           communicate(){

// vars
    int                     ret = -1;
    char                    buffer[MAX_BUF + 1];
	json_error_t	        jsonError;
	json_t*			        jsonMessage = NULL;

// handshake
    do { ret = gnutls_handshake( this->tlsSession ); }
    while( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

// failed ?
    if (ret < 0) {
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Handshake failed: %s", gnutls_strerror(ret) );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    } else {
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Handshake complete." );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    }


// We subscribe on the remote-node for our node
    json_t*     jsonTempRequest = NULL;
    char        jsonTempRequestString[128];
    uuid_t      newUUID;
    char        newUUIDString[37];
        

    etDebugMessage( etID_LEVEL_DETAIL_APP, "send subscriptions to remote" );

    memset( jsonTempRequestString, 0, 128 );
    snprintf( jsonTempRequestString, 128, "{ \"t\": \"%s\" }", coCore::ptr->nodeName() );

// generate an uuid
    uuid_generate( newUUID );
    uuid_unparse( newUUID, newUUIDString );
    psBus::toJson( &jsonTempRequest, newUUIDString, "", "", "bus", "subscribe", jsonTempRequestString );
    this->sendJson( jsonTempRequest );
    json_decref(jsonTempRequest);

    uuid_generate( newUUID );
    uuid_unparse( newUUID, newUUIDString );
    psBus::toJson( &jsonTempRequest, newUUIDString, "", "", "bus", "subscribe", "{ \"t\": \"all\" }" );
    this->sendJson( jsonTempRequest );
    json_decref(jsonTempRequest);


    for (;;){

    // recieve data ( unblocked
        memset( buffer, 0, MAX_BUF + 1 );
        ret = gnutls_record_recv( this->tlsSession, buffer, MAX_BUF );

    // close
        if( ret == 0 ){
            etDebugMessage( etID_LEVEL_ERR, "Peer has closed the TLS connection" );
            return false;
        }
    // Error
        else if( ret < 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error: %s", gnutls_strerror (ret) );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Received %s", buffer );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );


    // try to parse the data into json
        jsonMessage = json_loads( (const char*)buffer, JSON_PRESERVE_ORDER, &jsonError );
        if( jsonMessage == NULL || jsonError.line >= 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "JSON ERROR: %s", jsonError.text );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            continue;
        }
        
/*
    // did we recieve a remote node name ?
        if( this->remoteNoteName == NULL ){
            
        // check jsonMessage for nodename
            json_t* jsonNodeName = NULL;
            const char*     msgCommand = NULL;
            const char*     msgPayload = NULL;
            
            psBus::fromJson( jsonMessage, NULL, NULL, NULL, NULL, &msgCommand, &msgPayload );

        // request of nodename ?
            if( strncmp(msgCommand,"getNodeName",11) == 0 ){
            // we dont need anymore the old message
                json_decref(jsonMessage);
                
                etDebugMessage( etID_LEVEL_DETAIL_APP, "Send Nodename to peer" );
                psBus::toJson( &jsonMessage, "", "", "", "", "nodename", coCore::ptr->nodeName() );
                sslSession::psSubscriberJsonMessage( jsonMessage, this );
                json_decref(jsonMessage);
                usleep( 5000 );
                continue;
            }

        // we get a node name, yeah
            if( strncmp(msgCommand,"nodename",8) == 0 ){
                etStringAlloc( this->remoteNoteName );
                etStringCharSet( this->remoteNoteName, msgPayload, -1 );
                
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "Get nodename '%s' from peer", msgPayload );
                etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
            }
            
            
            json_decref(jsonMessage);
            usleep( 5000 );
            continue;
        }
*/
        
    // we subscribe ?
        const char* nodeSource, *nodeID, *nodeCmd;
        psBus::fromJson( jsonMessage, &nodeID, &nodeSource, NULL, NULL, &nodeCmd, NULL );
        

    // an external message should not come from us
        if( coCore::ptr->isNodeName( nodeSource ) == true ){
            etDebugMessage( etID_LEVEL_WARNING, "We recieve a message from another host with our hostname as source. Message will be dropped" );
        } else {

            psBus::inst->publishOrSubscribe( this, jsonMessage, NULL, NULL, sslSession::onSubscriberMessage );

        }


    // cleanup
        json_decref( jsonMessage );

        usleep( 5000 );
    }


    return true;
}





int sslSession::            onSubscriberMessage( void* objectSource, json_t* jsonObject, void* userdata ){

// vars
    sslSession*     sslSessionInst = (sslSession*)objectSource;

// send it out
    sslSessionInst->sendJson( jsonObject );

// we dont have anything to reply, because the answer comes asynchron
	return 0;
}






#endif
