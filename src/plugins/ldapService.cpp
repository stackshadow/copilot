/*  Copyright (C) 2017 by Martin Langlotz alias stackshadow

    This file is part of doDB.

    doDB is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    doDB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with doDB.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ldapService_C
#define ldapService_C

#include "coCore.h"
#include "plugins/ldapService.h"



ldapService::                       ldapService() : coPlugin( "ldap", coCore::ptr->nodeName(), "ldap" ) {

// setup timeout
    this->searchTimeout.tv_sec = 20;
    this->searchTimeout.tv_usec = 0;


    this->configLoad();


// init values
    this->ldapConnectionAdmin = NULL;
    this->ldapConnectionAdminActive = false;
    this->ldapConnection = NULL;
    this->ldapConnectionActive = false;

/*
// clean
    this->dumpAll( this->ldapConnectionAdmin, "cn=config" );
    if( this->ldapConnectionActive == true ) this->dumpAll( this->ldapConnection, "dc=evilbrain,dc=de" );



// demo
    this->dbRemove( "dc=evilbrain,dc=de" );


    this->dbAdd( "dc=evilbrain,dc=de", "mdb" );
    this->attributeAdd( "olcDatabase={1}mdb,cn=config", "olcAccess", "to * by dn=\"cn=admin,cn=config\" write" );




    //this->exist( "dc=evilbrain,dc=de" );

// create base groups
    std::string tempString = "";


    this->orgaAdd( "evilbrain", "de" );
    this->dumpAll( this->ldapConnection, "" );

    tempString = this->userdn.substr(3);
    this->orgaUnitAdd( tempString.c_str() );


    tempString = this->groupdn.substr(3);
    this->orgaUnitAdd( tempString.c_str() );


    this->userAdd( "dummyMember" );
    this->groupAdd( "wikiAdmin", "The admin of the wiki" );

*/
// register plugin
	coCore::ptr->plugins->append( this );
}


ldapService::                       ~ldapService(){

}



coPlugin::t_state ldapService::		onBroadcastMessage( coMessage* message ){

// vars
	const char*			msgTarget = message->nodeNameTarget();
    const char*			msgSource = message->nodeNameSource();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
    int                 msgCommandLen = 0;
	const char*			msgPayload = message->payload();
    char*               jsonTempString = NULL;

// check
    if( msgCommand == NULL ) return coPlugin::NO_REPLY;
    msgCommandLen = strlen(msgCommand);

    std::string     tempString;


    if( coCore::strIsExact("configGet",msgCommand,msgCommandLen) == true ){

    // get passwords ( we dont want to send it over websocket...
        json_t* adminpass = json_object_get( this->jsonObjectConfig, "adminpass" );
        json_t* loginpass = json_object_get( this->jsonObjectConfig, "loginpass" );

    // remove it from object
        json_incref(adminpass);
        json_object_del( this->jsonObjectConfig, "adminpass" );
        json_incref(loginpass);
        json_object_del( this->jsonObjectConfig, "loginpass" );

    // dump
        jsonTempString = json_dumps( this->jsonObjectConfig, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        msgTarget, msgSource, "ldap", "config", jsonTempString );

    // cleanup
        free( jsonTempString );

    // save it back to json
        json_object_set_new( this->jsonObjectConfig, "adminpass", adminpass );
        json_object_set_new( this->jsonObjectConfig, "loginpass", loginpass );

        return coPlugin::MESSAGE_FINISHED;
    }


    if( coCore::strIsExact("configSet",msgCommand,msgCommandLen) == true ){

    // vars
        json_error_t    jsonError;
        json_t*         jsonNewValues = NULL;
        void*           jsonIterator = NULL;
        const char*     jsonKey = NULL;
        json_t*         jsonValue = NULL;

	// parse json
		jsonNewValues = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
		if( jsonNewValues == NULL || jsonError.line > -1 ){
			return coPlugin::NO_REPLY;
		}

    // iterate values and update internal config
        jsonIterator = json_object_iter( jsonNewValues );
        while( jsonIterator != NULL ){
            jsonKey = json_object_iter_key(jsonIterator);
            jsonValue = json_object_iter_value(jsonIterator);

            json_object_set_new( this->jsonObjectConfig, jsonKey, json_string( json_string_value(jsonValue)) );

            jsonIterator = json_object_iter_next( jsonNewValues, jsonIterator );
        }

    // save
        this->configSave();

    // destroy temporary object
        json_decref( jsonNewValues );

    // send message
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->nodeName(), "", "ldap", "saved", "" );

        return coPlugin::MESSAGE_FINISHED;
    }


    if( coCore::strIsExact("connect",msgCommand,msgCommandLen) == true ){
    // connect
        int ldapVersion = LDAP_VERSION3;
        int ldapTimeout = 30;


    // admin-connection
        this->ldapConnectionAdminActive = ldapService::connect( &this->ldapConnectionAdmin,
            &ldapVersion, &ldapTimeout, this->uri.c_str(), this->admindn.c_str(), this->adminpass.c_str() );

    // dbconnection
        this->ldapConnectionActive = ldapService::connect( &this->ldapConnection,
            &ldapVersion, &ldapTimeout, this->uri.c_str(), this->logindn.c_str(), this->loginpass.c_str() );


    // create db
        if( this->dbAdd( this->basedn.c_str(), "mdb" ) == true ){

        // set username and password
            this->dbChangeCreds( this->basedn.c_str(), this->logindn.c_str(), this->loginpass.c_str() );
        //this->attributeAdd( "olcDatabase={1}mdb,cn=config", "olcAccess", "to * by dn=\"cn=admin,cn=config\" write" );
        }

    // change
        this->dbChangeCreds( this->basedn.c_str(), this->logindn.c_str(), this->loginpass.c_str() );

    // add orga if needed
        this->orgaAdd( this->basedn.c_str() );
    // create user-tree
        tempString = this->userdn.substr(3);
        this->orgaUnitAdd( tempString.c_str() );
    // create group-tree
        tempString = this->groupdn.substr(3);
        this->orgaUnitAdd( tempString.c_str() );

    // create dummyMember for new groups
        this->userAdd( "dummyMember" );

        goto connstate;
        return coPlugin::MESSAGE_FINISHED;
    }


    if( coCore::strIsExact("disconnect",msgCommand,msgCommandLen) == true ){

        ldap_unbind_ext_s( this->ldapConnectionAdmin, NULL, NULL );
        this->ldapConnectionAdminActive = false;
        this->ldapConnectionAdmin = NULL;

        ldap_unbind_ext_s( this->ldapConnection, NULL, NULL );
        this->ldapConnectionActive = false;
        this->ldapConnection = NULL;

        goto connstate;
        return coPlugin::MESSAGE_FINISHED;
    }


    if( coCore::strIsExact("status",msgCommand,msgCommandLen) == true ){
connstate:
    // connected ?
        if( this->ldapConnectionAdminActive && this->ldapConnectionActive ){
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->nodeName(), "", "ldap", "connected", "connected" );
        } else {
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->nodeName(), "", "ldap", "disconnected", "disconnected" );
        }

        return coPlugin::MESSAGE_FINISHED;
    }


    if( coCore::strIsExact("userlist",msgCommand,msgCommandLen) == true ){

    // connected ?
        if( this->ldapConnection == NULL || this->ldapConnectionActive == false ){
            return coPlugin::MESSAGE_UNKNOWN;
        }

    // vars
        json_t*     jsonUserlist = json_object();

    // add users to json
        this->dumpUsers( this->ldapConnection, jsonUserlist );

    // dump
        jsonTempString = json_dumps( jsonUserlist, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        msgTarget, msgSource, "ldap", "users", jsonTempString );

    // cleanup
        free( jsonTempString );
        json_decref(jsonUserlist);

        return coPlugin::MESSAGE_FINISHED;
    }


    if( coCore::strIsExact("grouplist",msgCommand,msgCommandLen) == true ){

    // connected ?
        if( this->ldapConnection == NULL || this->ldapConnectionActive == false ){
            return coPlugin::MESSAGE_UNKNOWN;
        }

    // vars
        json_t*     jsonGrouplist = json_object();

    // add users to json
//        this->dumpGroups( this->ldapConnection, jsonGrouplist );

    // dump
        jsonTempString = json_dumps( jsonGrouplist, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        msgTarget, msgSource, "ldap", "groups", jsonTempString );

    // cleanup
        free( jsonTempString );
        json_decref( jsonGrouplist );

        return coPlugin::MESSAGE_FINISHED;
    }


    if( coCore::strIsExact("useradd",msgCommand,msgCommandLen) == true ){
    // vars
        json_error_t    jsonError;
        json_t*         jsonNewValues = NULL;
        void*           jsonIterator = NULL;
        const char*     jsonKey = NULL;
        json_t*         jsonValue = NULL;

	// parse json
		jsonNewValues = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
		if( jsonNewValues == NULL || jsonError.line > -1 ){
			return coPlugin::NO_REPLY;
		}

    // username
        jsonValue = json_object_get( jsonNewValues, "name" );
        if( jsonValue == NULL ){
            json_decref( jsonNewValues );
            return coPlugin::NO_REPLY;
        }
        const char* userName = json_string_value( jsonValue );

    // password
        jsonValue = json_object_get( jsonNewValues, "pw" );
        if( jsonValue == NULL ){
            json_decref( jsonNewValues );
            return coPlugin::NO_REPLY;
        }
        const char* userPassword = json_string_value( jsonValue );

    // add
        if( this->userAdd( userName ) == true ){
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->nodeName(), "", "ldap", "userAdded", "" );
        } else {
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->nodeName(), "", "ldap", "userNotAdded", "" );
        }
    }



    return coPlugin::MESSAGE_UNKNOWN;
}


bool ldapService::                  onSetup(){

}


bool ldapService::                  onExecute(){

}



bool ldapService::                  configLoad(){

// vars
    bool needToSave = false;

// get config path
    this->jsonObjectConfig = coCore::ptr->config->section( "ldap" );

// load config from json
    this->configFromJson( this->jsonObjectConfig );


}


bool ldapService::                  configToJson( json_t* jsonObject ){

    coCore::jsonValue( jsonObject, "uri", &this->uri, "ldap://localhost/", true );

    coCore::jsonValue( jsonObject, "logindn", &this->logindn, "cn=Manager,dc=evilbrain,dc=de", true );
    coCore::jsonValue( jsonObject, "loginpass", &this->loginpass, "secret", true );

    coCore::jsonValue( jsonObject, "basedn", &this->basedn, "dc=evilbrain,dc=de", true );
    coCore::jsonValue( jsonObject, "groupdn", &this->groupdn, "ou=groups", true );
    coCore::jsonValue( jsonObject, "userdn", &this->userdn, "ou=accounts", true );


    return true;
}


bool ldapService::                  configFromJson( json_t* jsonObject ){

// vars
    int resultValue = 0;

    resultValue |= coCore::jsonValue( jsonObject, "uri", &this->uri, "ldap://localhost/", false );

    resultValue |= coCore::jsonValue( jsonObject, "admindn", &this->admindn, "cn=admin,cn=config", false );
    resultValue |= coCore::jsonValue( jsonObject, "adminpass", &this->adminpass, "secret", false );

    resultValue |= coCore::jsonValue( jsonObject, "logindn", &this->logindn, "cn=admin,dc=beispiel,dc=de", false );
    resultValue |= coCore::jsonValue( jsonObject, "loginpass", &this->loginpass, "secret", false );

    resultValue |= coCore::jsonValue( jsonObject, "basedn", &this->basedn, "dc=beispiel,dc=de", false );
    resultValue |= coCore::jsonValue( jsonObject, "groupdn", &this->groupdn, "ou=groups", false );
    resultValue |= coCore::jsonValue( jsonObject, "userdn", &this->userdn, "ou=accounts", false );

    if( resultValue == 3 ){
        this->configSave();
    }
}


bool ldapService::                  configSave(){
// save json to internal values
    this->configFromJson( this->jsonObjectConfig );

// return
    return coCore::ptr->config->save();
}


bool ldapService::                  connect( LDAP** connection, int* ldapVersion, int* ldapTimeout, const char* uri, const char* logindn, const char* pass ){
// vars
    LDAP*   ldapConnection = *connection;
    int     returnCode = -1;

// disconnect, if connected
    if( ldapConnection != NULL ){
        ldap_unbind_ext_s( ldapConnection, NULL, NULL );
        ldapConnection = NULL;
    }

// init
    returnCode = ldap_initialize( &ldapConnection, uri );
    if( returnCode != LDAP_SUCCESS ){
        return false;
    }

// set protocoll version
    returnCode = ldap_set_option( ldapConnection, LDAP_OPT_PROTOCOL_VERSION, (void*)ldapVersion );
    returnCode = ldap_set_option( ldapConnection, LDAP_OPT_NETWORK_TIMEOUT, (void*)ldapTimeout );


// login
    struct berval credentials;
    struct berval* servercredp;
    credentials.bv_val = (char*)pass;
    credentials.bv_len = 6;


// connect
    returnCode = ldap_sasl_bind_s(
        ldapConnection,
        logindn,
        NULL,
        &credentials,
        NULL,
        NULL,
        &servercredp );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );

    // send message
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->nodeName(), "", "ldap", "msgError", errorMessage );

        *connection = NULL;
        return false;
    }

// debug message
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Connected" );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );


    *connection = ldapConnection;
    return true;
}



bool ldapService::                  ldapModAppend( LDAPMod*** mod, int* LDAPModLen, int op, const char* property, const char* value1, const char* value2 ){

// vars
    int             newModLen = *LDAPModLen + 1;
    size_t          newModSize = sizeof(LDAPMod*) * newModLen;
    LDAPMod**       newMod = NULL;
    const char**    newModAttrValues = NULL;
    size_t          newModAttrValuesSize = sizeof(char*) * 3;

// initial mod
    if( *LDAPModLen == 0 ){
        newModLen = 2;
    }

// allocate
    newMod = (LDAPMod**)malloc( newModSize );
    newModAttrValues = (const char**)malloc( newModAttrValuesSize );

// copy old pointer addr
    if( *mod != NULL ){
        memcpy( newMod, *mod, *LDAPModLen * sizeof(LDAPMod*) );
    }

// values
    newModAttrValues[0] = value1;
    newModAttrValues[1] = value2;
    newModAttrValues[2] = NULL;

// mod
    newMod[newModLen-2] = (LDAPMod*)malloc(sizeof(LDAPMod));
    newMod[newModLen-2]->mod_op = op;
    newMod[newModLen-2]->mod_type = (char*)property;
    newMod[newModLen-2]->mod_vals.modv_strvals = (char**)newModAttrValues;
    newMod[newModLen-1] = NULL;


    *mod = newMod;
    *LDAPModLen = newModLen;


}


bool ldapService::                  ldapModMemFree( LDAPMod*** mod ){
    if( mod == NULL ) return false;


    LDAPMod**       mods = *mod;
    LDAPMod*        actualMod;
    int             actualModIndex = 0;

    actualMod = mods[actualModIndex];
    while( actualMod != NULL ){

    // free values
        free(actualMod->mod_vals.modv_strvals);
        actualMod->mod_vals.modv_strvals = NULL;

    // free mod
        free(actualMod);
        mods[actualModIndex] = NULL;

        actualModIndex++;
        actualMod = mods[actualModIndex];
    }

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Freed %i mods", actualModIndex );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// free the mod itselfe
    free(*mod);
    *mod = NULL;

}




bool ldapService::                  exist( LDAP* connection, const char* dn ){
    if( connection == NULL ) return false;

// vars
    std::string     tmpFullDn = dn;
    LDAPMessage*    returnMessage;
    int             returnCode;
    LDAPMessage*    actualMessage = NULL;

/*
ldap_search_ext_s LDAP_P((
	LDAP			*ld,
	LDAP_CONST char	*base,
	int				scope,
	LDAP_CONST char	*filter,
	char			**attrs,
	int				attrsonly,
	LDAPControl		**serverctrls,
	LDAPControl		**clientctrls,
	struct timeval	*timeout,
	int				sizelimit,
	LDAPMessage		**res ));
*/
    returnCode = ldap_search_ext_s(
        connection,
        dn,
        LDAP_SCOPE_BASE,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        &this->searchTimeout,
        10,
        &returnMessage );

// everything correct ?
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        return false;
    }

// check if something was returned
    actualMessage = ldap_first_entry( connection, returnMessage );
    while( actualMessage != NULL ){
        return true;
    }

// nothing found
    return false;
}

/**
 * @brief
 * @param connection
 * @param searchBase
 * @param attribute
 * @param value
 * @param dn Return the founded dn or NULL if it was not found. dn must be freed with ldap_memfree() ! ( if not NULL )
 * @return
 */
bool ldapService::                  find( LDAP* connection, const char* searchBase, const char* attribute, const char* value, std::string* dn ){
// not connected
    if( connection == NULL ) return false;

// vars
    int                 returnCode;
    LDAPMessage*        actualMessage = NULL;
    LDAPMessage*        actualMessages = NULL;
    char*               entryDN = NULL;
    std::string         filter;

    filter  = "(&(";
    filter += attribute;
    filter += "=";
    filter += value;
    filter += "))";

// iterate
    returnCode = ldap_search_ext_s( connection, searchBase, LDAP_SCOPE_SUBTREE, filter.c_str(),
        NULL, 0,
        NULL, NULL, &this->searchTimeout, 10,
        &actualMessages
    );
    if( returnCode != LDAP_SUCCESS || actualMessages == NULL ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        goto onerror;
    }

// element was found ?
    actualMessage = ldap_first_entry( connection, actualMessages );
    if( actualMessage != NULL ){
        entryDN = ldap_get_dn( connection, actualMessage );
        if( dn != NULL ){
            dn->assign( entryDN );
        }

        ldap_memfree(entryDN);
        return true;
    }

onerror:
    if( dn != NULL ){
        dn->assign( "" );
    }
    return false;
}


bool ldapService::                  iterate( const char* searchBase ){
// not connected
    if( this->ldapConnectionActive == false ) return false;


// search
    ldap_search_ext_s(
        this->ldapConnection,
        searchBase,
        LDAP_SCOPE_SUBTREE,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        &this->searchTimeout,
        10,
        &this->resultMessages
    );

}


bool ldapService::                  dumpElement( LDAP* connection, LDAPMessage* actualMessage, json_t* jsonObjectOutput ){
// not connected
    if( connection == NULL ) return false;

// vars
    const char*     entryDN = NULL;
    BerElement*     attributes = NULL;
    char*           attributeName = NULL;
    struct berval** attributeValues = NULL;
    struct berval*  attributeValue = NULL;
    int             attributeValueCount = 0;

// get the entry DN
    entryDN = ldap_get_dn( connection, actualMessage );
    fprintf( stdout, "-------------- entry ------------\ndn: %s\n", entryDN );


// iterate attributes
    attributeName = ldap_first_attribute( connection, actualMessage, &attributes );
    while( attributeName != NULL ){
        attributeValues = ldap_get_values_len( connection, actualMessage, attributeName );
        attributeValueCount = ldap_count_values_len( attributeValues );

        for( ; attributeValueCount > 0; attributeValueCount-- ){
            attributeValue = attributeValues[attributeValueCount-1];
            fprintf( stdout, "attribute: %s=%s\n", attributeName, attributeValue->bv_val );

            if( jsonObjectOutput != NULL ){
                json_object_set_new( jsonObjectOutput, attributeName, json_string(attributeValue->bv_val) );
            }

        }

        attributeName = ldap_next_attribute( connection, actualMessage, attributes );
    }

    fflush( stdout );
    return true;
}


bool ldapService::                  dumpAll( LDAP* connection, const char* basedn ){
// not connected
    if( connection == NULL ) return false;

// vars
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;

// iterate
    ldap_search_ext_s(
        connection,
        basedn,
        LDAP_SCOPE_SUBTREE,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        &this->searchTimeout,
        10,
        &this->resultMessages
    );

// iterate over messages
    actualMessage = ldap_first_entry( connection, this->resultMessages );
    while( actualMessage != NULL ){

        entryDN = ldap_get_dn( connection, actualMessage );

    // dump the entry
        this->dumpElement( connection, actualMessage );
        ldap_memfree( entryDN );

        actualMessage = ldap_next_entry( connection, actualMessage );
    }
}


bool ldapService::                  dumpDBs( LDAP* connection, json_t* jsonObjectOutput ){
    if( connection == NULL ) return false;

// vars
    LDAPMessage*        actualMessages = NULL;
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;

// iterate
    ldap_search_ext_s( connection, "cn=config", LDAP_SCOPE_ONE,
        "(objectClass=olcDatabaseConfig)", NULL, 0, NULL, NULL,
        &this->searchTimeout, 1024, &actualMessages
    );

// iterate over messages
    actualMessage = ldap_first_entry( connection, actualMessages );
    while( actualMessage != NULL ){

        entryDN = ldap_get_dn( connection, actualMessage );

        json_object_set_new( jsonObjectOutput, entryDN, json_string("") );
        ldap_memfree( entryDN );

        actualMessage = ldap_next_entry( connection, actualMessage );
    }

    return true;
}


bool ldapService::                  dumpUsers( LDAP* connection, json_t* jsonObjectOutput ){
    if( connection == NULL ) return false;

// vars
    LDAPMessage*        actualMessages = NULL;
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;
    struct berval*      attributeValue = NULL;
    struct berval**     attributeValues = NULL;
    int                 attributeValuesCount = 0;
    std::string         fullDN;

// build dn
    fullDN  = this->userdn;
    fullDN += ",";
    fullDN += this->basedn;

// iterate
    ldap_search_ext_s( connection, fullDN.c_str(), LDAP_SCOPE_ONE,
        NULL, NULL, 0, NULL, NULL,
        &this->searchTimeout, 1024, &actualMessages
    );

// iterate over messages
    actualMessage = ldap_first_entry( connection, actualMessages );
    while( actualMessage != NULL ){

        entryDN = ldap_get_dn( connection, actualMessage );

        json_t* jsonDN = json_object();

        json_object_set_new( jsonDN, "dn", json_string(entryDN) );

    // sn
        attributeValues = ldap_get_values_len( connection, actualMessage, "sn" );
        attributeValuesCount = ldap_count_values_len( attributeValues );
        if( attributeValuesCount > 0 ){
            attributeValue = attributeValues[0];
            json_object_set_new( jsonDN, "sn", json_string(attributeValue->bv_val) );
        }
        ldap_value_free_len( attributeValues );

    // cn
        attributeValues = ldap_get_values_len( connection, actualMessage, "cn" );
        attributeValuesCount = ldap_count_values_len( attributeValues );
        if( attributeValuesCount > 0 ){
            attributeValue = attributeValues[0];
            json_object_set_new( jsonDN, "cn", json_string(attributeValue->bv_val) );
        }
        ldap_value_free_len( attributeValues );

    // uid
        attributeValues = ldap_get_values_len( connection, actualMessage, "uid" );
        attributeValuesCount = ldap_count_values_len( attributeValues );
        if( attributeValuesCount > 0 ){
            attributeValue = attributeValues[0];
            json_object_set_new( jsonDN, "uid", json_string(attributeValue->bv_val) );
        }
        ldap_value_free_len( attributeValues );

    // mail
        attributeValues = ldap_get_values_len( connection, actualMessage, "mail" );
        attributeValuesCount = ldap_count_values_len( attributeValues );
        if( attributeValuesCount > 0 ){
            attributeValue = attributeValues[0];
            json_object_set_new( jsonDN, "mail", json_string(attributeValue->bv_val) );
        }
        ldap_value_free_len( attributeValues );

    // set the new object inside the output object
        json_object_set_new(  jsonObjectOutput, entryDN, jsonDN );

        ldap_memfree( entryDN );
        actualMessage = ldap_next_entry( connection, actualMessage );
    }

    return true;
}




// admin server tasks

bool ldapService::                  dbAdd( const char* suffix, const char* dbType ){
    if( this->ldapConnectionAdminActive == false || this->ldapConnectionAdmin == NULL ) return false;

//
    LDAPMod**       mods;
    int             modsLen = 0;
    int             returnCode;
    std::string     fullDN;
    std::string     dbDirectory;
    std::string     tempCommand;

// check if db already exist
    if( this->find( this->ldapConnectionAdmin, "cn=config", "olcSuffix", suffix, NULL ) == true ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "DB '%s' already exist", suffix );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
        return false;
    }




// create directory
    dbDirectory  = "/var/lib/openldap/";
    dbDirectory += suffix;

	if( access( dbDirectory.c_str(), F_OK ) != 0 ){
        std::string createDirCmd = "mkdir -p ";
        createDirCmd += dbDirectory;
		system( createDirCmd.c_str() );
	}



// attribute - objectClass
    if( coCore::strIsExact( dbType, "bdb", 3 ) == true ){
        this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "objectClass", "olcDatabaseConfig", "olcBdbConfig" );
    }
    if( coCore::strIsExact( dbType, "mdb", 3 ) == true ){
        this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "objectClass", "olcDatabaseConfig", "olcMdbConfig" );
    }

// attribute - olcDatabase
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "olcDatabase", dbType );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "olcDbDirectory", dbDirectory.c_str() );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "olcSuffix", suffix );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "olcRootDN", this->logindn.c_str() );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "olcRootPW", this->loginpass.c_str() );


    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "olcDbIndex", "objectClass eq" );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "olcLastMod", "TRUE" );

// build dn
    fullDN  = "olcDatabase=";
    fullDN += dbType;
    fullDN += ",cn=config";


// call
    returnCode = ldap_add_ext_s( this->ldapConnectionAdmin, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }

// return
    this->ldapModMemFree( &mods );
    return true;
}


bool ldapService::                  dbChangeCreds( const char* suffix, const char* username, const char* password ){

/*
dn: olcDatabase={1}bdb,cn=config
changetype: modify
replace: olcRootDN
olcRootDN: cn=admin,dc=evilbrain,dc=de
-
replace: olcRootPW
olcRootPW: secret
*/

//
    LDAPMod*        mods[3];
    int             returnCode;
    std::string     fullDN;
    std::string     fullUserName;


// get the db of db-suffix
    if( this->find( this->ldapConnectionAdmin, "cn=config", "olcSuffix", suffix, &fullDN ) != true ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "DB '%s' dont exist", suffix );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }
    //ldap_memfree( entryDN );

// build username
    fullUserName  = username;


// attribute - olcRootDN
    LDAPMod modattr_olcRootDN;
    char*   modattr_olcRootDN_values[] = { (char*)fullUserName.c_str(), NULL };

    modattr_olcRootDN.mod_op = LDAP_MOD_REPLACE;
    modattr_olcRootDN.mod_type = (char*)"olcRootDN";
    modattr_olcRootDN.mod_vals.modv_strvals = modattr_olcRootDN_values;

// attribute - olcRootPW
    LDAPMod modattr_olcRootPW;
    char*   modattr_olcRootPW_values[] = { (char*)password, NULL };

    modattr_olcRootPW.mod_op = LDAP_MOD_REPLACE;
    modattr_olcRootPW.mod_type = (char*)"olcRootPW";
    modattr_olcRootPW.mod_vals.modv_strvals = modattr_olcRootPW_values;



// build mods
    mods[0] = &modattr_olcRootDN;
    mods[1] = &modattr_olcRootPW;
    mods[2] = NULL;


// call
    returnCode = ldap_modify_ext_s( this->ldapConnectionAdmin, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        return false;
    }

    return true;
}


bool ldapService::                  dbRemove( const char* suffix ){
    if( this->ldapConnectionAdminActive == false || this->ldapConnectionAdmin == NULL ) return false;

// vars
    std::string         fullDN;
    int                 messageCounter = 0;
    int                 returnCode = 0;
    LDAPMessage*        actualMessages = NULL;
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;


// purge all
    this->purge();


// get the db of db-suffix
    if( this->find( this->ldapConnectionAdmin, "cn=config", "olcSuffix", suffix, &fullDN ) != true ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "DB '%s' dont exist", suffix );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

// iterate over messages
    returnCode = ldap_delete_ext_s( this->ldapConnectionAdmin, fullDN.c_str(), NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error on delete dn '%s': %s", suffix, errorMessage );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

        return false;
    }

    return true;
}




// tasks on an db

bool ldapService::                  purge(){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;

// vars
    LDAPMessage*        actualMessages = NULL;
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;
    int                 returnCode = 0;
    int                 messageCounter = 0;

// iterate
    ldap_search_ext_s( this->ldapConnection, this->basedn.c_str(), LDAP_SCOPE_CHILDREN,
        NULL, NULL, 0, NULL, NULL,
        &this->searchTimeout, 1024, &actualMessages
    );

    messageCounter = ldap_count_messages( this->ldapConnection, actualMessages );

    while( messageCounter > 1 ){

        // iterate over messages
        actualMessage = ldap_first_entry( this->ldapConnection, actualMessages );
        while( actualMessage != NULL ){

            entryDN = ldap_get_dn( this->ldapConnection, actualMessage );

            returnCode = ldap_delete_ext_s( this->ldapConnection, entryDN, NULL, NULL );

            ldap_memfree( entryDN );

            actualMessage = ldap_next_entry( this->ldapConnection, actualMessage );
        }


        ldap_search_ext_s( this->ldapConnection, this->basedn.c_str(), LDAP_SCOPE_CHILDREN,
            NULL, NULL, 0, NULL, NULL,
            &this->searchTimeout, 1024, &actualMessages
        );

        messageCounter = ldap_count_messages( this->ldapConnection, actualMessages );

    }

}


bool ldapService::                  attributeAdd( const char* dn, const char* property, const char* value ){

// vars
    LDAPMod**       mods = NULL;
    int             modsLen = 0;
    int             returnCode;

    ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, property, value );
    //ldapModAppend( property, value, &mods, &modsLen );

// call
    returnCode = ldap_modify_ext_s( this->ldapConnectionAdmin, dn, mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        return false;
    }
}


bool ldapService::                  orgaAdd( const char* basedn ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;

// check if orga already exist
    if( this->exist( this->ldapConnection, basedn ) == true ){
        return true;
    }


/*
    #Organization for Example Corporation
    dn: dc=example,dc=com
    objectClass: dcObject
    objectClass: organization
    dc: example
    o: Example Corporation
    description: The Example Corporation
*/

//
    LDAPMod**       mods = NULL;
    int             modsLen = 0;
    int             returnCode;
    std::string     fullDN;


// attribute - objectClass
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "objectClass", "dcObject", "organization" );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "o", "orga" );


// call
    returnCode = ldap_add_ext_s( this->ldapConnection, basedn, mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }


    this->ldapModMemFree( &mods );
    return true;
}


bool ldapService::                  orgaUnitAdd( const char *orgaName, const char* base ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;


// create account-group
    LDAPMod**       mods;
    int             modsLen = 0;
    int             returnCode;
    std::string     fullDN;

// build dn
    fullDN  = "ou=";
    fullDN += orgaName;
    fullDN += ",";
    if( base != NULL ){
        fullDN += base;
    } else {
        fullDN += this->basedn;
    }


// check if orga already exist
    if( this->exist( this->ldapConnection, fullDN.c_str() ) == true ){
        return true;
    }

// attribute - objectClass
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "objectClass", "organizationalUnit" );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "ou", orgaName );


// call
    returnCode = ldap_add_ext_s( this->ldapConnection, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        this->ldapModMemFree( &mods );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        return false;
    }

    this->ldapModMemFree( &mods );
    return true;
}


bool ldapService::                  groupAdd( const char *name, const char* description ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;



// create account-group
    LDAPMod**       mods = NULL;
    int             modsLen = 0;
    int             returnCode;
    std::string     fullDN;
    std::string     dummyMemberDN;


// build dn
    dummyMemberDN  = "uid=dummyMember,";
    dummyMemberDN += this->userdn;
    dummyMemberDN += ",";
    dummyMemberDN += this->basedn;

    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "objectClass", "groupOfNames" );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "cn", name );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "description", description );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "member", dummyMemberDN.c_str() );


// build dn
    fullDN  = "cn=";
    fullDN += name;
    fullDN += ",";
    fullDN += this->groupdn;
    fullDN += ",";
    fullDN += this->basedn;

// call
    returnCode = ldap_add_ext_s( this->ldapConnection, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }

    this->ldapModMemFree( &mods );
    return true;
}


bool ldapService::                  userAdd( const char *accountName ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;



// create account-group
    LDAPMod**       mods;
    int             modsLen = 0;
    int             returnCode;
    std::string     fullDN;

// build dn
    fullDN  = "uid=";
    fullDN += accountName;
    fullDN += ",";
    fullDN += this->userdn;
    fullDN += ",";
    fullDN += this->basedn;

// check if orga already exist
    if( this->exist( this->ldapConnection, fullDN.c_str() ) == true ){
        return true;
    }


    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "objectClass", "person", "inetOrgPerson" );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "sn", "unknown" );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "cn", "unknown" );
    //this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "userid", accountName );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "uid", accountName );
    //this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "shadowInactive", "0" );


// call
    returnCode = ldap_add_ext_s( this->ldapConnection, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }

    this->ldapModMemFree( &mods );
    return true;
}



#endif // ldapService_C
