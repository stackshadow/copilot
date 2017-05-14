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



ldapService::                   ldapService() : coPlugin( "ldap" ) {

// setup timeout
    this->searchTimeout.tv_sec = 20;
    this->searchTimeout.tv_usec = 0;


    this->configLoad();
    this->connect();

// clean
    this->dumpAll();
    this->purge();
    this->purge();
    this->purge();
    this->purge();
    this->purge();
    this->dumpAll();

    this->exist( "dc=evilbrain,dc=de" );

// create base groups
    std::string tempString = "";

    tempString = this->userdn.substr(3);
    this->orgaAdd( tempString.c_str() );


    tempString = this->groupdn.substr(3);
    this->orgaAdd( tempString.c_str() );


    this->userAdd( "testuser" );
}


ldapService::                   ~ldapService(){

}


bool ldapService::              configLoad(){

    json_error_t jsonError;
    this->jsonObjectConfig = json_load_file( configFile("ldap.json"), JSON_PRESERVE_ORDER, &jsonError );
    if( jsonError.position == 0 || jsonError.line >= 0 ){
        this->jsonObjectConfig = json_object();
    }

// load config from json
    this->configFromJson( this->jsonObjectConfig );

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

    coCore::jsonValue( jsonObject, "logindn", &this->logindn, "cn=Manager,dc=evilbrain,dc=de", false );
    coCore::jsonValue( jsonObject, "loginpass", &this->loginpass, "secret", false );

    coCore::jsonValue( jsonObject, "basedn", &this->basedn, "dc=evilbrain,dc=de", false );
    coCore::jsonValue( jsonObject, "groupdn", &this->groupdn, "ou=groups", false );
    coCore::jsonValue( jsonObject, "userdn", &this->userdn, "ou=accounts", false );

}


bool ldapService::              connect(){

// vars
    int returnCode = -1;
    int ldapVersion = LDAP_VERSION3;
    int ldapTimeout = 30;
    this->connected = false;

// init
    returnCode = ldap_initialize( &this->ldapConnection, this->uri.c_str() );
    if( returnCode != LDAP_SUCCESS ){
        return false;
    }

// set protocoll version

    returnCode = ldap_set_option( this->ldapConnection, LDAP_OPT_PROTOCOL_VERSION, (void*)&ldapVersion );

    returnCode = ldap_set_option( this->ldapConnection, LDAP_OPT_NETWORK_TIMEOUT, (void*)&ldapTimeout );


// login
    struct berval credentials;
    struct berval* servercredp;
    credentials.bv_val = (char*)this->loginpass.c_str();
    credentials.bv_len = 6;


// connect
    returnCode = ldap_sasl_bind_s(
        this->ldapConnection,
        this->logindn.c_str(),
        NULL,
        &credentials,
        NULL,
        NULL,
        &servercredp );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        return false;
    }

// connected
    this->connected = true;

// debug message
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Connected" );
    etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );




}




bool ldapService::              exist( const char* dn ){


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
        this->ldapConnection,
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
    actualMessage = ldap_first_entry( this->ldapConnection, returnMessage );
    while( actualMessage != NULL ){
        return true;
    }

// nothing found
    return false;
}


bool ldapService::              iterate( const char* searchBase ){
// not connected
    if( this->connected == false ) return false;


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


bool ldapService::              dump( LDAPMessage* actualMessage ){

// vars
    const char*     entryDN = NULL;
    BerElement*     attributes = NULL;
    char*           attributeName = NULL;
    struct berval** attributeValues = NULL;
    struct berval*  attributeValue = NULL;
    int             attributeValueCount = 0;

// get the entry DN
    entryDN = ldap_get_dn( this->ldapConnection, actualMessage );
    fprintf( stdout, "-------------- entry ------------\ndn: %s\n", entryDN );
    fflush( stdout );

// iterate attributes
    attributeName = ldap_first_attribute( this->ldapConnection, actualMessage, &attributes );
    while( attributeName != NULL ){
        attributeValues = ldap_get_values_len( this->ldapConnection, actualMessage, attributeName );
        attributeValueCount = ldap_count_values_len( attributeValues );

        for( ; attributeValueCount > 0; attributeValueCount-- ){
            attributeValue = attributeValues[attributeValueCount-1];
            fprintf( stdout, "attribute: %s=%s\n", attributeName, attributeValue->bv_val );
        }
        fflush( stdout );

        attributeName = ldap_next_attribute( this->ldapConnection, actualMessage, attributes );
    }

    return true;
}


bool ldapService::              dumpAll(){
// not connected
    if( this->connected == false ) return false;

// vars
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;

// iterate
    this->iterate( this->basedn.c_str() );

// iterate over messages
    actualMessage = ldap_first_entry( this->ldapConnection, this->resultMessages );
    while( actualMessage != NULL ){

        entryDN = ldap_get_dn( this->ldapConnection, actualMessage );

    // dump the entry
        this->dump( actualMessage );

        actualMessage = ldap_next_entry( this->ldapConnection, actualMessage );
    }
}




bool ldapService::              purge(){
// not connected
    if( this->connected == false ) return false;

// vars
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;

// iterate
    this->iterate( this->basedn.c_str() );

// iterate over messages
    actualMessage = ldap_first_entry( this->ldapConnection, this->resultMessages );
    while( actualMessage != NULL ){

        entryDN = ldap_get_dn( this->ldapConnection, actualMessage );

        ldap_delete_ext_s( this->ldapConnection, entryDN, NULL, NULL );

        actualMessage = ldap_next_entry( this->ldapConnection, actualMessage );
    }

}




bool ldapService::              orgaAdd( const char *orgaName, const char* base ){


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



// create account-group
    LDAPMod*        mods[5];
    int             returnCode;
    std::string     fullDN;

// attribute - objectClass
    LDAPMod modattr_class;
    char*   modattr_class_values[] = { (char*)"top", (char*)"account", (char*)"shadowAccount", NULL };

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
