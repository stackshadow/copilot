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

#ifndef wolfssl_C
#define wolfssl_C

#include "plugins/gnutls.h"
#include "coCore.h"

#define SERV_PORT 11111
#define MAX_LINE 4096
#define MAX_BUF 1024

// create var
gnutls_dh_params_t sslService::dhParams;

void my_log_func( int level, const char* message ){
    etDebugMessage( etID_LEVEL_WARNING, message );
}

sslService::                sslService() : coPlugin( "sslService", coCore::ptr->hostNameGet(), "cocom" ){

// check core-config path
	if( access( wsslServerKeyPath, F_OK ) != 0 ){
		system( "mkdir -p " wsslServerKeyPath );
	}
	if( access( wsslKeyReqPath, F_OK ) != 0 ){
		system( "mkdir -p " wsslKeyReqPath );
	}
	if( access( wsslAcceptedKeyPath, F_OK ) != 0 ){
		system( "mkdir -p " wsslAcceptedKeyPath );
	}
	if( access( wsslClientKeyPath, F_OK ) != 0 ){
		system( "mkdir -p " wsslClientKeyPath );
	}

    //Cert        myCert;
// vars
int rc;
    unsigned int bits;

// create cert
    //InitCert(&myCert);

    gnutls_global_set_log_level( 10 );
    gnutls_global_set_log_function( my_log_func );
/* Generate Diffie-Hellman parameters - for use with DHE
kx algorithms. When short bit length is used, it might
be wise to regenerate parameters often.
*/
    bits = gnutls_sec_param_to_pk_bits( GNUTLS_PK_DH, GNUTLS_SEC_PARAM_NORMAL );
    gnutls_dh_params_init (&this->dhParams);
    gnutls_dh_params_generate2 (this->dhParams, bits);


int c = 0;
//fprintf (outfile, "\tPublic Key Algorithm: ");
//cprint = gnutls_pk_algorithm_get_name (ret);
//fprintf (outfile, "\n%s\n", buffer);
}

sslService::                ~sslService(){

}


bool sslService::           certInfo( const char* fileName ){

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




bool sslService::           import( const char* fileName, gnutls_privkey_t privateKey ){

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


bool sslService::           import( const char* fileName, gnutls_pubkey_t  publicKey ){

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





bool sslService::           generateKeyPair( const char* name, const char* folder ){

// vars
    int                     rc;
    unsigned int            bits;
    gnutls_privkey_t        privKey;
    gnutls_pubkey_t         pubKey;
    gnutls_x509_privkey_t   x509KeyPrivate;
    //gnutls_x509_privkey_t   x509KeyPublic;
    FILE*                   fileTemp;
    size_t                  bufferSize = 10240;
    size_t                  bufferSizeOut;
    char                    buffer[bufferSize];
    const char*             errorString = NULL;
    std::string             tempFilePath;
    std::string             tempSubject;

// get the amount of bits according to security level
    bits = gnutls_sec_param_to_pk_bits( GNUTLS_PK_ECDSA, GNUTLS_SEC_PARAM_HIGH );

// ######################################### Pivate key #########################################
    tempFilePath  = folder;
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

        this->import( tempFilePath.c_str(), privKey );

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
    tempFilePath += name;
    tempFilePath += ".cfg";
    if( access( tempFilePath.c_str(), F_OK ) != 0 ){
        tempSubject  = "# The organization of the subject.\n";
        tempSubject += "# organization = \"Copilot Org\"\n\n";

        tempSubject += "# The organizational unit of the subject.\n";
        tempSubject += "# unit = \"\"\n\n";

        tempSubject += "# The state of the certificate owner.\n";
        tempSubject += "# state = \"\"\n\n";

        tempSubject += "# The country of the subject. Two letter code.\n";
        tempSubject += "# country = DE\n\n";

        tempSubject += "# # The common name of the certificate owner.\n";
        tempSubject += "cn = \"";
        tempSubject += name;
        tempSubject += "\"\n\n";

        tempSubject += "expiration_days = -1";

        fileTemp = fopen( tempFilePath.c_str(), "w+" );
        fwrite( tempSubject.c_str(), 1, tempSubject.length(), fileTemp );
        fflush( fileTemp );
        fclose( fileTemp );
    }



// ######################################### Certificate #########################################
    tempFilePath  = folder;
    tempFilePath += name;
    tempFilePath += "-cert.pem";
    if( access( tempFilePath.c_str(), F_OK ) != 0 ){
    // create command
        tempSubject  = "/usr/bin/certtool ";
        tempSubject += "--generate-self-signed ";

        tempSubject += "--load-privkey ";
        tempSubject += folder;
        tempSubject += name;
        tempSubject += "-key.pem ";

        tempSubject += "--template ";
        tempSubject += folder;
        tempSubject += name;
        tempSubject += ".cfg ";

        tempSubject += "--outfile ";
        tempSubject += tempFilePath;

    // run command
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s", tempSubject.c_str() );
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



bool sslService::           credCreate( gnutls_certificate_credentials_t* xcred, const char* certFile, const char* privKey, gnutls_certificate_verify_function* func ){

// vars
    int rc;

// allocate memory
/*
    rc = gnutls_certificate_allocate_credentials( xcred );
    if( rc != GNUTLS_E_SUCCESS ){
        gnutls_strerror( rc );
        return false;
    }
*/

// set trusted file
/*
    rc = gnutls_certificate_set_x509_trust_file( *xcred, CAFILE, GNUTLS_X509_FMT_PEM);
    if( rc != GNUTLS_E_SUCCESS ){
        gnutls_strerror( rc );
        return false;
    }
*/
// set key / cert
    rc = gnutls_certificate_set_x509_key_file( *xcred, certFile, privKey, GNUTLS_X509_FMT_PEM );
    if( rc != GNUTLS_E_SUCCESS ){
        gnutls_strerror( rc );
        return false;
    }

// setup callback function
    if( func != NULL ){
        gnutls_certificate_set_verify_function( *xcred, func );
    }


    return true;
}


#define CHECK(x) assert((x)>=0)


/* This function will verify the peer's certificate, and check
 * if the hostname matches, as well as the activation, expiration dates.
 */
static int
_verify_certificate_callback (gnutls_session_t session)
{


    unsigned int              certStatus;
    const gnutls_datum_t*     cert_list;
    unsigned int cert_list_size;
    int ret;
    gnutls_x509_crt_t cert;
    const char *hostname;

/* read hostname */
    hostname = (const char*)gnutls_session_get_ptr (session);

/* This verification function uses the trusted CAs in the credentials
* structure. So you must have installed one or more CA certificates.
*/
    ret = gnutls_certificate_verify_peers2( session, &certStatus );
    if (ret < 0) {
        printf ("Error\n");
        return GNUTLS_E_CERTIFICATE_ERROR;
    }

    gnutls_certificate_type_t type = gnutls_certificate_type_get( session );

    if (certStatus & GNUTLS_CERT_SIGNER_NOT_FOUND)
        printf ("The certificate hasn't got a known issuer.\n");

    if (certStatus & GNUTLS_CERT_REVOKED)
        printf ("The certificate has been revoked.\n");

    if (certStatus & GNUTLS_CERT_EXPIRED)
        printf ("The certificate has expired\n");

    if (certStatus & GNUTLS_CERT_NOT_ACTIVATED){
        printf ("The certificate is not yet activated\n");
    }

    if (certStatus & GNUTLS_CERT_INVALID) {
        printf ("The certificate is not trusted...yet\n");

        unsigned int cert_list_size;
        const gnutls_datum_t* cert_list = gnutls_certificate_get_peers( session, &cert_list_size );
        if( cert_list == NULL ){
                printf("No certificate was found!\n");
                return GNUTLS_E_CERTIFICATE_ERROR;
        }

        ret = gnutls_verify_stored_pubkey( "/tmp/gnutls.key", NULL, hostname, "copilot", type, &cert_list[0], 0);
        if( ret == GNUTLS_E_NO_CERTIFICATE_FOUND ){

            // not found, save it
            ret = gnutls_store_pubkey( "/tmp/gnutls.key", NULL, hostname, "copilot", type, &cert_list[0], 0, 0);
            if( ret != GNUTLS_E_SUCCESS ){
                gnutls_strerror( ret );
                return GNUTLS_E_CERTIFICATE_ERROR;
            }

        }
        else if( ret == GNUTLS_E_CERTIFICATE_KEY_MISMATCH ){
            // key mismatch
            printf( "Error, key was found but mismatch !" );
            return GNUTLS_E_CERTIFICATE_ERROR;
        }


    }

/*
  if (!gnutls_x509_crt_check_hostname (cert, hostname))
    {
      printf ("The certificate's owner does not match hostname '%s'\n",
              hostname);
      return GNUTLS_E_CERTIFICATE_ERROR;
    }
*/
  //gnutls_x509_crt_deinit (cert);

  /* notify gnutls to continue handshake normally */
  return 0;
}


bool sslService::           serve(){

// vars
    int                     listen_sd;
    int                     sd, ret;
    struct sockaddr_in      sa_serv;
    struct sockaddr_in      sa_cli;
    socklen_t               client_len;
    char                    topbuf[512];
    gnutls_session_t        session;
    char                    buffer[MAX_BUF + 1];
    int                     optval = 1;

    /* this must be called once in the program
    */
    gnutls_global_init ();

    gnutls_certificate_allocate_credentials (&this->x509Cred);


    /* gnutls_certificate_set_x509_system_trust(xcred);
    gnutls_certificate_set_x509_trust_file (this->x509Cred, CAFILE,
                                          GNUTLS_X509_FMT_PEM);*/

/*
    gnutls_certificate_set_x509_crl_file (this->x509Cred, CRLFILE,
                                        GNUTLS_X509_FMT_PEM);
*/
    this->generateKeyPair( "server", wsslServerKeyPath );
    this->credCreate( &this->x509Cred, wsslServerKeyPath "server-cert.pem", wsslServerKeyPath "server-key.pem", NULL );

    if (ret < 0)
    {
      printf("No certificate or key were found\n");
      exit(1);
    }


    gnutls_priority_init (&priority_cache, "PERFORMANCE:%SERVER_PRECEDENCE", NULL);


    gnutls_certificate_set_dh_params( this->x509Cred, this->dhParams );

    /* Socket operations
    */
    listen_sd = socket (AF_INET, SOCK_STREAM, 0);

    memset (&sa_serv, '\0', sizeof (sa_serv));
    sa_serv.sin_family = AF_INET;
    sa_serv.sin_addr.s_addr = INADDR_ANY;
    sa_serv.sin_port = htons( this->port );      /* Server Port number */

    setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (void *) &optval,
              sizeof (int));

    bind( listen_sd, (struct sockaddr *) & sa_serv, sizeof (sa_serv));

    listen (listen_sd, 1024);

    printf( "Server ready. Listening to port '%d'.\n\n", this->port );

    client_len = sizeof (sa_cli);
    for (;;){


        gnutls_init( &session, GNUTLS_SERVER );
        gnutls_priority_set( session, priority_cache );
        gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, this->x509Cred );


        gnutls_certificate_server_set_request(session, GNUTLS_CERT_IGNORE);



      sd = accept( listen_sd, (struct sockaddr *) & sa_cli, &client_len );

      printf ("- connection from %s, port %d\n",
              inet_ntop (AF_INET, &sa_cli.sin_addr, topbuf,
                         sizeof (topbuf)), ntohs (sa_cli.sin_port));

      gnutls_transport_set_ptr( session, (gnutls_transport_ptr_t)sd );

      do
        {
          ret = gnutls_handshake (session);
        }
      while (ret < 0 && gnutls_error_is_fatal (ret) == 0);

      if (ret < 0)
        {
          close (sd);
          gnutls_deinit (session);
          fprintf (stderr, "*** Handshake has failed (%s)\n\n",
                   gnutls_strerror (ret));
          continue;
        }
      printf ("- Handshake was completed\n");

      /* see the Getting peer's information example */
      /* print_info(session); */

      for (;;)
        {
          memset (buffer, 0, MAX_BUF + 1);
          ret = gnutls_record_recv (session, buffer, MAX_BUF);

          if (ret == 0)
            {
              printf ("\n- Peer has closed the GnuTLS connection\n");
              break;
            }
          else if (ret < 0)
            {
              fprintf (stderr, "\n*** Received corrupted "
                       "data(%d). Closing the connection.\n\n", ret);
              break;
            }
          else if (ret > 0)
            {
              /* echo data back to the client
               */
              gnutls_record_send (session, buffer, strlen (buffer));
            }
        }
      printf ("\n");
      /* do not wait for the peer to close the connection.
       */
      gnutls_bye (session, GNUTLS_SHUT_WR);

      close (sd);
      gnutls_deinit (session);

    }
    close (listen_sd);

    gnutls_certificate_free_credentials( this->x509Cred );
    gnutls_priority_deinit (priority_cache);

    gnutls_global_deinit ();

    return 0;
}

#define MSG "GET / HTTP/1.0\r\n\r\n"
extern int tcp_connect (void);
extern void tcp_close (int sd);
bool sslService::           client(){

// vars
    int ret, sd, ii;
    gnutls_session_t session;
    char buffer[MAX_BUF + 1];
    const char *err;
    struct sockaddr_in      saClient;


    gnutls_certificate_credentials_t xcred;

    gnutls_global_init();
    gnutls_global_set_log_level( 99 );

    /* X509 stuff */
    gnutls_certificate_allocate_credentials( &xcred );

    /* sets the trusted cas file
    */
    /* gnutls_certificate_set_x509_system_trust(xcred); */
    this->generateKeyPair( "client", wsslClientKeyPath );
    this->credCreate( &xcred, wsslClientKeyPath "client-cert.pem", wsslClientKeyPath "client-key.pem", _verify_certificate_callback );

/*
    gnutls_certificate_set_x509_trust_file( xcred, CAFILE, GNUTLS_X509_FMT_PEM);
    gnutls_certificate_set_verify_function( xcred, _verify_certificate_callback);

    ret = gnutls_certificate_set_x509_key_file( xcred, "/tmp/ssl/client-cert.pem", "/tmp/ssl/client-key.pem", GNUTLS_X509_FMT_PEM );
    if( ret != GNUTLS_E_SUCCESS ){
        gnutls_strerror( ret );
        return false;
    }
*/


    /* If client holds a certificate it can be set using the following:
    *
    gnutls_certificate_set_x509_key_file (xcred,
    "cert.pem", "key.pem",
    GNUTLS_X509_FMT_PEM);
    */

    /* Initialize TLS session
    */
    gnutls_init (&session, GNUTLS_CLIENT);

    gnutls_session_set_ptr (session, (void *) "localhost");
    gnutls_server_name_set (session, GNUTLS_NAME_DNS, "localhost",
    strlen("localhost"));

    /* Use default priorities */
    ret = gnutls_priority_set_direct (session, "NORMAL", &err);
    if (ret < 0)
    {
    if (ret == GNUTLS_E_INVALID_REQUEST)
    {
    fprintf (stderr, "Syntax error at: %s\n", err);
    }
    exit (1);
    }

    /* put the x509 credentials to the current session
    */
    gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, xcred);

    /* connect to the peer
    */
    memset (&saClient, '\0', sizeof (saClient));
    saClient.sin_family = AF_INET;
    saClient.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    saClient.sin_port = htons( this->port );      /* Server Port number */
    int sdSocket = socket( AF_INET, SOCK_STREAM, 0 );
    sd = connect( sdSocket, (const sockaddr*)&saClient, sizeof(saClient) );

    gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) sdSocket);

    /* Perform the TLS handshake
    */
    do
    {
    ret = gnutls_handshake (session);
    }
    while (ret < 0 && gnutls_error_is_fatal (ret) == 0);

    if (ret < 0)
    {
    fprintf (stderr, "*** Handshake failed\n");
    gnutls_perror (ret);
    goto end;
    }
    else
    {
    printf ("- Handshake was completed\n");
    }

    gnutls_record_send (session, MSG, strlen (MSG));

    ret = gnutls_record_recv (session, buffer, MAX_BUF);
    if (ret == 0)
    {
    printf ("- Peer has closed the TLS connection\n");
    goto end;
    }
    else if (ret < 0)
    {
    fprintf (stderr, "*** Error: %s\n", gnutls_strerror (ret));
    goto end;
    }

    printf ("- Received %d bytes: ", ret);
    for (ii = 0; ii < ret; ii++)
    {
    fputc (buffer[ii], stdout);
    }
    fputs ("\n", stdout);

    gnutls_bye (session, GNUTLS_SHUT_RDWR);

    end:

    close(sd);

    gnutls_deinit (session);

    gnutls_certificate_free_credentials (xcred);

    gnutls_global_deinit ();

    return 0;
}




#endif
