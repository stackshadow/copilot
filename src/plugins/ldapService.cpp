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
#include "pubsub.h"


ldapService::                       ldapService() {

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
// subscribe
	psBus::inst->subscribe( this, coCore::ptr->nodeName(), "ldap", NULL, ldapService::onSubscriberMessage, NULL );
}


ldapService::                       ~ldapService(){

}




int ldapService::                   onSubscriberMessage( void* objectSource, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, void* userdata ){

    // vars
    int                 msgCommandLen = 0;
    ldapService*		ldapServiceInst = (ldapService*)objectSource;
    const char*			myNodeName = coCore::ptr->nodeName();
    char*               jsonTempString = NULL;

    // check
    if( command == NULL ) return psBus::END;
    msgCommandLen = strlen(command);


    std::string     tempString;


    if( coCore::strIsExact("configGet",command,msgCommandLen) == true ){

    // copy
        json_t* configCopy = json_deep_copy( ldapServiceInst->jsonObjectConfig );
        
    // remove it from object
        json_object_del( configCopy, "adminpass" );
        json_object_del( configCopy, "loginpass" );

    // dump
        jsonTempString = json_dumps( configCopy, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "config", jsonTempString );

    // cleanup
        free( jsonTempString );
        json_decref( configCopy );


        return psBus::END;
    }


    if( coCore::strIsExact("configSet",command,msgCommandLen) == true ){

    // vars
        json_error_t    jsonError;
        json_t*         jsonNewValues = NULL;
        void*           jsonIterator = NULL;
        const char*     jsonKey = NULL;
        json_t*         jsonValue = NULL;

    // parse json
        jsonNewValues = json_loads( payload, JSON_PRESERVE_ORDER, &jsonError );
        if( jsonNewValues == NULL || jsonError.line > -1 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: %s", __PRETTY_FUNCTION__, jsonError.text );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

            return psBus::END;
        }

    // iterate values and update internal config
        jsonIterator = json_object_iter( jsonNewValues );
        while( jsonIterator != NULL ){
            jsonKey = json_object_iter_key(jsonIterator);
            jsonValue = json_object_iter_value(jsonIterator);

            json_object_set_new( ldapServiceInst->jsonObjectConfig, jsonKey, json_string( json_string_value(jsonValue)) );

            jsonIterator = json_object_iter_next( jsonNewValues, jsonIterator );
        }

    // save
        ldapServiceInst->configSave();

    // destroy temporary object
        json_decref( jsonNewValues );

    // send message
        psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "configSaved", "" );

        return psBus::END;
    }


    if( coCore::strIsExact("connect",command,msgCommandLen) == true ){
    // connect
        int ldapVersion = LDAP_VERSION3;
        int ldapTimeout = 30;


    // admin-connection
        etDebugMessage( etID_LEVEL_DETAIL_APP, "Try to connect as Admin" );
        ldapServiceInst->ldapConnectionAdminActive = ldapServiceInst->connect( 
            &ldapServiceInst->ldapConnectionAdmin,
            &ldapVersion, 
            &ldapTimeout, 
            ldapServiceInst->uri.c_str(), 
            ldapServiceInst->admindn.c_str(), 
            ldapServiceInst->adminpass.c_str() 
        );

    // dbconnection
        etDebugMessage( etID_LEVEL_DETAIL_APP, "Try to connect as Manager" );
        ldapServiceInst->ldapConnectionActive = ldapServiceInst->connect( 
            &ldapServiceInst->ldapConnection,
            &ldapVersion,
            &ldapTimeout,
            ldapServiceInst->uri.c_str(),
            ldapServiceInst->logindn.c_str(),
            ldapServiceInst->loginpass.c_str()
        );


    // create db
        if( ldapServiceInst->dbAdd( ldapServiceInst->basedn.c_str(), "mdb" ) == true ){

        // set username and password
            ldapServiceInst->dbChangeCreds( ldapServiceInst->basedn.c_str(), ldapServiceInst->logindn.c_str(), ldapServiceInst->loginpass.c_str() );
        //this->attributeAdd( "olcDatabase={1}mdb,cn=config", "olcAccess", "to * by dn=\"cn=admin,cn=config\" write" );
        }

    // change
        ldapServiceInst->dbChangeCreds( ldapServiceInst->basedn.c_str(), ldapServiceInst->logindn.c_str(), ldapServiceInst->loginpass.c_str() );

    // add hashing type
        ldapServiceInst->dbAddHash( "{SSHA}" );

    // add orga if needed
        ldapServiceInst->orgaAdd( ldapServiceInst->basedn.c_str() );
    // create user-tree
        tempString = ldapServiceInst->userdn.substr(3);
        ldapServiceInst->orgaUnitAdd( tempString.c_str() );
    // create group-tree
        tempString = ldapServiceInst->groupdn.substr(3);
        ldapServiceInst->orgaUnitAdd( tempString.c_str() );

    // create dummyMember for new groups
        ldapServiceInst->userAdd( "dummyMember" );

        goto connstate;
        return psBus::END;
    }


    if( coCore::strIsExact("disconnect",command,msgCommandLen) == true ){

        ldap_unbind_ext_s( ldapServiceInst->ldapConnectionAdmin, NULL, NULL );
        ldapServiceInst->ldapConnectionAdminActive = false;
        ldapServiceInst->ldapConnectionAdmin = NULL;

        ldap_unbind_ext_s( ldapServiceInst->ldapConnection, NULL, NULL );
        ldapServiceInst->ldapConnectionActive = false;
        ldapServiceInst->ldapConnection = NULL;

        goto connstate;
        return psBus::END;
    }


    if( coCore::strIsExact("stateGet",command,msgCommandLen) == true ){
    connstate:
    // connected ?
        if( ldapServiceInst->ldapConnectionAdminActive && ldapServiceInst->ldapConnectionActive ){
            psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "state", "connected" );
        } else {
            psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "state", "disconnected" );
        }

        return psBus::END;
    }



    // user stuff
    if( coCore::strIsExact("userlist",command,msgCommandLen) == true ){

    // connected ?
        if( ldapServiceInst->ldapConnection == NULL || ldapServiceInst->ldapConnectionActive == false ){
            return psBus::END;
        }

    // vars
        json_t*     jsonUserlist = json_object();

    // add users to json
        ldapServiceInst->userDump( ldapServiceInst->ldapConnection, jsonUserlist );

    // dump
        jsonTempString = json_dumps( jsonUserlist, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "users", jsonTempString );

    // cleanup
        free( jsonTempString );
        json_decref(jsonUserlist);

        return psBus::END;
    }


    if( coCore::strIsExact("userGet",command,msgCommandLen) == true ){

    // connected ?
        if( ldapServiceInst->ldapConnection == NULL || ldapServiceInst->ldapConnectionActive == false ){
            return psBus::END;
        }
        
        if( payload == NULL ){
            etDebugMessage( etID_LEVEL_ERR, "can not get user. Username missing" );
            return psBus::END;
        }

    // vars
        json_t*         jsonUserlist = json_object();
        std::string     fullDN;

    // build dn
        fullDN  = "uid=";
        fullDN += payload;
        fullDN += ",";
        fullDN += ldapServiceInst->userdn;
        fullDN += ",";
        fullDN += ldapServiceInst->basedn;

    // add the message to list
        const char* attributes[] = { "uid", "mail", NULL };
        ldapServiceInst->dump( jsonUserlist, attributes, fullDN.c_str(), true );

    // dump
        jsonTempString = json_dumps( jsonUserlist, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "user", jsonTempString );

    // cleanup
        free( jsonTempString );
        json_decref(jsonUserlist);

        return psBus::END;
    }

    // modificate a user ( add / change / delete )
    if( coCore::strIsExact("userMod",command,msgCommandLen) == true ){
    // connected ?
        if( ldapServiceInst->ldapConnection == NULL || ldapServiceInst->ldapConnectionActive == false ){
            psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "userNotChanged", "no connection" );
            return psBus::END;
        }

    // vars
        json_error_t    jsonError;
        json_t*         jsonNewValues = NULL;
        void*           jsonIterator = NULL;
        const char*     jsonKey = NULL;
        json_t*         jsonValue = NULL;

    // parse json
        jsonNewValues = json_loads( payload, JSON_PRESERVE_ORDER, &jsonError );
        if( jsonNewValues == NULL || jsonError.line > -1 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: %s", __PRETTY_FUNCTION__, jsonError.text );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

            return psBus::END;
        }

    // action ( 1=add, 2=change, 3=delete )
        int action = 0;
        jsonValue = json_object_get( jsonNewValues, "action" );
        if( jsonValue == NULL ){
            json_decref( jsonNewValues );
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: No 'action' in json-object", __PRETTY_FUNCTION__ );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return psBus::END;
        }
        action = json_integer_value( jsonValue );


    // username
        const char* userName = NULL;
        jsonValue = json_object_get( jsonNewValues, "uid" );
        if( jsonValue == NULL ){
            json_decref( jsonNewValues );
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: No 'uid' in json-object", __PRETTY_FUNCTION__ );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return psBus::END;

        }
        userName = json_string_value( jsonValue );


    // add
        if( action == 1 ){
            if( ldapServiceInst->userAdd( userName ) == true ){

            // we change to command to user-change, to add additional values ( if possible )
            // we also dont return from this function !
                action = 2;

                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "userAdded", userName );
            } else {
                
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "userNotAdded", userName );

                json_decref( jsonNewValues );
                return psBus::END;
            }
        }

    // change
        if( action == 2 ){

        // password
            const char* userPassword = NULL;
            jsonValue = json_object_get( jsonNewValues, "pw" );
            if( jsonValue != NULL ){
                userPassword = json_string_value( jsonValue );
            }

        // mail
            const char* userMail = NULL;
            jsonValue = json_object_get( jsonNewValues, "mail" );
            if( jsonValue != NULL ){
                userMail = json_string_value( jsonValue );
            }

            if( ldapServiceInst->userChange( userName, userPassword, userMail ) ){
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "userChanged", userName );
            } else {
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "userNotChanged", userName );
            }

            json_decref( jsonNewValues );
            return psBus::END;
        }

    // delete
        if( action == 3 ){

        // we can not delete dummyMember !
            if( strncmp( "uid=dummyMember",userName, 14 ) == 0 ){
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "userNotDeleted", userName );
                json_decref( jsonNewValues );
                return psBus::END;
            }

        // here the userName is DN
        ///@todo Check if its an DN !

        // try to remove it
            if( ldapServiceInst->userDelete( userName ) == true ){
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "userDeleted", userName );
            } else {
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "userNotDeleted", userName );
            }
        }

        json_decref( jsonNewValues );
        return psBus::END;
    }


    if( coCore::strIsExact("userMembersGet",command,msgCommandLen) == true ){
    // connected ?
        if( ldapServiceInst->ldapConnection == NULL || ldapServiceInst->ldapConnectionActive == false ){
            return psBus::END;
        }

    // vars
        json_t*         jsonGroups = json_object();
        std::string     fullDN;


    // add the message to list
        ldapServiceInst->userDumpMembership( payload, jsonGroups );

    // dump
        jsonTempString = json_dumps( jsonGroups, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "userMembers", jsonTempString );

    // cleanup
        free( jsonTempString );
        json_decref(jsonGroups);
        return psBus::END;
    }

    // group get
    if( coCore::strIsExact("grouplist",command,msgCommandLen) == true ){

    // connected ?
        if( ldapServiceInst->ldapConnection == NULL || ldapServiceInst->ldapConnectionActive == false ){
            return psBus::END;
        }

    // vars
        json_t*     jsonGrouplist = json_object();

    // add users to json
        ldapServiceInst->groupDump( jsonGrouplist );

    // dump
        jsonTempString = json_dumps( jsonGrouplist, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groups", jsonTempString );

    // cleanup
        free( jsonTempString );
        json_decref( jsonGrouplist );

        return psBus::END;
    }


    if( coCore::strIsExact("groupGet",command,msgCommandLen) == true ){

    // connected ?
        if( ldapServiceInst->ldapConnection == NULL || ldapServiceInst->ldapConnectionActive == false ){
            return psBus::END;
        }

    // vars
        json_t*         jsonGrouplist = json_object();
        std::string     fullDN;

    // build dn
        fullDN  = "cn=";
        fullDN += payload;
        fullDN += ",";
        fullDN += ldapServiceInst->groupdn;
        fullDN += ",";
        fullDN += ldapServiceInst->basedn;

    // dump ldap to json
        const char* attributes[] = { "cn", "description", NULL };
        ldapServiceInst->dump( jsonGrouplist, attributes, fullDN.c_str(), true );

    // dump json to string
        jsonTempString = json_dumps( jsonGrouplist, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "group", jsonTempString );

    // cleanup
        free( jsonTempString );
        json_decref( jsonGrouplist );

        return psBus::END;
    }


    if( coCore::strIsExact("groupmemberlist",command,msgCommandLen) == true ){

    // connected ?
        if( ldapServiceInst->ldapConnection == NULL || ldapServiceInst->ldapConnectionActive == false ){
            return psBus::END;
        }

    // vars
        json_t*     jsonMemberList = json_object();

    // add users to json
        ldapServiceInst->groupMembersDump( payload, jsonMemberList );

    // dump
        jsonTempString = json_dumps( jsonMemberList, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupmembers", jsonTempString );

    // cleanup
        free( jsonTempString );
        json_decref(jsonMemberList);

        return psBus::END;
    }

    // modificate a user ( add / change / delete )
    if( coCore::strIsExact("groupMod",command,msgCommandLen) == true ){
    // vars
        json_error_t    jsonError;
        json_t*         jsonNewValues = NULL;
        void*           jsonIterator = NULL;
        const char*     jsonKey = NULL;
        json_t*         jsonValue = NULL;

    // parse json
        jsonNewValues = json_loads( payload, JSON_PRESERVE_ORDER, &jsonError );
        if( jsonNewValues == NULL || jsonError.line > -1 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: %s", __PRETTY_FUNCTION__, jsonError.text );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

            return psBus::END;
        }

    // action ( 1=add, 2=change, 3=delete )
        int action = 0;
        jsonValue = json_object_get( jsonNewValues, "action" );
        if( jsonValue == NULL ){
            json_decref( jsonNewValues );
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: No 'action' in json-object", __PRETTY_FUNCTION__ );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return psBus::END;
        }
        action = json_integer_value( jsonValue );
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: got action '%i'", __PRETTY_FUNCTION__, action );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

    // groupname
        const char* groupName = NULL;
        jsonValue = json_object_get( jsonNewValues, "cn" );
        if( jsonValue == NULL ){
            json_decref( jsonNewValues );
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: No 'cn' in json-object", __PRETTY_FUNCTION__ );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return psBus::END;
        }
        groupName = json_string_value( jsonValue );


    // add
        if( action == 1 ){
            if( ldapServiceInst->groupAdd( groupName ) == true ){

            // we change to command to user-change, to add additional values ( if possible )
            // we also dont return from this function !
                action = 2;

                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupAdded", groupName );
            } else {
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupNotAdded", groupName );

                json_decref( jsonNewValues );
                return psBus::END;
            }
        }

    // change
        if( action == 2 ){
        // password
            const char* description = NULL;
            jsonValue = json_object_get( jsonNewValues, "description" );
            if( jsonValue != NULL ){
                description = json_string_value( jsonValue );
            }

            if( ldapServiceInst->groupChange( groupName, description ) ){
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupChanged", groupName );
            } else {
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupNotChanged", groupName );
            }

            json_decref( jsonNewValues );
            return psBus::END;
        }

    // delete
        if( action == 3 ){

        // here the userName is DN
        ///@todo Check if its an DN !

        // try to remove it
            if( ldapServiceInst->groupDelete( groupName ) ){
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupDeleted", groupName );
            } else {
                psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupNotDeleted", groupName );
            }

            json_decref( jsonNewValues );
            return psBus::END;
        }

    // get member to add/remove user from a group member
        if( action == 4 || action == 5 ){

        // password
            const char* memberUserName = NULL;
            jsonValue = json_object_get( jsonNewValues, "member" );
            if( jsonValue == NULL ){
                json_decref( jsonNewValues );
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: No 'member' in json-object", __PRETTY_FUNCTION__ );
                etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
                return psBus::END;
            }
            memberUserName = json_string_value( jsonValue );

        // add user to group
            if( action == 4 ){
                if( ldapServiceInst->groupAddMember( groupName, memberUserName ) ){
                    psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupChanged", groupName );
                } else {
                    psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupNotChanged", groupName );
                }
            }

        // remove user to group
            if( action == 5 ){
                if( ldapServiceInst->groupRemoveMember( groupName, memberUserName ) ){
                    psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupChanged", groupName );
                } else {
                    psBus::inst->publish( ldapServiceInst, id, myNodeName, nodeSource, "ldap", "groupNotChanged", groupName );
                }
            }

            json_decref( jsonNewValues );
            return psBus::END;
        }

        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: Action '%i' is not defined ", __PRETTY_FUNCTION__, action );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

        json_decref( jsonNewValues );
        return psBus::END;
    }






    return psBus::NEXT_SUBSCRIBER;
}





bool ldapService::                  configLoad(){

// vars
    bool needToSave = false;

// get config path
    this->jsonObjectConfig = coConfig::ptr->section( "ldap" );

// load config from json
    this->configFromJson( this->jsonObjectConfig );

    return true;
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
    
    return true;
}


bool ldapService::                  configSave(){
// save json to internal values
    this->configFromJson( this->jsonObjectConfig );

// return
    return coConfig::ptr->save();
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
		psBus::inst->publish( this, NULL, coCore::ptr->nodeName(), NULL, "ldap", "msgError", errorMessage );

        *connection = NULL;
        return false;
    }

// debug message
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Connected" );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );


    *connection = ldapConnection;
    return true;
}



bool ldapService::                  ldapModAppend( LDAPMod*** mod, int* LDAPModLen, int op, const char* property, const char* value1, const char* value2, const char* value3 ){

// vars
    int             newModLen = *LDAPModLen + 1;
    size_t          newModSize = sizeof(LDAPMod*) * newModLen;
    LDAPMod**       newMod = NULL;
    const char**    newModAttrValues = NULL;
    size_t          newModAttrValuesSize = sizeof(char*) * 4;

// initial mod
    if( *LDAPModLen == 0 ){
        newModLen = 2;
    }

// allocate modification struct
    newMod = (LDAPMod**)malloc( newModSize );

// if not delete, we create the attributes
    //if( op != LDAP_MOD_DELETE ){
    if( value1 != NULL || value2 != NULL || value3 != NULL ){

    // allocate
        newModAttrValues = (const char**)malloc( newModAttrValuesSize );

    // copy old pointer addr
        if( *mod != NULL ){
            memcpy( newMod, *mod, *LDAPModLen * sizeof(LDAPMod*) );
        }

    // values
        newModAttrValues[0] = value1;
        newModAttrValues[1] = value2;
        newModAttrValues[2] = value3;
        newModAttrValues[3] = NULL;

    }

// mod
    newMod[newModLen-2] = (LDAPMod*)malloc(sizeof(LDAPMod)); memset( newMod[newModLen-2], 0, sizeof(LDAPMod) );
    newMod[newModLen-2]->mod_op = op;
    newMod[newModLen-2]->mod_type = (char*)property;
    newMod[newModLen-1] = NULL;

// if all values are NULLm we set vals to NULL
    if( value1 != NULL || value2 != NULL || value3 != NULL ){

    //if( op != LDAP_MOD_DELETE ){
        newMod[newModLen-2]->mod_vals.modv_strvals = (char**)newModAttrValues;
    } else {
        newMod[newModLen-2]->mod_vals.modv_strvals = NULL;
    }


    *mod = newMod;
    *LDAPModLen = newModLen;

    return true;
}


bool ldapService::                  ldapModMemFree( LDAPMod*** mod ){
    if( mod == NULL ) return false;
    if( *mod == NULL ) return false;

    LDAPMod**       mods = *mod;
    LDAPMod*        actualMod;
    int             actualModIndex = 0;

    actualMod = mods[actualModIndex];
    while( actualMod != NULL ){

    // free values
        if( actualMod->mod_vals.modv_strvals != NULL ){
            free(actualMod->mod_vals.modv_strvals);
            actualMod->mod_vals.modv_strvals = NULL;
        }

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

    return true;
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


bool ldapService::                  removeDN( const char* dn ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;

// vars
    int                 returnCode = 0;

// delete dn
    returnCode = ldap_delete_ext_s( this->ldapConnection, dn, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error on delete dn '%s': %s", dn, errorMessage );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

        return false;
    }

    return true;
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

    return true;
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
    
    return true;
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


bool ldapService::                  dump( json_t* jsonObjectOutput, const char **attributes, const char* searchDN, bool singleDN, const char* LDAPFilter ){

// vars
    const char*         attributeName = NULL;
    int                 attributeIndex = 0;

    LDAPMessage*        actualMessageArray = NULL;
    LDAPMessage*        actualMessage = NULL;
    char*               entryDN = NULL;
    struct berval*      attributeValue = NULL;
    struct berval**     attributeValueArray = NULL;
    int                 attributeValuesCount = 0;
    int                 attributeValuesIndex = 0;


    int searchScope = LDAP_SCOPE_ONE;
    if( singleDN == true ){
        searchScope = LDAP_SCOPE_BASE;
    }

// iterate
    ldap_search_ext_s( this->ldapConnection, searchDN, searchScope, LDAPFilter, NULL, 0, NULL, NULL, &this->searchTimeout, 1024, &actualMessageArray );

// check
    if( actualMessageArray == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "No result aviable" );
        return false;
    }

// iterate over messages
    actualMessage = ldap_first_entry( this->ldapConnection, actualMessageArray );
    while( actualMessage != NULL ){

    // dn
        entryDN = ldap_get_dn( this->ldapConnection, actualMessage );

    // dn
        json_t* jsonDN = json_object();
        json_object_set_new( jsonDN, "dn", json_string(entryDN) );

    // iterate over attributes
        if( attributes != NULL ){
            attributeIndex = 0;
            attributeName = attributes[attributeIndex];
            while( attributeName != NULL ){

            // get attribute array
                attributeValueArray = ldap_get_values_len( this->ldapConnection, actualMessage, attributeName );
                attributeValuesCount = ldap_count_values_len( attributeValueArray );
                if( attributeValuesCount > 0 ){

                // the value object
                    json_t* jsonValueAObject = NULL;

                // if attribute has only one value, a string is enough
                    if( attributeValuesCount == 1 ){

                        attributeValue = attributeValueArray[0];
                        jsonValueAObject = json_string( attributeValue->bv_val );

                    }

                // an array
                    if( attributeValuesCount > 1 ){

                        jsonValueAObject = json_array();

                        attributeValuesIndex = 0;
                        for( attributeValuesIndex = 0; attributeValuesIndex < attributeValuesCount; attributeValuesIndex++ ){
                            attributeValue = attributeValueArray[attributeValuesIndex];
                            json_array_append( jsonValueAObject, json_string(attributeValue->bv_val) );
                        }

                    }

                    json_object_set_new( jsonDN, attributeName, jsonValueAObject );

                }
                ldap_value_free_len( attributeValueArray );

            // next attribute
                attributeIndex++;
                attributeName = attributes[attributeIndex];
            }
        }

    // add it to parent object
        json_object_set_new( jsonObjectOutput, entryDN, jsonDN );


        ldap_memfree( entryDN );
        actualMessage = ldap_next_entry( this->ldapConnection, actualMessage );
    }

    ldap_msgfree( actualMessageArray );
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




// build dn
    fullDN  = "olcDatabase=";
    fullDN += dbType;
    fullDN += ",cn=config";

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


bool ldapService::                  dbAddHash( const char* hash ){
// not connected
    if( this->ldapConnectionAdminActive == false || this->ldapConnectionAdmin == NULL ) return false;



// create account-group
    LDAPMod**       mods = NULL;
    int             modsLen = 0;
    int             returnCode;
    std::string     fullDN;


// Required Attributes
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_REPLACE, "olcPasswordHash", hash );


// build dn
    fullDN  = "olcDatabase={-1}frontend,cn=config";

// call
    returnCode = ldap_modify_ext_s( this->ldapConnectionAdmin, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }

    this->ldapModMemFree( &mods );
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

    return true;
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
    
    return true;
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


bool ldapService::                  orgaUnitAdd( const char* orgaName, const char* base ){
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




bool ldapService::                  groupDump( json_t* jsonObjectOutput ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;

// vars
    std::string         fullDN;

// build dn
    fullDN  = this->groupdn;
    fullDN += ",";
    fullDN += this->basedn;

    const char* attributes[] = { "cn", "description", NULL };
    this->dump( jsonObjectOutput, attributes, fullDN.c_str() );


    return true;
}


bool ldapService::                  groupMembersDump( const char* groupName, json_t* jsonObjectOutput ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;

// vars
    std::string         fullDN;

// build dn
    fullDN  = "cn=";
    fullDN += groupName;
    fullDN += ",";
    fullDN += this->groupdn;
    fullDN += ",";
    fullDN += this->basedn;


    const char* attributes[] = { "member", NULL };
    this->dump( jsonObjectOutput, attributes, fullDN.c_str() );


    return true;
}


bool ldapService::                  groupAdd( const char* name ){
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

// Required Attributes
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "objectClass", "groupOfNames" );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "cn", name );
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


bool ldapService::                  groupChange( const char* name, const char* description ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;
    if( name == NULL && description == NULL ) return false;



// create account-group
    LDAPMod**       mods = NULL;
    int             modsLen = 0;
    int             returnCode;
    std::string     fullDN;

// build dn
    fullDN  = "cn=";
    fullDN += name;
    fullDN += ",";
    fullDN += this->groupdn;
    fullDN += ",";
    fullDN += this->basedn;

// check if orga already exist
    if( this->exist( this->ldapConnection, fullDN.c_str() ) != true ){
        return false;
    }

// change description
    if( description != NULL ){
    // we can not set an empty description
        if( description[0] == '\0' ){
            this->ldapModAppend( &mods, &modsLen, LDAP_MOD_DELETE, "description", description );
        } else {
            this->ldapModAppend( &mods, &modsLen, LDAP_MOD_REPLACE, "description", description );
        }
    }

// call
    returnCode = ldap_modify_ext_s( this->ldapConnection, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }

    this->ldapModMemFree( &mods );
    return true;
}


bool ldapService::                  groupDelete( const char* name ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;
    if( name == NULL ) return false;

    
// create account-group
    std::string     fullDN;

// build dn
    fullDN  = "cn=";
    fullDN += name;
    fullDN += ",";
    fullDN += this->groupdn;
    fullDN += ",";
    fullDN += this->basedn;

    return this->removeDN( fullDN.c_str() );
}


bool ldapService::                  groupAddMember( const char* groupName, const char* memberUserName ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;
    if( groupName == NULL && memberUserName == NULL ) return false;


// create account-group
    LDAPMod**       mods;
    int             modsLen = 0;
    int             returnCode;
    std::string     groupDN;
    std::string     userDN;

// build dn
    groupDN  = "cn=";
    groupDN += groupName;
    groupDN += ",";
    groupDN += this->groupdn;
    groupDN += ",";
    groupDN += this->basedn;

// build dn
    userDN  = "uid=";
    userDN += memberUserName;
    userDN += ",";
    userDN += this->userdn;
    userDN += ",";
    userDN += this->basedn;

// check if orga already exist
    if( this->exist( this->ldapConnection, groupDN.c_str() ) != true ){
        return false;
    }
    if( this->exist( this->ldapConnection, userDN.c_str() ) != true ){
        return false;
    }



// change description
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "member", userDN.c_str() );
// call
    returnCode = ldap_modify_ext_s( this->ldapConnection, groupDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }
// clean
    this->ldapModMemFree( &mods );
	modsLen = 0;




// change description
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "memberOf", groupDN.c_str() );
// call
    returnCode = ldap_modify_ext_s( this->ldapConnection, userDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }
// clean
    this->ldapModMemFree( &mods );




    return true;
}


bool ldapService::                  groupRemoveMember( const char* groupName, const char* memberUserName ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;
    if( groupName == NULL && memberUserName == NULL ) return false;


// create account-group
    LDAPMod**       mods;
    int             modsLen = 0;
    int             returnCode;
    std::string     groupDN;
    std::string     userDN;

// build dn
    groupDN  = "cn=";
    groupDN += groupName;
    groupDN += ",";
    groupDN += this->groupdn;
    groupDN += ",";
    groupDN += this->basedn;

// build dn
    userDN  = "uid=";
    userDN += memberUserName;
    userDN += ",";
    userDN += this->userdn;
    userDN += ",";
    userDN += this->basedn;

// check if orga already exist
    if( this->exist( this->ldapConnection, userDN.c_str() ) != true ){
        return false;
    }

// change description
	modsLen = 0;
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_DELETE, "member", userDN.c_str() );
// call
    returnCode = ldap_modify_ext_s( this->ldapConnection, groupDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }
    this->ldapModMemFree( &mods );
	modsLen = 0;

// change description
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_DELETE, "memberOf", groupDN.c_str() );
// call
    returnCode = ldap_modify_ext_s( this->ldapConnection, userDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }
    this->ldapModMemFree( &mods );


    return true;
}





bool ldapService::                  userDump( LDAP* connection, json_t* jsonObjectOutput ){
    if( connection == NULL ) return false;

// vars
    std::string         fullDN;

// build dn
    fullDN  = this->userdn;
    fullDN += ",";
    fullDN += this->basedn;

    const char* attributes[] = { "sn", "cn", "uid", "mail", NULL };
    this->dump( jsonObjectOutput, attributes, fullDN.c_str() );


    return true;
}


bool ldapService::                  userDumpMembership( const char* uid, json_t* jsonObjectOutput ){

// vars
    std::string         filter;
    std::string         searchBase;

// build dn
    searchBase  = this->groupdn;
    searchBase += ",";
    searchBase += this->basedn;

// build filter
    filter  = "(&(member=uid=";
    filter += uid;
    filter += ",";
    filter += this->userdn;
    filter += ",";
    filter += this->basedn;
    filter += "))";

    const char* attributes[] = { "cn", NULL };
    this->dump( jsonObjectOutput, attributes, searchBase.c_str(), false, filter.c_str() );

    return true;
}


bool ldapService::                  userAdd( const char* accountName ){
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

// Required Attributes
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "objectClass", "top", "person", "inetOrgPerson" );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "uid", accountName );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "sn", "unknown" );
    this->ldapModAppend( &mods, &modsLen, LDAP_MOD_ADD, "cn", "unknown" );



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


bool ldapService::                  userChange( const char* accountName, const char* passwordClearText, const char* mail ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;
    if( passwordClearText == NULL && mail == NULL ) return false;


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
    if( this->exist( this->ldapConnection, fullDN.c_str() ) != true ){
        return false;
    }

// change password
    if( passwordClearText != NULL ){
        this->ldapModAppend( &mods, &modsLen, LDAP_MOD_REPLACE, "userPassword", passwordClearText );
    }

// change mail
    if( mail != NULL ){
        this->ldapModAppend( &mods, &modsLen, LDAP_MOD_REPLACE, "mail", mail );
    }

// call
    returnCode = ldap_modify_ext_s( this->ldapConnection, fullDN.c_str(), mods, NULL, NULL );
    if( returnCode != LDAP_SUCCESS ){
        char *errorMessage = ldap_err2string( returnCode );
        etDebugMessage( etID_LEVEL_ERR, errorMessage );
        this->ldapModMemFree( &mods );
        return false;
    }

    this->ldapModMemFree( &mods );
    return true;

}


bool ldapService::                  userDelete( const char* uid ){
// not connected
    if( this->ldapConnectionActive == false || this->ldapConnection == NULL ) return false;
    if( uid == NULL ) return false;

    
// create account-group
    std::string     fullDN;

// build dn
    fullDN  = "uid=";
    fullDN += uid;
    fullDN += ",";
    fullDN += this->userdn;
    fullDN += ",";
    fullDN += this->basedn;
    
    return this->removeDN( fullDN.c_str() );
}




#endif // ldapService_C
