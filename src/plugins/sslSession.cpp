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




sslSession::                sslSession() : coPlugin( "sslSession", "", "" ){

// allocate memory
    etStringAlloc( this->sessionHost );



// we use an global priority-cache
// this cache selects the cipher in tls

// register plugin
	coCore::ptr->plugins->append( this );

}


sslSession::                ~sslSession(){

// remove string
    etStringFree( this->sessionHost );

// deregister plugin
	coCore::ptr->plugins->remove( this );
}




bool sslSession::           globalInit( const char* myNodeName ){

//
    const char*         configPath = NULL;
    int                 ret = -1;
    std::string         mkdirCommand;
    coCore::ptr->config->configPath( &configPath );


// init tls
    gnutls_global_init();
    gnutls_global_set_log_level( 1 );

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
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not open File '%s'" );
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
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not open File '%s'" );
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
    memset( rawKeyID, rawKeyIDSize, sizeof(unsigned char) );
    memset( outBuffer, *outBufferSize, sizeof(char) );

// get raw size
    gnutls_pubkey_get_key_id( publicKey, 0, rawKeyID, &rawKeyIDSize );


    gnutls_datum_t tmp;
    tmp.data = rawKeyID;
    tmp.size = rawKeyIDSize;

    ret = gnutls_hex_encode( &tmp, outBuffer, outBufferSize );

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
    char            publicKeyBuffer[publicKeyBufferSize];
    size_t          acceptedKeyBufferSize = 1024;
    size_t          acceptedKeyBufferSizeOut = acceptedKeyBufferSize;
    char            acceptedKeyBuffer[acceptedKeyBufferSize];
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


// generate full filename
// get accepted key path
    etStringCharGet( sslSession::pathAcceptedKeys, sslKeyPath );
    fileName  = sslKeyPath;
    fileName += "/";
    fileName += peerHostName;

// first check if the key exist
    if( access( fileName.c_str(), F_OK ) != 0 ){
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
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s fileSize %l", fileName.c_str(), fileSize );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// compare buffer size
    if( fileSizeReaded != publicKeyBufferSizeOut ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s keysize is different: %i <> %i", peerHostName, fileSizeReaded, publicKeyBufferSizeOut );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

// compare buffer contents
    for( bufferIndex = 0; bufferIndex < publicKeyBufferSizeOut; bufferIndex++ ){
        if( publicKeyBuffer[bufferIndex] != acceptedKeyBuffer[bufferIndex] ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s key content differ", peerHostName );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }
    }

    snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: accepted", peerHostName );
    etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
    return true;

pinkey:
    if( pinning == true ){
        file = fopen( fileName.c_str(), "w+" );
        fwrite( publicKeyBuffer, 1, publicKeyBufferSizeOut, file );
        fflush(file);
        fclose(file);

        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: key pinned", peerHostName );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
        return true;
    } else {
        etStringCharGet( sslSession::pathRequestedKeys, sslKeyPath );
        fileName  = sslKeyPath;
        fileName += "/";
        fileName += peerHostName;
        file = fopen( fileName.c_str(), "w+" );
        fwrite( publicKeyBuffer, 1, publicKeyBufferSizeOut, file );
        fflush(file);
        fclose(file);

        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: key requested for acceptence", peerHostName );
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
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not open File '%s'" );
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
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Handshake complete.", gnutls_strerror(ret) );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    }


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

    // add message to queue
        coMessage* message = new coMessage();
        if( message->fromJson( jsonMessage ) == true ){
            message->source( this );

        // an external message should not come from us
            if( coCore::ptr->isHostName( message->nodeNameSource() ) == true ){
                etDebugMessage( etID_LEVEL_ERR, "We recieve a message from another host with our hostname as source. Message will be dropped" );
                delete message;
            } else {
                coCore::ptr->plugins->messageQueue->add( this, message );
            }
        }

    // cleanup
        json_decref( jsonMessage );

        usleep( 5000 );
    }


    return true;
}





coPlugin::t_state sslSession::onBroadcastMessage( coMessage* message ){
    if( this->sessionState < sslSession::CONNECTED ) return NO_REPLY;

// vars
    const char*     myNodeName = coCore::ptr->nodeName();
    const char*     msgSource = message->nodeNameSource();
    const char*     msgTarget = message->nodeNameTarget();
	json_t*			jsonMessage = NULL;
	char*			jsonString = NULL;

// if there is a message for myHost we dont need to send it out to the world
    if( strncmp(msgTarget,myNodeName,strlen(myNodeName)) == 0 ){
        return NO_REPLY;
    }

// create json from message
	jsonMessage = json_object();
	if( message->toJson( jsonMessage ) == false ){
		return NO_REPLY;
	}

// dump / send json
	jsonString = json_dumps( jsonMessage, JSON_PRESERVE_ORDER | JSON_COMPACT );
	if( jsonString != NULL ){

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen,
        "[SEND] [%s -> %s] [%s - %s]",
        msgSource, msgTarget,
        message->group(), message->command() );
        etDebugMessage( etID_LEVEL_DETAIL_NET, etDebugTempMessage );


        gnutls_record_send( this->tlsSession, jsonString, strlen(jsonString) );


    }

// we dont have anything to reply, because the answer comes asynchron
	return coPlugin::NO_REPLY;
}








#endif
