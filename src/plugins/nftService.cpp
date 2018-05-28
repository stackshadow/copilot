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
#include "pubsub.h"
#include <string>

nftService::                        nftService(){

    this->jsonRootObject = NULL;

// json
    this->load();


}


nftService::                        ~nftService(){
    if( this->jsonRootObject != NULL ){
        json_decref( this->jsonRootObject );
    }
}




int nftService::                    onSubscriberMessage( const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, void* userdata ){

// vars
    nftService*         nftServiceInst = (nftService*)userdata;
    int                 msgCommandLen = 0;


// check
    if( command == NULL ) return psBus::END;
    msgCommandLen = strlen(command);

    std::string     answerTopic;


    if( coCore::strIsExact("chainsCountGet",command,msgCommandLen) == true ){

    // vars
        const char*     chainName = NULL;
        json_t*         jsonChainCounter = json_object();
        int             chainCounter = 0;
        char*           jsonString = NULL;

    // get the host
        if( nftServiceInst->selectHost(nodeTarget) == false ){
            return psBus::END;
        }

        nftServiceInst->iterate();
        while( nftServiceInst->nextChain(&chainName) == true ){
            chainCounter = json_array_size( nftServiceInst->jsonRulesArray );
            json_object_set_new( jsonChainCounter, chainName, json_integer(chainCounter) );
        }

    // dump
        jsonString = json_dumps( jsonChainCounter, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // send back
        psBus::inst->publish( id, nodeTarget, nodeSource, "nft", "chainsCount", jsonString );

    // clean
        free( (void*)jsonString );
        json_decref( jsonChainCounter );
        return psBus::END;
    }


    if( coCore::strIsExact("chainsList",command,msgCommandLen) == true ){


    // get the host
        if( nftServiceInst->selectHost(nodeTarget) == false ){
            return psBus::END;
        }

        char* jsonString = json_dumps( nftServiceInst->jsonChainsObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

        psBus::inst->publish( id, nodeTarget, nodeSource, "nft", "chains", jsonString );


        free( (void*)jsonString );
        return psBus::END;
    }


    if( coCore::strIsExact("chainGet",command,msgCommandLen) == true ){

    // vars
        json_t*         jsonChain = json_object();

    // get the host
        if( nftServiceInst->selectHost(nodeTarget) == false ){
            return psBus::END;
        }

    // select the chain
        if( nftServiceInst->selectChain(payload) == false ){
            psBus::inst->publish( id, nodeTarget, nodeSource, "", "msgError", "Chain dont exist !" );

            return psBus::END;
        }

    // set the object
        json_object_set( jsonChain, payload, nftServiceInst->jsonRulesArray );

    // dump
        char* jsonString = json_dumps( jsonChain, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // send back
        psBus::inst->publish( id, nodeTarget, nodeSource, "nft", "chain", jsonString );

    // clean
        free( (void*)jsonString );
        json_decref( jsonChain );
        return psBus::END;
    }


    if( coCore::strIsExact("chainSave",command,msgCommandLen) == true ){

    // vars
        json_t*         jsonChain = NULL;
        json_error_t    jsonError;
        int             msgPayloadLen = strlen( payload );
        const char*     chainName = NULL;
        void*           jsonIterator = NULL;
        json_t*         jsonRulesArray = NULL;

    // parse json
        jsonChain = json_loads( payload, msgPayloadLen, &jsonError );
        if( jsonChain == NULL || jsonError.line > 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "chainSave error: '%s'", jsonError.text );
            etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
            psBus::inst->publish( id, nodeTarget, nodeSource, "", "msgError", etDebugTempMessage );
            return psBus::END;
        }

    // get the chain name
        jsonIterator = json_object_iter( jsonChain );
        if( jsonIterator == NULL ) return psBus::END;
        chainName = json_object_iter_key( jsonIterator );
        if( chainName == NULL ) return psBus::END;

        jsonRulesArray = json_object_iter_value( jsonIterator );

    // select the requested host
        if( nftServiceInst->selectHost(nodeTarget) == false ){
            return psBus::END;
        }

    // overwrite chain
        json_object_set( nftServiceInst->jsonChainsObject, chainName, jsonRulesArray );

    // clean
        json_decref( jsonChain );

        return psBus::END;
    }


    if( coCore::strIsExact("save",command,msgCommandLen) == true ){

    // save it to the file
        nftServiceInst->save();

        psBus::inst->publish( id, nodeTarget, nodeSource, "nft", "saveok", "" );

        return psBus::END;
    }


    if( coCore::strIsExact("apply",command,msgCommandLen) == true ){
        
        std::string returnMessage = "";
        
        if( nftServiceInst->applyRules(nodeTarget,&returnMessage) == false ){
            psBus::inst->publish( id, nodeTarget, nodeSource, "nft", "msgError", returnMessage.c_str() );
            return psBus::END;
        }
        psBus::inst->publish( id, nodeTarget, nodeSource, "nft", "msgInfo", "Rules active." );


        return psBus::END;
    }


    return psBus::END;
}




void nftService::               	load(){
    this->jsonRootObject = coCore::ptr->config->section("nft");
}


bool nftService::               	save(){
    return coCore::ptr->config->save();
}




void nftService::               	iterate(){

// reset
    this->jsonHostsObject = this->jsonRootObject;

// get the first host
    this->jsonHostsIterator = json_object_iter( this->jsonHostsObject );


    this->jsonChainsObject = NULL;

}


bool nftService::               	nextHost( const char** host ){



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


bool nftService::               	nextChain( const char** chainName ){
// precheck
    if( this->jsonHostObject == NULL ) return false;

// get the "chains"-object
    if( this->jsonChainsObject == NULL ){
        this->jsonChainsObject = json_object_get( this->jsonHostObject, "chains" );
        if( this->jsonChainsObject == NULL ) {
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Chain '%s' not aviable in host.", *chainName );
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


bool nftService::               	nextRule( const char **type ){

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




bool nftService::               	selectHost( const char* hostName ){

    if( this->jsonRootObject == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "No root in json-file" );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }




// the host-object
    this->jsonHostObject = json_object_get( this->jsonRootObject, hostName );
    if( this->jsonHostObject == NULL ){
        this->jsonHostObject = json_object();

	/** @todo This is maybe useless, because we WANT to select a host
	not use only the local hostname.
	*/
	// vars
		const char* localHostName = coCore::ptr->nodeName();


    // add the host
        json_object_set_new( this->jsonRootObject, localHostName, this->jsonHostObject );
        json_object_set_new( this->jsonHostObject, "displayName", json_string(localHostName) );
    }


// the chains-object inside the host
    this->jsonChainsObject = json_object_get( this->jsonHostObject, "chains" );
    if( this->jsonChainsObject == NULL ){
        this->jsonChainsObject = json_object();
        json_object_set_new( this->jsonHostObject, "chains", this->jsonChainsObject );
    }

// default chains
    this->jsonRulesArray = json_object_get( this->jsonChainsObject, "prerouting" );
    if( this->jsonRulesArray == NULL ) json_object_set_new( this->jsonChainsObject, "prerouting", json_array() );
    this->jsonRulesArray = json_object_get( this->jsonChainsObject, "input" );
    if( this->jsonRulesArray == NULL ) json_object_set_new( this->jsonChainsObject, "input", json_array() );
    this->jsonRulesArray = json_object_get( this->jsonChainsObject, "output" );
    if( this->jsonRulesArray == NULL ) json_object_set_new( this->jsonChainsObject, "output", json_array() );
    this->jsonRulesArray = json_object_get( this->jsonChainsObject, "forward" );
    if( this->jsonRulesArray == NULL ) json_object_set_new( this->jsonChainsObject, "forward", json_array() );
    this->jsonRulesArray = json_object_get( this->jsonChainsObject, "postrouting" );
    if( this->jsonRulesArray == NULL ) json_object_set_new( this->jsonChainsObject, "postrouting", json_array() );



// reset the rest
    this->jsonChainsIterator = NULL;
    this->jsonRulesArray = NULL;
    this->jsonRulesArrayIndex = 0;
    this->jsonRulesArrayLen = 0;
    this->jsonRuleObject = NULL;

    return true;
}


bool nftService::               	selectChain( const char* chainName ){
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





bool nftService::               	createRuleCommand( std::string* nftCommand, const char* chainName, json_t* jsonRule ){

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


bool nftService::               	applyChain( const char* chainName, const char* chainType, std::string* returnMessage ){

// vars
    std::string     command;
    int             returnValue = -1;
    const char*     msgSource = NULL;
    const char*     msgTarget = NULL;

// clear return-message
    returnMessage->clear();

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
            returnMessage->assign( "Could not create chain" );
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
            returnMessage->assign( "Could not create ipv6 chain" );
            return false;
        }

    // apply rules
        while( this->nextRule(NULL) == true ){
            if( nftService::createRuleCommand( &command, chainName, this->jsonRuleObject ) == true ){
                fprintf( stdout, "nft: %s\n", command.c_str() );
                returnValue = system( command.c_str() );
                if( returnValue != 0 ){

                    command = "Could not apply " + command;
                    returnMessage->assign( command );

                    return false;
                }
            }
        }
    }

    return true;
}


bool nftService::               	applyRules( const char* hostName, std::string* returnMessage ){

// vars
    int             returnValue = -1;
    std::string     command;
    json_t*         jsonString = NULL;
    const char*     jsonStringChar = NULL;
    const char*     msgSource = NULL;
    const char*     msgTarget = NULL;

// clear return-message
    returnMessage->clear();

// select host
    if( this->selectHost( hostName ) == false ) return false;

// save the original rules
    //sudo nft list ruleset > /tmp/fw.rules
    //sudo nft -f /tmp/fw.rules


//
    returnValue = system( "sudo nft flush ruleset" );
    fprintf( stdout, "nft: sudo nft flush ruleset\n" );
    if( returnValue != 0 ){
        returnMessage->assign( "Could not flush rules" );
        return false;
    }

// create tables
    returnValue = system( "sudo nft add table ip default" );
    fprintf( stdout, "nft: sudo nft add table ip default\n" );
    if( returnValue != 0 ){
        returnMessage->assign( "Could not create ip table" );
        return false;
    }
    returnValue = system( "sudo nft add table ip6 default" );
    fprintf( stdout, "nft: sudo nft add table ip6 default\n" );
    if( returnValue != 0 ){
        returnMessage->assign( "Could not create ip6 table" );
        return false;
    }

// prerouting
    if( this->applyChain( "prerouting",     "nat",      returnMessage ) == false ) return false;
    if( this->applyChain( "input",          "filter",   returnMessage ) == false ) return false;
    if( this->applyChain( "forward",        "filter",   returnMessage ) == false ) return false;
    if( this->applyChain( "output",         "filter",   returnMessage ) == false ) return false;
    if( this->applyChain( "postrouting",    "nat",      returnMessage ) == false ) return false;



    returnMessage->assign( "Rules active." );

    return true;
}


bool nftService::               	applyRules(){
    const char* hostName = coCore::ptr->nodeName();
    std::string returnMessage = "";
    return this->applyRules( hostName, &returnMessage );
}



#endif // ldapService_C
