

#include "pubsub.h"

#include "core/etDebug.h"
#include "lockPthread.h"


class psSubscriber {
	
	
    private:
        char*                       nodeSource;
        char*                       nodeTarget;
        char*                       group;
        void*                       userdata;
        psSubscriberMessage         onMessage;
        psSubscriberJsonMessage     onJsonMessage;
        
    public:


                            psSubscriber( const char* nodeTarget, const char* group, void* userdata, psSubscriberMessage onMessage, psSubscriberJsonMessage jsonCallback ){


    size_t	memsize = 0;

// nodetarget
    if( nodeTarget != NULL ){
        memsize = strlen(nodeTarget) * sizeof(char);
        this->nodeTarget = (char*)malloc( memsize + sizeof(char) );
        memset( this->nodeTarget, 0, memsize + sizeof(char) );
        memcpy( this->nodeTarget, nodeTarget, memsize );
    } else {
        this->nodeTarget = NULL;
    }

// group
    if( group != NULL ){
        memsize = strlen(group) * sizeof(char);
        this->group = (char*)malloc( memsize );
        memset( this->group, 0, memsize + sizeof(char) );
        memcpy( this->group, group, memsize );
    } else {
        this->group = NULL;
    }

// callbackfunction
    this->userdata = userdata;
    this->onMessage = onMessage;
    this->onJsonMessage = jsonCallback;

// debug
    if( this->onMessage != NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%p] subscribe plain on [%s]/[%s]", this->onMessage, nodeTarget, group );
        etDebugMessage( etID_LEVEL_DETAIL_BUS, etDebugTempMessage );
    }
    if( this->onJsonMessage != NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%p] subscribe json on [%s]/[%s]", this->onJsonMessage, nodeTarget, group );
        etDebugMessage( etID_LEVEL_DETAIL_BUS, etDebugTempMessage );
    }

}
                    
                        
                            ~psSubscriber();


        bool                publishValidate( const char* nodeTarget, const char* group ){
            
            etDebugMessage( etID_LEVEL_DETAIL_BUS, "validate..." );
            
        // target node
            if( nodeTarget == NULL ){
                etDebugMessage( etID_LEVEL_ERR, "ERROR, you can not publish to [null] target...." );
                return false;
            }
            if( this->nodeTarget == NULL ){
                etDebugMessage( etID_LEVEL_DETAIL_BUS, "subscriber has no target, thats okay.." );
            }
        // compare
            if( nodeTarget != NULL && this->nodeTarget != NULL ){
                if( strncmp(nodeTarget,this->nodeTarget,strlen(this->nodeTarget)) != 0 ){
                    
                    //snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] !-> [%s]/[%s]/[%s] -> [%p] nodeTarget dont match", nodeSource, nodeTarget, group, command, this->onMessage);
                    //etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
                    
                    return false;
                }
            }

        // group
            if( group == NULL ){
                etDebugMessage( etID_LEVEL_ERR, "ERROR, you can not publish to [null] group...." );
                return false;
            }
            if( this->group == NULL ){
                etDebugMessage( etID_LEVEL_DETAIL_BUS, "subscriber has no group, thats okay.." );
            }
        // compare
            if( group != NULL && this->group != NULL ) {
                if( strncmp(group,this->group,strlen(this->group)) != 0 ){
                    
                    //snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] !-> [%s]/[%s]/[%s] -> [%p] group dont match", nodeSource, nodeTarget, group, command, this->onMessage );
                    //etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
                    
                    return false;
                }
            }
            
            etDebugMessage( etID_LEVEL_DETAIL_BUS, "success.." );
            return true;
        }


        void                publish( const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, bool broadcast = false ){
            
        // validate ( only if not broadcast ... )
            if( broadcast == false ){
                if( this->publishValidate(nodeTarget,group) == false ) return;
            } else {
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] -> [broadcast]/[%s]/[%s] -> [%p](onMessage)", nodeSource, group, command, this->onMessage );
                etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
            }


        // call callback
            if( this->onMessage != NULL ){
            // debug
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] -> [%s]/[%s]/[%s] -> [%p](onMessage)", nodeSource, nodeTarget, group, command, this->onMessage );
                etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
                
                this->onMessage( id, this->nodeSource, this->nodeTarget, this->group, command, payload, this->userdata );
            }

        //  call json-callback
            if( this->onJsonMessage != NULL ){
            // debug
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] -> [%s]/[%s]/[%s] -> [%p](onJsonMessage)", nodeSource, nodeTarget, group, command, this->onJsonMessage );
                etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
                
                json_t* newJsonObject = NULL;
                psBus::toJson( &newJsonObject, id, nodeSource, nodeTarget, group, command, payload );
                this->onJsonMessage( newJsonObject, this->userdata );
                
                json_decref( newJsonObject );
            }


    return;
    }


        void                publish( json_t* jsonObject, bool broadcast = false ){
            

        // vars
            json_t*         jsonObjectValue = NULL;
            const char*     msgID = NULL;
            const char*     msgNodeTarget = NULL;
            const char*     msgNodeSource = NULL;
            const char*     msgGroup = NULL;
            const char*     msgCommand = NULL;
            const char*     msgValue = NULL;
            
            
        // target and group
            psBus::fromJson( jsonObject, NULL, NULL, &msgNodeTarget, &msgGroup, NULL, NULL );

        
        // validate ( only if not broadcast ... )
            if( broadcast == false ){
                if( this->publishValidate(msgNodeTarget,msgGroup) == false ) return;
            }
            
        // call callback
            if( this->onMessage != NULL ){
                
            // get the rest
                psBus::fromJson( jsonObject, &msgID, &msgNodeSource, NULL, NULL, &msgCommand, &msgValue );
                        

            // debug
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] -> [%s]/[%s]/[%s] -> [%p](onMessage)", msgNodeSource, nodeTarget, msgGroup, msgCommand, this->onMessage );
                etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
                
                this->onMessage( msgID, msgNodeSource, msgNodeTarget, msgGroup, msgCommand, msgValue, this->userdata );
            }
            
        //  call json-callback
            if( this->onJsonMessage != NULL ){
                
            // extract values from json
                jsonObjectValue = json_object_get( jsonObject, "c" );
                if( jsonObjectValue != NULL ) msgCommand = json_string_value( jsonObjectValue );
                
            // debug
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] -> [%s]/[%s]/[%s] -> [%p](onJsonMessage)", msgNodeSource, nodeTarget, msgGroup, msgCommand, this->onJsonMessage );
                etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

                this->onJsonMessage( jsonObject, this->userdata );
            }

        }


};






psBus* psBus::              inst = NULL;

psBus::                     psBus(){

// init
	this->psSubscriberArray = NULL;
	this->psSubscriberArrayLen = 0;
	this->psSubscriberArrayLock = 0;

	this->inst = this;
}




void psBus::                toJson( json_t** jsonObject, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload ){

    json_t* newJsonObject = json_object();
    json_object_set_new( newJsonObject, "id", 		json_string(id) );
    json_object_set_new( newJsonObject, "s", 		json_string(nodeSource) );
    json_object_set_new( newJsonObject, "t", 		json_string(nodeTarget) );
    json_object_set_new( newJsonObject, "g", 		json_string(group) );
    json_object_set_new( newJsonObject, "c", 		json_string(command) );
    if( payload != NULL ){
        json_object_set_new( newJsonObject, "v",	json_string(payload) );
    }

    *jsonObject = newJsonObject;
}


bool psBus::                fromJson( json_t* jsonObject, const char** id, const char** nodeSource, const char** nodeTarget, const char** group, const char** command, const char** payload ){

// vars
    json_t*     jsonObjectValue = NULL;

// id ( optional )
    if( id != NULL ){
        jsonObjectValue = json_object_get( jsonObject, "id" );
        if( jsonObjectValue == NULL ){
            *id = NULL;
            //return false;
        } else {
            *id = json_string_value( jsonObjectValue );
        }
    }

// nodeSource ( needed )
    if( nodeSource != NULL ){
        jsonObjectValue = json_object_get( jsonObject, "s" );
        if( jsonObjectValue == NULL ){
            *nodeSource = NULL;
            etDebugMessage( etID_LEVEL_ERR, "'s' is missing in Message.");
            //return false;
        } else {
            *nodeSource = json_string_value( jsonObjectValue );
        }
    }

// nodeTarget ( needed )
    if( nodeTarget != NULL ){
        jsonObjectValue = json_object_get( jsonObject, "t" );
        if( jsonObjectValue == NULL ){
            *nodeTarget = NULL;
            etDebugMessage( etID_LEVEL_ERR, "'t' is missing in Message.");
            //return false;
        } else {
            *nodeTarget = json_string_value( jsonObjectValue );
        }
    }

// group ( needed )
    if( group != NULL ){
        jsonObjectValue = json_object_get( jsonObject, "g" );
        if( jsonObjectValue == NULL ){
            *group = NULL;
            etDebugMessage( etID_LEVEL_ERR, "'g' is missing in Message.");
            //return false;
        } else {
            *group = json_string_value( jsonObjectValue );
        }
    }

// command ( needed )
    if( command != NULL ){
        jsonObjectValue = json_object_get( jsonObject, "c" );
        if( jsonObjectValue == NULL ){
            *command = NULL;
            etDebugMessage( etID_LEVEL_ERR, "'c' is missing in Message.");
            //return false;
        } else {
            *command = json_string_value( jsonObjectValue );
        }
        
    }
	
// payload ( optional )
    if( payload != NULL ){
        jsonObjectValue = json_object_get( jsonObject, "v" );
        if( jsonObjectValue == NULL ){
            *payload = NULL;
            //return false;
        } else {
            *payload = json_string_value( jsonObjectValue );
        }
    }
    
    return true;
}




void psBus::				subscribe( const char* nodeTarget, const char* group, void* userdata, psSubscriberMessage callback, psSubscriberJsonMessage jsonCallback ){
	lockPthread(this->psSubscriberArrayLock);

// allocate a new array
	size_t psSubscriberNewArraySize = (this->psSubscriberArrayLen + 1) * sizeof(void*);
	void** psSubscriberNewArray = (void**)malloc( psSubscriberNewArraySize );
	memset( psSubscriberNewArray, 0, psSubscriberNewArraySize );
	
// copy
	memcpy( psSubscriberNewArray, this->psSubscriberArray, this->psSubscriberArrayLen * sizeof(void*) );
	this->psSubscriberArrayLen++;
	
// replace
	free( this->psSubscriberArray );
	this->psSubscriberArray = psSubscriberNewArray;
	
// create / save
	psSubscriber* subscriber = new psSubscriber( nodeTarget, group, userdata, callback, jsonCallback );
	this->psSubscriberArray[this->psSubscriberArrayLen - 1] = subscriber;

	unlockPthread(this->psSubscriberArrayLock);
}


void psBus::				publish( const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload ){
	lockPthread(this->psSubscriberArrayLock);

// vars
    psSubscriber*	subscriber = NULL;
    size_t			index = 0;
	
	
	for( index = 0; index < this->psSubscriberArrayLen; index++ ){
		subscriber = (psSubscriber*)this->psSubscriberArray[index];
		subscriber->publish( id, nodeSource, nodeTarget, group, command, payload );
	}


	unlockPthread(this->psSubscriberArrayLock);
}


void psBus::				publish( json_t* jsonObject ){
	lockPthread(this->psSubscriberArrayLock);

// vars
    psSubscriber*	subscriber = NULL;
    size_t			index = 0;
	
	
	for( index = 0; index < this->psSubscriberArrayLen; index++ ){
		subscriber = (psSubscriber*)this->psSubscriberArray[index];
		subscriber->publish( jsonObject );
	}


	unlockPthread(this->psSubscriberArrayLock);
}

// this function checks for subscibe message inside the payload
// this is for plugins which subscribe for external stuff, like ssl/websockets
void psBus::                publishOrSubscribe( json_t* jsonObject, void* userdata, psSubscriberMessage callback, psSubscriberJsonMessage jsonCallback ){


    const char* msgGroup;
    const char* msgCommand;

// get command
    psBus::fromJson( jsonObject, NULL, NULL, NULL, &msgGroup, &msgCommand, NULL );
    if( msgGroup == NULL || msgCommand == NULL ) return;

// check if we get an subscribe-message
    if( strncmp(msgGroup,"bus",2) == 0 && strncmp(msgCommand,"subscribe",9) == 0 ){

    // get subscribe-topic and group
        json_error_t	jsonError;	
        const char*		jsonSubscribePayload;
        json_t* 		jsonSubscribe;
        json_t* 		jsonSubscribeValue;
        const char* 	subscribeTarget = NULL;
        const char* 	subscribeGroup = NULL;
        
    // parse subscribe-request
        psBus::fromJson( jsonObject, NULL, NULL, NULL, NULL, NULL, &jsonSubscribePayload );
        jsonSubscribe = json_loads( jsonSubscribePayload, JSON_PRESERVE_ORDER, &jsonError );
        if( jsonSubscribe == NULL || jsonError.line >= 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "json error: '%s'", jsonError.text );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return;
        }

    // get target
        jsonSubscribeValue = json_object_get( jsonSubscribe, "t" );
        if( jsonSubscribeValue != NULL ){
            subscribeTarget = json_string_value( jsonSubscribeValue );
        }

    // get group
        jsonSubscribeValue = json_object_get( jsonSubscribe, "g" );
        if( jsonSubscribeValue != NULL ){
            subscribeGroup = json_string_value( jsonSubscribeValue );
        }

    // subscribe
        this->subscribe( subscribeTarget, subscribeGroup, userdata, callback, jsonCallback );
        
    // cleanup
        json_decref(jsonSubscribe);
        
    }

// publish it
    this->publish( jsonObject );
}


