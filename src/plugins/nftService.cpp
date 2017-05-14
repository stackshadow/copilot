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

#ifndef nftService_C
#define nftService_C

#include "coCore.h"
#include "plugins/nftService.h"


nftService::                    nftService() : coPlugin( "nft" ) {

    this->jsonRootObject = NULL;

// json
    this->load();

// test
    this->iterate();
    const char* hostname;
    const char* chainName;
    while( this->nextHost( &hostname ) == true ){

        while( this->nextChain( &chainName ) == true ){

        }


    }



// register this plugin
    coCore::ptr->registerPlugin( this, coCore::ptr->hostInfo.nodename, "nft" );

}


nftService::                    ~nftService(){
    if( this->jsonRootObject != NULL ){
        json_decref( this->jsonRootObject );
    }
}




void nftService::               load(){

    json_error_t jsonError;
    this->jsonRootObject = json_load_file( configFile("nftrules.json"), JSON_PRESERVE_ORDER, &jsonError );
    if( jsonError.position == 0 || jsonError.line >= 0 ){

    // there is an error, we create an empty element
        this->jsonRootObject = json_object();
    }

// validate file

// the host-object
    this->jsonHostObject = json_object_get( this->jsonRootObject, coCore::ptr->hostInfo.nodename );
    if( this->jsonHostObject == NULL ){
        this->jsonHostObject = json_object();
    // add the host
        json_object_set_new( this->jsonRootObject, coCore::ptr->hostInfo.nodename, this->jsonHostObject );
        json_object_set_new( this->jsonHostObject, "displayName", json_string(coCore::ptr->hostInfo.nodename) );
    }


// the chains-object inside the host
    this->jsonChainsObject = json_object_get( this->jsonHostObject, "chains" );
    if( this->jsonChainsObject == NULL ){
        this->jsonChainsObject = json_object();
        json_object_set_new( this->jsonHostObject, "chains", this->jsonChainsObject );

    // add default chains
        json_object_set_new( this->jsonChainsObject, "prerouting", json_array() );
        json_object_set_new( this->jsonChainsObject, "input", json_array() );
        json_object_set_new( this->jsonChainsObject, "output", json_array() );
        json_object_set_new( this->jsonChainsObject, "forward", json_array() );
        json_object_set_new( this->jsonChainsObject, "postrouting", json_array() );
    }


}


bool nftService::               save(){
    int state = json_dump_file( this->jsonRootObject,
                    configFile("nftrules.json"), JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    if( state == 0 ){
        return true;
    }

    return false;
}




void nftService::               iterate(){

// reset
    this->jsonHostsObject = this->jsonRootObject;

// get the first host
    this->jsonHostsIterator = json_object_iter( this->jsonHostsObject );


    this->jsonChainsObject = NULL;

}


bool nftService::               nextHost( const char** host ){



// no more hosts host, we finished
    if( this->jsonHostsIterator == NULL ){
        return false;
    }

// remember the host object
    this->jsonHostObject = json_object_iter_value( this->jsonHostsIterator );

// reset the rest
    this->jsonChainsObject = NULL;
    this->jsonChainsIterator = NULL;
    this->jsonRulesArray = NULL;
    this->jsonRulesArrayIndex = 0;
    this->jsonRulesArrayLen = 0;
    this->jsonRuleObject = NULL;

// return the hostname
    *host = json_object_iter_key( this->jsonHostsIterator );

// next
    this->jsonHostsIterator = json_object_iter_next( this->jsonHostsObject, this->jsonHostsIterator );


    return true;
}


bool nftService::               nextChain( const char** chainName ){
// precheck
    if( this->jsonHostObject == NULL ) return false;

// get the "chains"-object
    if( this->jsonChainsObject == NULL ){
        this->jsonChainsObject = json_object_get( this->jsonHostObject, "chains" );
        if( this->jsonChainsObject == NULL ) {
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Chain '%s' not aviable in host.", chainName );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }

    // get the iter and the first chain
        this->jsonChainsIterator = json_object_iter( this->jsonChainsObject );

    }

// the end of the chains
    if( this->jsonChainsIterator == NULL ) return false;

// remember the chain object
    this->jsonRulesArray = json_object_iter_value( this->jsonChainsIterator );
    this->jsonRulesArrayIndex = 0;
    this->jsonRulesArrayLen = json_array_size( this->jsonRulesArray );
    this->jsonRuleObject = NULL;


// return the hostname
    const char* tmpChainName = json_object_iter_key( this->jsonChainsIterator );
    if( chainName != NULL ){
        *chainName = tmpChainName;
    }

// next
    this->jsonChainsIterator = json_object_iter_next( this->jsonChainsObject, this->jsonChainsIterator );


    return true;
}


bool nftService::               nextRule( const char **type ){

// end of the list
    if( this->jsonRulesArrayIndex >= this->jsonRulesArrayLen ){
        this->jsonRulesArrayLen = json_array_size( this->jsonRulesArray );
        this->jsonRulesArrayIndex = 0;
        return false;
    }

// get the rule
    this->jsonRuleObject = json_array_get( this->jsonRulesArray, this->jsonRulesArrayIndex );
    if( this->jsonRuleObject == NULL ) return false;

// get the type
    json_t*     jsonRuleType = json_object_get( this->jsonRuleObject, "rule" );
    if( jsonRuleType == NULL ) return false;
    if( type != NULL ){
        *type = json_string_value( jsonRuleType );
    }

// iterate
    this->jsonRulesArrayIndex++;

    return true;
}




bool nftService::               selectHost( const char* hostName ){

    if( this->jsonHostsObject == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "No hosts in json-file" );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

    this->jsonHostObject = json_object_get( this->jsonHostsObject, hostName );
    if( this->jsonHostObject == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Host '%s' not found in json-file.", hostName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

// reset the rest
    this->jsonChainsObject = NULL;
    this->jsonChainsIterator = NULL;
    this->jsonRulesArray = NULL;
    this->jsonRulesArrayIndex = 0;
    this->jsonRulesArrayLen = 0;
    this->jsonRuleObject = NULL;

    return true;
}


bool nftService::               selectChain( const char* chainName ){
// precheck
    if( this->jsonHostObject == NULL ) return false;

// get the "chains"-object
    if( this->jsonChainsObject == NULL ){
        this->jsonChainsObject = json_object_get( this->jsonHostObject, "chains" );
        if( this->jsonChainsObject == NULL ) {
            return false;
        }
    }

// get the selected chain-array
    this->jsonRulesArray = json_object_get( this->jsonChainsObject, chainName );
    if( this->jsonRulesArray == NULL ) return false;

// reset the rest
    this->jsonRulesArrayIndex = 0;
    this->jsonRulesArrayLen = json_array_size(this->jsonRulesArray);


    return true;
}





bool nftService::               createRuleCommand( std::string* nftCommand, const char* chainName, json_t* jsonRule ){

    // vars
    json_t*         jsonString = NULL;
    const char*     jsonStringChar = NULL;

// rule active ?
    jsonString = json_object_get( jsonRule, "active" );
    if( jsonString == NULL ) return false;
    jsonStringChar = json_string_value( jsonString );
    if( strncmp(jsonStringChar,"1",1) != 0 ){
        return false;
    }

// create
    *nftCommand = "sudo nft add rule ";

// add family
    jsonString = json_object_get( jsonRule, "family" );
    if( jsonString == NULL ) return false;
    jsonStringChar = json_string_value( jsonString );
    if( strncmp(jsonStringChar,"ip4",3) == 0 ){
        *nftCommand += "ip ";
    } else {
        *nftCommand += "ip6 ";
    }

    *nftCommand += "default ";

// add chain
    *nftCommand += chainName;
    *nftCommand += " ";

// add the rule
    jsonString = json_object_get( jsonRule, "rule" );
    if( jsonString == NULL ) return false;
    jsonStringChar = json_string_value( jsonString );

    *nftCommand += jsonStringChar;

    return true;
}


bool nftService::               applyChain( const char* chainName, const char* chainType, json_t* jsonAnswerObject ){

// vars
    std::string     command;
    int             returnValue = -1;


    if( this->selectChain(chainName) == true ){

    // add ip ( v4 ) chain
        command  = "sudo nft add chain ip default ";
        command += chainName;
        command += " { type ";
        command += chainType;
        command += " hook ";
        command += chainName;
        command += " priority 100 \\; }";
        returnValue = system( command.c_str() );
        fprintf( stdout, "nft: %s\n", command.c_str() );
        if( returnValue != 0 ){
            json_object_set_new( jsonAnswerObject, "topic", json_string("msgError") );
            json_object_set_new( jsonAnswerObject, "payload", json_string("Could not create chain") );
            return false;
        }

    // add ip ( v6 ) chain
        command  = "sudo nft add chain ip6 default ";
        command += chainName;
        command += " { type ";
        command += chainType;
        command += " hook ";
        command += chainName;
        command += " priority 100 \\; }";
        returnValue = system( command.c_str() );
        fprintf( stdout, "nft: %s\n", command.c_str() );
        if( returnValue != 0 ){
            json_object_set_new( jsonAnswerObject, "topic", json_string("msgError") );
            json_object_set_new( jsonAnswerObject, "payload", json_string("Could not create chain") );
            return false;
        }

    // apply rules
        while( this->nextRule(NULL) == true ){
            if( nftService::createRuleCommand( &command, chainName, this->jsonRuleObject ) == true ){
                fprintf( stdout, "nft: %s\n", command.c_str() );
                returnValue = system( command.c_str() );
                if( returnValue != 0 ){
                    json_object_set_new( jsonAnswerObject, "topic", json_string("msgError") );
                    json_object_set_new( jsonAnswerObject, "payload", json_string("Could not apply rule") );
                    return false;
                }
            }
        }
    }

    return true;
}


bool nftService::               applyRules( const char* hostName, json_t* jsonAnswerObject ){

// vars
    int             returnValue = -1;
    std::string     command;
    json_t*         jsonString = NULL;
    const char*     jsonStringChar = NULL;

// select host
    if( this->selectHost( hostName ) == false ) return false;

// save the original rules
    //sudo nft list ruleset > /tmp/fw.rules
    //sudo nft -f /tmp/fw.rules


//
    returnValue = system( "sudo nft flush ruleset" );
    fprintf( stdout, "nft: sudo nft flush ruleset\n" );
    if( returnValue != 0 ){
        json_object_set_new( jsonAnswerObject, "topic", json_string("msgError") );
        json_object_set_new( jsonAnswerObject, "payload", json_string("Could not flush rules") );
        return false;
    }

// create tables
    returnValue = system( "sudo nft add table ip default" );
    fprintf( stdout, "nft: sudo nft add table ip default\n" );
    if( returnValue != 0 ){
        json_object_set_new( jsonAnswerObject, "topic", json_string("msgError") );
        json_object_set_new( jsonAnswerObject, "payload", json_string("Could not create ip table") );
        return false;
    }
    returnValue = system( "sudo nft add table ip6 default" );
    fprintf( stdout, "nft: sudo nft add table ip6 default\n" );
    if( returnValue != 0 ){
        json_object_set_new( jsonAnswerObject, "topic", json_string("msgError") );
        json_object_set_new( jsonAnswerObject, "payload", json_string("Could not create ip6 table") );
        return false;
    }

// prerouting
    if( this->applyChain( "prerouting",     "nat",      jsonAnswerObject ) == false ) return false;
    if( this->applyChain( "input",          "filter",   jsonAnswerObject ) == false ) return false;
    if( this->applyChain( "forward",        "filter",   jsonAnswerObject ) == false ) return false;
    if( this->applyChain( "output",         "filter",   jsonAnswerObject ) == false ) return false;
    if( this->applyChain( "postrouting",    "nat",      jsonAnswerObject ) == false ) return false;


    json_object_set_new( jsonAnswerObject, "topic", json_string("msgInfo") );
    json_object_set_new( jsonAnswerObject, "payload", json_string("Rules active.") );
    return true;
}




bool nftService::               onBroadcastMessage(     const char*     msgHostName,
                                                        const char*     msgGroup,
                                                        const char*     msgCommand,
                                                        const char*     msgPayload,
                                                        json_t*         jsonAnswerObject ){

    std::string     answerTopic;



    if( strncmp( (char*)msgCommand, "hostsList", 9 ) == 0 ){

        json_t* jsonHosts = json_object();

    // iterate over hosts
        void* hostsIterator = json_object_iter( this->jsonRootObject );
        while( hostsIterator != NULL ){
            json_t*         jsonHost = json_object_iter_value(hostsIterator);
            const char*     hostName = json_object_iter_key(hostsIterator);

        // the display name
            json_t* jsonHostDisplayName = json_object_get( jsonHost, "displayName" );
            const char* hostDisplayName = json_string_value(jsonHostDisplayName);

        // set the hostname
            json_object_set_new( jsonHosts, hostName, json_string(hostDisplayName) );

        // next
            hostsIterator = json_object_iter_next( this->jsonRootObject, hostsIterator );
        }


        json_object_set_new( jsonAnswerObject, "topic", json_string("hosts") );
        json_object_set_new( jsonAnswerObject, "payload", jsonHosts );

        return true;
    }


    if( strncmp( (char*)msgCommand, "chainsList", 10 ) == 0 ){


    // get the host
        if( this->selectHost(msgHostName) == false ){
            return false;
        }
    // get the chains
        if( this->nextChain(NULL) == false ) return false;


        const char* jsonString = json_dumps( this->jsonChainsObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

        json_object_set_new( jsonAnswerObject, "topic", json_string("chains") );
        json_object_set_new( jsonAnswerObject, "payload", json_string(jsonString) );

        free( (void*)jsonString );
        return true;
    }


    if( strncmp( (char*)msgCommand, "save", 4 ) == 0 ){

        json_error_t    jsonError;
        int             msgPayloadLen = 0;
        json_t*         jsonChains = NULL;

    // get the host
        if( this->selectHost(msgHostName) == false ) return false;


    // because jsonPayload is a string, we need to reparse it
        if( msgPayload == NULL ) return false;
        msgPayloadLen = strlen(msgPayload);
        jsonChains = json_loads( msgPayload, msgPayloadLen, &jsonError );
        if( jsonError.column >= 0 ) return false;


    // overwrite with new chains
        json_object_set_new( this->jsonHostObject, "chains", jsonChains );

    // save it to the file
        this->save();


        json_object_set_new( jsonAnswerObject, "topic", json_string("saveok") );
        json_object_set_new( jsonAnswerObject, "payload", json_string("") );

        return true;
    }


    if( strncmp( (char*)msgCommand, "apply", 4 ) == 0 ){
        this->applyRules(msgHostName, jsonAnswerObject);
        return true;
    }


    return false;
}






#endif // ldapService_C
