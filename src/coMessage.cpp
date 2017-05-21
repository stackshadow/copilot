





#include "coMessage.h"


coMessage::					coMessage(){

	etStringAllocLen( this->hostName_t, 128 );
	etStringAllocLen( this->group_t, 128 );
	etStringAllocLen( this->command_t, 128 );
	etStringAllocLen( this->payload_t, 1024 );

	etStringAllocLen( this->replyCommand_t, 128 );
	etStringAllocLen( this->replyPayload_t, 1024 );

	etStringAllocLen( this->temp, 1024 );
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


const char*	coMessage::		topic(){
// vars
	const char*		tempChar = NULL;

	etStringClean( this->temp );

	etStringCharSet( this->temp, "nodes/", 6 );

	etStringCharGet( this->hostName_t, tempChar );
	etStringCharAdd( this->temp, tempChar );

	etStringCharAdd( this->temp, "/" );
	etStringCharGet( this->group_t, tempChar );
	etStringCharAdd( this->temp, tempChar );

	etStringCharAdd( this->temp, "/" );
	etStringCharGet( this->command_t, tempChar );
	etStringCharAdd( this->temp, tempChar );

	etStringCharGet( this->temp, tempChar );
	return tempChar;
}



bool coMessage:: 			replyExists(){
	if( this->replyCommand_t->lengthActual > 0 ) return true;
	return false;
}

const char* coMessage:: 	replyComandFull(){

// vars
	const char*		tempChar = NULL;

	etStringClean( this->temp );

	etStringCharSet( this->temp, "nodes/", 6 );

	etStringCharGet( this->hostName_t, tempChar );
	etStringCharAdd( this->temp, tempChar );

	etStringCharAdd( this->temp, "/" );
	etStringCharGet( this->group_t, tempChar );
	etStringCharAdd( this->temp, tempChar );

	etStringCharAdd( this->temp, "/" );
	etStringCharGet( this->replyCommand_t, tempChar );
	etStringCharAdd( this->temp, tempChar );

	etStringCharGet( this->temp, tempChar );
	return tempChar;
}



bool coMessage:: 			clear(){
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


