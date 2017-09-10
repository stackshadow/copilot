





#include "coMessage.h"


coMessage::					coMessage(){

	etStringAllocLen( this->reqID_t, 128 );

	etStringAllocLen( this->hostName_t, 128 );
	etStringAllocLen( this->group_t, 128 );
	etStringAllocLen( this->command_t, 128 );
	etStringAllocLen( this->payload_t, 1024 );

	etStringAllocLen( this->replyCommand_t, 128 );
	etStringAllocLen( this->replyPayload_t, 1024 );

	etStringAllocLen( this->temp, 1024 );
}

coMessage::					~coMessage(){

	etStringFree( this->reqID_t );

	etStringFree( this->hostName_t );
	etStringFree( this->group_t );
	etStringFree( this->command_t );
	etStringFree( this->payload_t );

	etStringFree( this->replyCommand_t );
	etStringFree( this->replyPayload_t );

	etStringFree( this->temp );

}

const char* coMessage::		setValue( etString* string, const char* newValue ){

	if( newValue == NULL ){
		const char* stringChar = NULL;
		if( etStringCharGet( string, stringChar ) != etID_YES ){
			return NULL;
		}

		return stringChar;
	}

// not null, so we would like to set the char
	if( etStringCharSet( string, newValue, -1 ) != etID_YES ){
		return NULL;
	}

	return newValue;
}


const char*	coMessage::		topic( const char* newTopic, bool isReply ){
// vars
	const char*		tempChar = NULL;

// only request the topic
	if( newTopic == NULL ){
		etStringClean( this->temp );

		etStringCharSet( this->temp, "nodes/", 6 );

		etStringCharGet( this->hostName_t, tempChar );
		etStringCharAdd( this->temp, tempChar );

		etStringCharAdd( this->temp, "/" );
		etStringCharGet( this->group_t, tempChar );
		etStringCharAdd( this->temp, tempChar );

		etStringCharAdd( this->temp, "/" );
		if( isReply == false ){
			etStringCharGet( this->command_t, tempChar );
		} else {
			etStringCharGet( this->replyCommand_t, tempChar );
		}
		etStringCharAdd( this->temp, tempChar );

		etStringCharGet( this->temp, tempChar );
		return tempChar;
	}

// new topic should be set
    char*               hostName;
    char*               group;
    char*               cmd;

// we grab the hostname out of the topic
    hostName = strtok( (char*)newTopic, "/" );
    hostName = strtok( NULL, "/" );
    group = strtok( NULL, "/" );
    cmd = strtok( NULL, "/" );

// check
    if( hostName == NULL ) return NULL;
    if( group == NULL ) return NULL;
    if( cmd == NULL ) return NULL;

// set
	if( this->hostName( hostName ) == NULL ) return NULL;
	if( this->group( group ) == NULL ) return NULL;
	if( isReply == false ){
		if( this->command( cmd ) == NULL ) return NULL;
	} else {
		if( this->replyCommand( cmd ) == NULL ) return NULL;
	}

	return newTopic;
}



bool coMessage:: 			replyExists(){
	if( this->replyCommand_t->lengthActual > 0 ) return true;
	return false;
}


const char* coMessage:: 	replyComandFull(){
	return this->topic( NULL, true );
}



bool coMessage:: 			clear(){
	etStringClean( this->reqID_t );
	etStringClean( this->hostName_t );
	etStringClean( this->group_t );
	etStringClean( this->command_t );
	etStringClean( this->payload_t );

	this->clearReply();
}


bool coMessage:: 			clearReply(){
	etStringClean( this->replyCommand_t );
	etStringClean( this->replyPayload_t );
}




/** @brief Convert coMessage to jsonObject

The calling application should create a new empty jsonObject and is responsible for destroy the jsonObject !

@param[in,out] jsonObject an empty json Object
@param[in] isReply If reply command/value should be used
@return
 - true if Conversation was successful
 - false an error occure
*/
bool coMessage::			toJson( json_t* jsonObject, bool isReply ){
	if( jsonObject == NULL ) return false;

// vars
	const char*		tempChar = NULL;

	tempChar = this->reqID();
	if( tempChar == NULL ) return false;
	json_object_set_new( jsonObject, "id", json_string(tempChar) );

	tempChar = this->hostName();
	if( tempChar == NULL ) return false;
	json_object_set_new( jsonObject, "h", json_string(tempChar) );

	tempChar = this->group();
	if( tempChar == NULL ) return false;
	json_object_set_new( jsonObject, "g", json_string(tempChar) );

	if( isReply == false){
		tempChar = this->command();
	} else {
		tempChar = this->replyCommand();
	}
	if( tempChar == NULL ) return false;
	json_object_set_new( jsonObject, "c", json_string(tempChar) );

	if( isReply == false ){
		tempChar = this->payload();
	} else {
		tempChar = this->replyPayload();
	}
	if( tempChar == NULL ) return false;
	json_object_set_new( jsonObject, "v", json_string(tempChar) );

	return true;
}


bool coMessage::			fromJson( json_t* jsonObject ){
	if( jsonObject == NULL ) return false;

// clear all
	this->clear();

// vars
	json_t*			jsonValue;

// id
	jsonValue = json_object_get( jsonObject, "id" );
	if( jsonValue == NULL ){
		this->reqID( "noid" );
	} else {
		this->reqID( json_string_value(jsonValue) );
	}

// host
	jsonValue = json_object_get( jsonObject, "h" );
	if( jsonValue == NULL ) return false;
	this->hostName( json_string_value(jsonValue) );

// group
	jsonValue = json_object_get( jsonObject, "g" );
	if( jsonValue == NULL ) return false;
	this->group( json_string_value(jsonValue) );

// command
	jsonValue = json_object_get( jsonObject, "c" );
	if( jsonValue == NULL ) return false;
	this->command( json_string_value(jsonValue) );

// value / payload
	jsonValue = json_object_get( jsonObject, "v" );
	if( jsonValue == NULL ) return false;
	this->payload( json_string_value(jsonValue) );


	return true;

error:
	this->clear();
	return false;
}

