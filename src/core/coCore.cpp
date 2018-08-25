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


#ifndef doCore_C
#define doCore_C


#include "coCore.h"
#include <sodium.h>
#include <string>

struct option optionEndElement = { NULL, 0, 0, 0 };

coCore*         coCore::ptr = NULL;
bool            coCore::setupMode = false;
coMessage*      coCore::message = NULL;


coCore::                    coCore(){

// init lock
    this->threadLock = 0;


// get host name
    struct utsname tempHostInfo;
    uname( &tempHostInfo );
    etStringAlloc( this->hostName );
    etStringCharSet( this->hostName, tempHostInfo.nodename, -1 );
    this->hostNodeNameLen = strlen( tempHostInfo.nodename );

// config path
    etStringAlloc( this->configBasePath );
    etStringCharSet( this->configBasePath, "/etc/copilot", -1 );

// my node name
    etStringAlloc( this->thisNodeName );
    etStringCharSet( this->thisNodeName, tempHostInfo.nodename, -1 );




// create temp-message
    coCore::message = new coMessage();


// because ssh-configuration is based on our hostname, we print it out for debugging
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "My hostname: %s", tempHostInfo.nodename );
    etDebugMessage( etID_LEVEL_INFO, etDebugTempMessage );


// init libsodium
    if (sodium_init() == -1) {
        exit(-1);
    }


// save the instance
    this->ptr = this;


// we need some options for core
    coCore::addOption( "help", "h", "print this help", no_argument );
    coCore::addOption( "debug", "d", "Enable debugging", no_argument );

    coCore::addOption( "websocket", "w", "Enable debugging", no_argument );

}

coCore::                    ~coCore(){
	delete this->message;
    etStringFree( this->hostName );

}



bool coCore::               addOption( const char* optionLong, const char* optionShort, const char* description, int has_arg ){


// allocate a new option
    struct option*      newOption = NULL;
    struct option*      newOptions = NULL;
    int                 newOptionsLen = coCore::ptr->optionsLen + 1;
    int                 newOptionNameLen = strlen( optionLong );


// allocate new option array
    newOptions = (struct option*)malloc( (newOptionsLen + 1) * sizeof(struct option) );
    memset( newOptions, 0, (newOptionsLen + 1) * sizeof(struct option) );
    if( coCore::ptr->options != NULL ){
    // copy the old one
        memcpy( newOptions, coCore::ptr->options, coCore::ptr->optionsLen * sizeof(struct option) );

    // release the old one
        free( coCore::ptr->options );
    }


// the new element
    newOption = &newOptions[newOptionsLen-1];
// name
    newOption->name = (const char*)malloc( (newOptionNameLen+1) * sizeof(char) );
    memset( (void*)newOption->name, 0, (newOptionNameLen+1) * sizeof(char) );
    memcpy( (void*)newOption->name, optionLong, newOptionNameLen * sizeof(char) );
// has arguments
    newOption->has_arg = has_arg;
// option No
    newOption->val = newOptionsLen - 1;
// flag
    newOption->flag = NULL;



// the new element
    newOption = &newOptions[newOptionsLen];
// name
    newOption->name = NULL;
// has arguments
    newOption->has_arg = 0;
// option No
    newOption->val = 0;
// flag
    newOption->flag = NULL;



    coCore::ptr->options = newOptions;
    coCore::ptr->optionsLen = newOptionsLen;

/*
// new option
    newOption = (struct option*)malloc( sizeof(struct option) );
    memset( newOption, 0, sizeof(struct option) );

// name
    newOption->name = (const char*)malloc( (newOptionNameLen+1) * sizeof(char) );
    memset( (void*)newOption->name, 0, (newOptionNameLen+1) * sizeof(char) );
    memcpy( (void*)newOption->name, optionLong, newOptionNameLen * sizeof(char) );

// set the new one
    newOptions[newOptionsLen-1] = newOption;
    newOptions[newOptionsLen] = &optionEndElement;

*/



}


bool coCore::               isOption( const char* optionLong, unsigned int optNum ){

// check
    if( optNum == '?' ){
        return false;
    }

    if( optNum > coCore::ptr->optionsLen ){
        etDebugMessage( etID_LEVEL_ERR, "This is impossible" );
        return false;
    }

// vars
    struct option*      newOption = &coCore::ptr->options[optNum];


    if( strcmp(newOption->name,optionLong) == 0 ){
        return true;
    }

    return false;
}


void coCore::               dumpOptions(){

// var
    struct option*      optionActual = NULL;

    for( unsigned int index = 0; index < coCore::ptr->optionsLen; index++ ){

        optionActual = &coCore::ptr->options[index];

        fprintf( stdout, "--%s\n", optionActual->name );


    }

    fflush( stdout );

}


bool coCore::               parseOpt( int argc, char *argv[] ){
// reset getopt
    optind = 1;

    int optionSelected = 0;
    while( optionSelected >= 0 ) {
        optionSelected = getopt_long(argc, argv, "", coCore::ptr->options, NULL);
        if( optionSelected < 0 ) break;

        if( coCore::isOption( "help", optionSelected ) == true ){
            fprintf( stdout, "\n====== core ======\n" );
            fprintf( stdout, "--nodename <nodename>: Sets the name of this node\n" );
            break;
        }

        if( coCore::isOption( "debug", optionSelected ) == true ){
            etDebugLevelSet( etID_LEVEL_DETAIL_APP );
            continue;
        }

        if( coCore::isOption( "configpath", optionSelected ) == true ){
            this->configPath( optarg );
            continue;
        }

        if( coCore::isOption( "nodename", optionSelected ) == true ){
            this->nodeName( optarg );
            continue;
        }
    }

    return true;
}



const char* coCore::        configPath( const char* path ){

// set it
    if( path != NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Set path to: '%s'", path );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

        etStringCharSet( this->configBasePath, path, -1 );
        return path;
    } else {
        const char* myConfigPath = NULL;
        etStringCharGet( this->configBasePath, myConfigPath );
        return myConfigPath;
    }

    return NULL;
}


const char* coCore::        nodeName( const char* newNodeName ){

// vars
    const char*         myNodeNameCharArray = NULL;

// set
    if( newNodeName != NULL ){
        etStringCharSet( this->thisNodeName, newNodeName, -1 );
        return newNodeName;
    }

// get
    etStringCharGet( this->thisNodeName, myNodeNameCharArray );
    return myNodeNameCharArray;
}


void coCore::               setHostName( const char *hostname ){
	etStringCharSet( this->hostName, hostname, -1 );
	this->hostNodeNameLen = this->hostName->lengthActual;

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Set hostname to %s", hostname );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

}


bool coCore::               hostNameGet( const char** hostName, int* hostNameChars ){
	if( hostName != NULL ){
		__etStringCharGet( this->hostName, hostName );
	}
	if( hostNameChars != NULL ){
		*hostNameChars = this->hostNodeNameLen;
	}
	return true;
}


const char* coCore::        hostNameGet(){

// vars
	const char*		tempCharArray = NULL;
	if( etStringCharGet( this->hostName, tempCharArray ) != etID_YES ){
		return NULL;
	}

	return tempCharArray;
}


bool coCore::               isHostName( const char* hostNameToCheck ){

// vars
	const char*		tempCharArray = NULL;
	if( etStringCharGet( this->hostName, tempCharArray ) != etID_YES ){
		return NULL;
	}

	if( strncmp( hostNameToCheck, tempCharArray, strlen(tempCharArray) ) == 0 ){
		return true;
	}
	return false;
}


bool coCore::               isNodeName( const char* nodeNameToCheck ){
    if( nodeNameToCheck == NULL ) return false;
    if( coCore::ptr == NULL ) return false;

// vars
	const char*		tempCharArray = coCore::ptr->nodeName();

	return coCore::strIsExact( nodeNameToCheck, tempCharArray, strlen(tempCharArray) );
}




/*
bool coCore::               setTopic( coPluginElement* pluginElement, json_t* jsonAnswerObject, const char* msgGroup  ){
// check
    if( pluginElement == NULL ) return false;
    if( jsonAnswerObject == NULL ) return false;

// vars
    json_t*         jsonTopic;
    std::string     fullTopic;
    const char*     hostNameChar;
    const char*     hostGroupChar;

// get existing topic
    jsonTopic = json_object_get( jsonAnswerObject, "topic" );
    if( jsonTopic == NULL ) return false;


    fullTopic = "nodes/";

// hostname
    etStringCharGet( pluginElement->listenHostName, hostNameChar );
    if( strlen(hostNameChar) > 0 ){
        fullTopic += hostNameChar;
    } else {
		coCore::ptr->hostNameGet( &hostNameChar, NULL );
        fullTopic += hostNameChar;
    }

// group
    //etStringCharGet( pluginElement->listenGroup, hostGroupChar );
    //fullTopic += "/";
    //fullTopic += hostGroupChar;
	fullTopic += "/";
	fullTopic += msgGroup;

// topic
    fullTopic += "/";
    fullTopic += json_string_value(jsonTopic);

// write it back
    json_object_set_new( jsonAnswerObject, "topic", json_string(fullTopic.c_str()) );

    return true;
}
*/

bool coCore::               strIsExact( const char* str1, const char* str2, int str2Len ){
    if( str1 == NULL ) return false;
    if( str2 == NULL ) return false;

// vars
    int str1Len = strlen(str1);

    if( str1Len != str2Len ) return false;

    if( strncmp(str1,str2,str1Len) == 0 ){
        return true;
    }

    return false;
}

/**
@return
 - 1: Value was added to jsonObject
 - 2: key was found in jsonObject, and value was set from jsonValue
 - 3: key was not found in jsonObject, new jsonValue was created with defaultValue and value is set do defaultValue
*/
int coCore::                jsonValue( json_t* jsonObject, const char* key, char* value, int valueMax, const char* defaultValue, bool toJson ){

// vars
    json_t*     jsonValue = NULL;

// from value to json
    if( toJson == true ){
        json_object_set_new( jsonObject, key, json_string(value) );
        return 1;
    }

// from json to value
    else {

    // clean
        memset( value, 0, valueMax );

    // set
        jsonValue = json_object_get( jsonObject, key );
        if( jsonValue != NULL ) {
            strncat( value, json_string_value(jsonValue), valueMax );
            return 2;
        } else {
            strncat( value, defaultValue, valueMax );
            json_object_set_new( jsonObject, key, json_string(defaultValue) );
            return 3;
        }

    }

    return 0;
}


int coCore::                jsonValue( json_t* jsonObject, const char* key, std::string* value, const char* defaultValue, bool toJson ){

// vars
    json_t*     jsonValue = NULL;

// from value to json
    if( toJson == true ){
        json_object_set_new( jsonObject, key, json_string(value->c_str()) );
        return 1;
    }

// from json to value
    else {

    // clean
        value->clear();


    // set
        jsonValue = json_object_get( jsonObject, key );
        if( jsonValue != NULL ) {
            value->assign( json_string_value(jsonValue) );
            return 2;
        } else {
            value->assign( defaultValue );
            json_object_set_new( jsonObject, key, json_string(defaultValue) );
            return 3;
        }

    }

    return 0;
}


bool coCore::               passwordCheck( const char* user, const char* pass ){

// vars
    const char*     baseConfigPath = NULL;
    std::string     configFile;
    json_t*         jsonAuthObject = NULL;
    json_t*         jsonUserObject = NULL;
    json_error_t    jsonError;
    const char*     jsonPassword = NULL;
    int             jsonPasswordLen = 0;

// get config path
    baseConfigPath = coCore::ptr->configPath();
    configFile  = baseConfigPath;
    configFile += "/";
    configFile += "auth.json";

// open the file
    jsonAuthObject = json_load_file( configFile.c_str(), JSON_PRESERVE_ORDER, &jsonError );
    if( jsonAuthObject == NULL ){
        jsonAuthObject = json_object();
        json_object_set_new( jsonAuthObject, "username", json_object() );
        json_dump_file( jsonAuthObject, configFile.c_str(), JSON_PRESERVE_ORDER | JSON_INDENT(4) );
    }

// try to get the user
    jsonUserObject = json_object_get( jsonAuthObject, user );
    if( jsonUserObject == NULL ) return false;

// try to get the password of the user
    jsonPasswordLen = 0;
    json_t* jsonPasswordObject = json_object_get( jsonUserObject, "salsa208sha256" );
    if( jsonPasswordObject != NULL ) {
        jsonPassword = json_string_value( jsonPasswordObject );
        jsonPasswordLen = strlen( jsonPassword );
    }

// no password provided yet, set it
    if( jsonPasswordLen == 0 ){

    // create the hashed password
        char hashed_password[crypto_pwhash_scryptsalsa208sha256_STRBYTES];
        if( crypto_pwhash_scryptsalsa208sha256_str( hashed_password, pass, strlen(pass),
                crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_SENSITIVE,
                crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_SENSITIVE) == 0) {
            json_object_set_new( jsonUserObject, "salsa208sha256", json_string(hashed_password) );
        }

    // save the auth-file
        json_dump_file( jsonAuthObject, configFile.c_str(), JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // recalc password-stuff
        jsonPassword = hashed_password;
        jsonPasswordLen = strlen( jsonPassword );
    }


// verify password
    if( crypto_pwhash_scryptsalsa208sha256_str_verify(jsonPassword, pass, strlen(pass)) == 0) {
        return true;
    }


    return false;
}


bool coCore::               passwordChange( const char* user, const char* oldpw, const char* newpw ){
    return false;
}


void coCore::               mainLoop(){
    while(1){
        sleep(5);
    }
}



#endif

















