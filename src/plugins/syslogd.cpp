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

#ifndef syslogd_C
#define syslogd_C


#include "plugins/syslogd.h"
#include "coCore.h"

syslogd* syslogd::ptr = NULL;

syslogd::               			    syslogd() : coPlugin( "syslogd", coCore::ptr->nodeName(), "syslogd" ){


    syslogd::ptr = this;

// register plugin
	coCore::ptr->plugins->append( this );
}


bool syslogd::                          onExecute(){
    this->messageThreadStart();
}


coPlugin::t_state syslogd::	            onBroadcastMessage( coMessage* message ){

// vars
    int                 returnValue = -1;
	const char*			msgTarget = message->nodeNameTarget();
    const char*			msgSource = message->nodeNameSource();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
    int                 msgCommandLen = 0;
	const char*			msgPayload = message->payload();

// check
    if( msgCommand == NULL ) return coPlugin::NO_REPLY;
    msgCommandLen = strlen(msgCommand);


    if( coCore::strIsExact("isRunning",msgCommand,msgCommandLen) == true ){

        if( this->messageThreadRunning == true ){
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->nodeName(), "", "syslogd", "state", "1" );
        } else {
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->nodeName(), "", "syslogd", "state", "0" );
        }

        return coPlugin::REPLY;
    }


    if( coCore::strIsExact("start",msgCommand,msgCommandLen) == true ){

        this->messageThreadGetPrevMsg = true;
        returnValue = this->messageThreadStart();
        if( returnValue == 1 ){
            etDebugMessage( etID_LEVEL_WARNING, "Already running...");
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->nodeName(), "", "syslogd", "msgError", "Already running..." );
        }
        return coPlugin::REPLY;
    }


    if( coCore::strIsExact("stop",msgCommand,msgCommandLen) == true ){
        returnValue = this->messageThreadStop();
        return coPlugin::REPLY;
    }


    if( coCore::strIsExact("refresh",msgCommand,msgCommandLen) == true ){
        this->messageThreadGetPrevMsg = true;
        return coPlugin::REPLY;
    }


}




void syslogd::                          filter( char* string ){

    int cmdLineStringLen = strlen( string );
    int jsonCharDumpIndex = 0;
    for( jsonCharDumpIndex = 0; jsonCharDumpIndex < cmdLineStringLen; jsonCharDumpIndex++ ){
        char testChar = string[jsonCharDumpIndex];
        if( testChar == '\\' || testChar == '\u0003' || testChar == '\u0001' || testChar == '\"' ){
            string[jsonCharDumpIndex] = ' ';
        }
        if( string[jsonCharDumpIndex] == 0 ){
            break;
        }
    }

}



int syslogd::                           messageThreadStart(){

// already running ?
    if( this->messageThreadRunning == true ){
        return 1;
    }

// start the thread which wait for clients
	pthread_t thread;
	pthread_create( &thread, NULL, syslogd::messageThread, this );
    char threadName[16] = { '\0' };
    snprintf( threadName, 16, "syslogd\0" );
    pthread_setname_np( thread, threadName );
	pthread_detach( thread );

    return 0;
}


int syslogd::                           messageThreadStop(){
    this->messageThreadStopReq = true;


}


void* syslogd::		                    messageThread( void* void_syslogd ){


// vars
    syslogd*        syslogdInstance = (syslogd*)void_syslogd;
    int             returnVal;
    sd_journal*     journal;

// open the journal
    returnVal = sd_journal_open(&journal, SD_JOURNAL_SYSTEM);

// go to the end
    sd_journal_seek_tail( journal );
    sd_journal_previous_skip( journal, 1 );

// started
    syslogdInstance->messageThreadRunning = true;
    syslogdInstance->messageThreadStopReq = false;
    coCore::ptr->plugins->messageQueue->add( syslogdInstance, coCore::ptr->nodeName(), coCore::ptr->nodeName(), "syslogd", "msgSuccess", "Running." );

// loop
    while( syslogdInstance->messageThreadStopReq == false ) {

        if( syslogdInstance->messageThreadGetPrevMsg == true ){
            sd_journal_seek_tail( journal );
            sd_journal_previous_skip( journal, 10 );
            syslogdInstance->messageThreadGetPrevMsg = false;
        }

        returnVal = sd_journal_next( journal );
        while( returnVal > 0 ){



        // time
            uint64_t        jTimestampMicrosecond;
            time_t          jTimestamp;
            struct tm*      jTimestampLocal;
            char            jTimestampString[80];
            int             jsonCharDumpIndex = 0;
            char*           journalMessage;
            size_t          journalMessageLen;
            char*           journalCmd = NULL;
            char*           journalText = NULL;


            sd_journal_get_realtime_usec( journal, &jTimestampMicrosecond );
            jTimestamp = jTimestampMicrosecond / 1000 / 1000;
            jTimestampLocal = localtime (&jTimestamp);
            strftime( jTimestampString, 80, "%Y-%m-%d %H:%M:%S", jTimestampLocal);

        // command
            returnVal = sd_journal_get_data( journal, "_CMDLINE", (const void **)&journalMessage, &journalMessageLen );
            if( returnVal < 0 ) {
                fprintf( stderr, "Failed to read message field: %s\n", strerror(returnVal) );
                //continue;
            } else {
                if( journalMessage != NULL ){
                    journalCmd = (char*)malloc( journalMessageLen * sizeof(char) );
                    memcpy( journalCmd, &journalMessage[8], (journalMessageLen-8) * sizeof(char) );
                    syslogd::filter( journalCmd );
                } else {
                    memcpy( journalCmd, "unknown\0", 8 );
                }
            }



        // message
            returnVal = sd_journal_get_data( journal, "MESSAGE", (const void **)&journalMessage, &journalMessageLen );
            if( returnVal < 0 ) {
                fprintf( stderr, "Failed to read message field: %s\n", strerror(returnVal) );
                continue;
            }
            if( journalMessage != NULL ){
                journalText = (char*)malloc( journalMessageLen * sizeof(char) );
                memcpy( journalText, &journalMessage[8], (journalMessageLen-8) * sizeof(char) );
                syslogd::filter( journalText );
            } else {
                memcpy( journalText, "unknown\0", 8 );
            }


        // build json-answer-object
            char jsonCharDump[2048];
            snprintf( jsonCharDump, 2048, "{ \"dt\": \"%s\", \"cmd\": \"%s\", \"msg\": \"%s\" }", jTimestampString, journalCmd, journalText );
            if( journalCmd != NULL ) free( journalCmd );
            if( journalText != NULL ) free( journalText );




        // add the message to list
            coCore::ptr->plugins->messageQueue->add( syslogd::ptr,
            coCore::ptr->nodeName(), "", "syslogd", "entry", jsonCharDump );

            sleep(1);


            returnVal = sd_journal_next( journal );
        }


        returnVal = sd_journal_wait( journal, (uint64_t)1000000 );
        if ( returnVal < 0 ) {
            //log_error_errno(r, "Couldn't wait for journal event: %m");
            goto finish;
        }
    }

finish:
    fflush(stdout);
    syslogdInstance->messageThreadStopReq = false;
    syslogdInstance->messageThreadRunning = false;
    coCore::ptr->plugins->messageQueue->add( syslogdInstance, coCore::ptr->nodeName(), coCore::ptr->nodeName(), "syslogd", "msgSuccess", "Stopped." );

}



#endif