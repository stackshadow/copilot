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



ldapService::                   ldapService() : coPlugin( "ldap", "", "ldap" ) {

// setup timeout
    this->searchTimeout.tv_sec = 20;
    this->searchTimeout.tv_usec = 0;


    this->configLoad();

// connect
    int ldapVersion = LDAP_VERSION3;
    int ldapTimeout = 30;

// admin-connection
    this->ldapConnectionAdminActive = ldapService::connect( &this->ldapConnectionAdmin,
        &ldapVersion, &ldapTimeout, this->uri.c_str(), this->admindn.c_str(), this->adminpass.c_str() );

// dbconnection
    this->ldapConnected = ldapService::connect( &this->ldapConnection,
        &ldapVersion, &ldapTimeout, this->uri.c_str(), this->logindn.c_str(), this->loginpass.c_str() );


// clean
    //this->dumpAll( this->ldapConnectionAdmin, "cn=config" );
    if( this->ldapConnected == true ) this->dumpAll( this->ldapConnection, "dc=evilbrain,dc=de" );



// demo
//    this->dbRemove( "olcDatabase={1}bdb" );
    this->dbAdd( "dc=evilbrain,dc=de" );
    this->dbChangeCreds( "dc=evilbrain,dc=de", "admin", "secret" );


    this->purge();
    this->purge();
    this->purge();
    this->purge();
    this->purge();

    //this->exist( "dc=evilbrain,dc=de" );

// create base groups
    std::string tempString = "";


    this->orgaAdd( "evilbrain", "de" );
    this->dumpAll( this->ldapConnection, "" );

    tempString = this->userdn.substr(3);
    this->orgaUnitAdd( tempString.c_str() );


    tempString = this->groupdn.substr(3);
    this->orgaUnitAdd( tempString.c_str() );


    this->userAdd( "testuser" );


// to json
    json_t* jsonObject = json_object();
//    this->dumpChilds( this->ldapConnection, "dc=evilbrain,dc=de", jsonObject );
    this->dumpChilds( this->ldapConnectionAdmin, "cn=config", jsonObject );
    char* jsonString = NULL;
    jsonString = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_INDENT(4) );
    fprintf( stdout, "%s\n", jsonString );
    free( jsonString );

}


ldapService::                   ~ldapService(){

}


bool ldapService::              configLoad(){

// vars
    bool needToSave = false;

    json_error_t jsonError;
    this->jsonObjectConfig = json_load_file( baseFilePath "ldap.json", JSON_PRESERVE_ORDER, &jsonError );
    if( jsonError.position == 0 || jsonError.line >= 0 ){
        this->jsonObjectConfig = json_object();
        needToSave = true;
    }

// load config from json
    this->configFromJson( this->jsonObjectConfig );


// save if needed
    if( needToSave == true ){
        this->configSave();
    }

}


bool ldapService::              configToJson( json_t* jsonObject ){

    coCore::jsonValue( jsonObject, "uri", &this->uri, "ldap://localhost/", true );

    coCore::jsonValue( jsonObject, "logindn", &this->logindn, "cn=Manager,dc=evilbrain,dc=de", true );
    coCore::jsonValue( jsonObject, "loginpass", &this->loginpass, "secret", true );

    coCore::jsonValue( jsonObject, "basedn", &this->basedn, "dc=evilbrain,dc=de", true );
    coCore::jsonValue( jsonObject, "groupdn", &this->groupdn, "ou=groups", true );
    coCore::jsonValue( jsonObject, "userdn", &this->userdn, "ou=accounts", true );


    return true;
}


bool ldapService::              configFromJson( json_t* jsonObject ){

    coCore::jsonValue( jsonObject, "uri", &this->uri, "ldap://localhost/", false );

    coCore::jsonValue( jsonObject, "admindn", &this->admindn, "cn=admin,cn=config", false );
    coCore::jsonValue( jsonObject, "adminpass", &this->adminpass, "secret", false );

    coCore::jsonValue( jsonObject, "logindn", &this->logindn, "cn=admin,dc=evilbrain,dc=de", false );
    coCore::jsonValue( jsonObject, "loginpass", &this->loginpass, "secret", false );

    coCore::jsonValue( jsonObject, "basedn", &this->basedn, "dc=evilbrain,dc=de", false );
    coCore::jsonValue( jsonObject, "groupdn", &this->groupdn, "ou=groups", false );
    coCore::jsonValue( jsonObject, "userdn", &this->userdn, "ou=accounts", false );

}


bool ldapService::              configSave(){
    json_dump_file( this->jsonObjectConfig, baseFilePath "ldap.json", JSON_PRESERVE_ORDER );
    return true;
}


bool ldapService::              connect( LDAP** connection, int* ldapVersion, int* ldapTimeout, const char* uri, const char* logindn, const char* pass ){
// vars
    LDAP*   ldapConnection = NULL;
    int     returnCode = -1;

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
        *connection = NULL;
        return false;
    }

// debug message
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Connected" );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    *connection = ldapConnection;
    return true;
}






bool ldapService::              exist( LDAP* connection, const char* dn ){
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
bool ldapService::              find( LDAP* connection, const char* searchBase, const char* attribute, const char* value, std::string* dn ){
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


bool ldapService::              iterate( const char* searchBase ){
// not connected
    if( this->ldapConnected == false ) return false;


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


bool ldapService::              dump( LDAP* connection, LDAPMessage* actualMessage ){
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
        }

        attributeName = ldap_next_attribute( connection, actualMessage, attributes );
    }

    fflush( stdout );
    return true;
}


bool ldapService::              dumpAll( LDAP* connection, const char* basedn ){
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
        this->dump( connection, actualMessage );
        ldap_memfree( entryDN );

        actualMessage = ldap_next_entry( connection, actualMessage );
    }
}


bool ldapService::              dumpChilds( LDAP* connection, const char* basedn, json_t* jsonObjectOutput ){
    if( connection == NULL ) return false;

// vars
    LDAPMessage*        actualMessages = NULL;
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;

// iterate
    ldap_search_ext_s( connection, basedn, LDAP_SCOPE_ONE,
        NULL, NULL, 0, NULL, NULL,
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




// admin server tasks

bool ldapService::              dbAdd( const char* suffix, const char* dbType ){
    if( this->ldapConnectionAdminActive == false || this->ldapConnectionAdmin == NULL ) return false;

//
    LDAPMod*        mods[5];
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

// create directory
    tempCommand  = "mkdir -p ";
    tempCommand += dbDirectory;
    system( tempCommand.c_str() );
    tempCommand  = "chown -R ldap:ldap ";
    tempCommand += dbDirectory;
    system( tempCommand.c_str() );

// attribute - objectClass
    LDAPMod modattr_class1;
    char*   modattr_class1_values[] = { (char*)"olcBdbConfig", NULL };

    modattr_class1.mod_op = LDAP_MOD_ADD;
    modattr_class1.mod_type = (char*)"objectClass";
    modattr_class1.mod_vals.modv_strvals = modattr_class1_values;

// attribute - olcDatabase
    LDAPMod modattr_olcDatabase;
    char*   modattr_olcDatabase_values[] = { (char*)dbType, NULL };

    modattr_olcDatabase.mod_op = LDAP_MOD_ADD;
    modattr_olcDatabase.mod_type = (char*)"olcDatabase";
    modattr_olcDatabase.mod_vals.modv_strvals = modattr_olcDatabase_values;

// attribute - olcDbDirectory
    LDAPMod modattr_olcDbDirectory;
    char*   modattr_olcDbDirectory_values[] = { (char*)dbDirectory.c_str(), NULL };

    modattr_olcDbDirectory.mod_op = LDAP_MOD_ADD;
    modattr_olcDbDirectory.mod_type = (char*)"olcDbDirectory";
    modattr_olcDbDirectory.mod_vals.modv_strvals = modattr_olcDbDirectory_values;

// attribute - olcSuffix
    LDAPMod modattr_olcSuffix;
    char*   modattr_olcSuffix_values[] = { (char*)suffix, NULL };

    modattr_olcSuffix.mod_op = LDAP_MOD_ADD;
    modattr_olcSuffix.mod_type = (char*)"olcSuffix";
    modattr_olcSuffix.mod_vals.modv_strvals = modattr_olcSuffix_values;


// build mods
    mods[0] = &modattr_class1;
    mods[1] = &modattr_olcDatabase;
    mods[2] = &modattr_olcDbDirectory;
    mods[3] = &modattr_olcSuffix;
    mods[4] = NULL;


// build dn
    fullDN  = "olcDatabase=";
    fullDN += dbType;
    fullDN += ",cn=config";


// call
    returnCode = ldap_add_ext_s( this->ldapConnectionAdmin, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        return false;
    }


}


bool ldapService::              dbChangeCreds( const char* suffix, const char* username, const char* password ){

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
    fullUserName  = "cn=";
    fullUserName += username;
    fullUserName += ",";
    fullUserName += suffix;


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


bool ldapService::              dbRemove( const char* configdn ){

// vars
    LDAPMessage*        actualMessage = NULL;
    int                 returnCode = 0;


// iterate over messages
    returnCode = ldap_delete_ext_s( this->ldapConnectionAdmin, configdn, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error on delete dn '%s': %s", configdn, errorMessage );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

        return false;
    }

    return true;
}




// tasks on an db

bool ldapService::              purge(){
// not connected
    if( this->ldapConnected == false || this->ldapConnection == NULL ) return false;

// vars
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;
    int                 returnCode = 0;

// iterate
    this->iterate( this->basedn.c_str() );

// iterate over messages
    actualMessage = ldap_first_entry( this->ldapConnection, this->resultMessages );
    while( actualMessage != NULL ){

        entryDN = ldap_get_dn( this->ldapConnection, actualMessage );

        returnCode = ldap_delete_ext_s( this->ldapConnection, entryDN, NULL, NULL );
        if( returnCode != LDAP_SUCCESS ){
            char *errorMessage = ldap_err2string( returnCode );
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error on delete dn '%s': %s", entryDN, errorMessage );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        }

        actualMessage = ldap_next_entry( this->ldapConnection, actualMessage );
    }

}


bool ldapService::              orgaAdd( const char* name, const char* domain ){
// not connected
    if( this->ldapConnected == false || this->ldapConnection == NULL ) return false;

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
    LDAPMod*        mods[3];
    int             returnCode;
    std::string     fullDN;

// attribute - objectClass
    LDAPMod modattr_class1;
    char*   modattr_class1_values[] = { (char*)"dcObject", (char*)"organization", NULL };

    modattr_class1.mod_op = LDAP_MOD_ADD;
    modattr_class1.mod_type = (char*)"objectClass";
    modattr_class1.mod_vals.modv_strvals = modattr_class1_values;


// attribute - o
    LDAPMod modattr_o;
    char*   modattr_o_values[] = { (char*)name, NULL };

    modattr_o.mod_op = LDAP_MOD_ADD;
    modattr_o.mod_type = (char*)"o";
    modattr_o.mod_vals.modv_strvals = modattr_o_values;

// build mods
    mods[0] = &modattr_class1;
    mods[1] = &modattr_o;
    mods[2] = NULL;


// build dn
    fullDN  = "dc=";
    fullDN += name;
    fullDN += ",";
    fullDN += "dc=";
    fullDN += domain;


// call
    returnCode = ldap_add_ext_s( this->ldapConnection, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        return false;
    }



    return true;
}


bool ldapService::              orgaUnitAdd( const char *orgaName, const char* base ){
// not connected
    if( this->ldapConnected == false || this->ldapConnection == NULL ) return false;


// create account-group
    LDAPMod*        mods[3];
    int             returnCode;
    std::string     fullDN;

// attribute - objectClass
    LDAPMod modattr_class1;
    char*   modattr_class1_values[] = { (char*)"organizationalUnit", NULL };

    modattr_class1.mod_op = LDAP_MOD_ADD;
    modattr_class1.mod_type = (char*)"objectClass";
    modattr_class1.mod_vals.modv_strvals = modattr_class1_values;

// attribute - ou
    LDAPMod modattr_ou;
    char*   modattr_ou_values[] = { (char*)orgaName, NULL };

    modattr_ou.mod_op = LDAP_MOD_ADD;
    modattr_ou.mod_type = (char*)"ou";
    modattr_ou.mod_vals.modv_strvals = modattr_ou_values;

    mods[0] = &modattr_class1;
    mods[1] = &modattr_ou;
    mods[2] = NULL;

// build dn
    fullDN  = "ou=";
    fullDN += orgaName;
    fullDN += ",";
    if( base != NULL ){
        fullDN += base;
    } else {
        fullDN += this->basedn;
    }

// call
    returnCode = ldap_add_ext_s( this->ldapConnection, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        return false;
    }

    return true;
}


bool ldapService::              groupAdd( const char *name, const char* description, const char *firstMember ){
// not connected
    if( this->ldapConnected == false || this->ldapConnection == NULL ) return false;



// create account-group
    LDAPMod*        mods[4];
    int             returnCode;
    std::string     fullDN;

// attribute - objectClass
    LDAPMod modattr_class1;
    char*   modattr_class1_values[] = { (char*)"top", (char*)"groupOfNames", NULL };

    modattr_class1.mod_op = LDAP_MOD_ADD;
    modattr_class1.mod_type = (char*)"objectClass";
    modattr_class1.mod_vals.modv_strvals = modattr_class1_values;

// attribute - cn
    LDAPMod modattr_cn;
    char*   modattr_cn_values[] = { (char*)name, NULL };

    modattr_cn.mod_op = LDAP_MOD_ADD;
    modattr_cn.mod_type = (char*)"cn";
    modattr_cn.mod_vals.modv_strvals = modattr_cn_values;

// attribute - member
    LDAPMod modattr_ou;
    char*   modattr_ou_values[] = { (char*)firstMember, NULL };

    modattr_ou.mod_op = LDAP_MOD_ADD;
    modattr_ou.mod_type = (char*)"member";
    modattr_ou.mod_vals.modv_strvals = modattr_ou_values;



    mods[0] = &modattr_class1;
    mods[1] = &modattr_cn;
    mods[2] = &modattr_ou;
    mods[3] = NULL;

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
        return false;
    }

    return true;
}


bool ldapService::              userAdd( const char *name ){
// not connected
    if( this->ldapConnected == false || this->ldapConnection == NULL ) return false;



// create account-group
    LDAPMod*        mods[5];
    int             returnCode;
    std::string     fullDN;

// attribute - objectClass
    LDAPMod modattr_class;
    char*   modattr_class_values[] = { /*(char*)"top",*/ (char*)"account", (char*)"shadowAccount", NULL };

    modattr_class.mod_op = LDAP_MOD_ADD;
    modattr_class.mod_type = (char*)"objectClass";
    modattr_class.mod_vals.modv_strvals = modattr_class_values;


// attribute - uid
    LDAPMod modattr_uid;
    char*   modattr_uid_values[] = { (char*)name, NULL };

    modattr_uid.mod_op = LDAP_MOD_ADD;
    modattr_uid.mod_type = (char*)"uid";
    modattr_uid.mod_vals.modv_strvals = modattr_uid_values;

/*
// attribute - member
    LDAPMod modattr_ou;
    char*   modattr_ou_values[] = { (char*)firstMember, NULL };

    modattr_ou.mod_op = LDAP_MOD_ADD;
    modattr_ou.mod_type = (char*)"member";
    modattr_ou.mod_vals.modv_strvals = modattr_ou_values;
*/


    mods[0] = &modattr_class;
    mods[1] = &modattr_uid;
    mods[2] = NULL;
    mods[3] = NULL;
    mods[4] = NULL;

// build dn
    fullDN  = "uid=";
    fullDN += name;
    fullDN += ",";
    fullDN += this->userdn;
    fullDN += ",";
    fullDN += this->basedn;

// call
    returnCode = ldap_add_ext_s( this->ldapConnection, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        return false;
    }

    return true;
}



#endif // ldapService_C
