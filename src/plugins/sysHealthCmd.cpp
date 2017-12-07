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

#ifndef sysHealthCmd_C
#define sysHealthCmd_C

#include "sysHealthCmd.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

#include "core/etDebug.h"

sysHealthCmd::              sysHealthCmd( const char* command, int delay, int min, int max, sysHealthSetFct* updateHealthFunction ){

    this->milliseconds = delay;
    this->cmdHealth = 100;
    this->cmdValueMin = min;
    this->cmdValueMax = max;

// copy
    int commandLen = strlen(command);
    if( commandLen * sizeof(char) >= cmdSize ){
        commandLen = cmdSize / sizeof(char);
    }

// copy the command
    memset( this->cmd, 0, cmdSize );
    strncpy( this->cmd, command, commandLen );

    this->updateHealth = updateHealthFunction;
}


sysHealthCmd::              ~sysHealthCmd(){

}




bool sysHealthCmd::         execute( char* commandOutput, int* commandOutputSize ){


// vars
    int*    cmdOutSize = 0;
    char*   cmdOutChar = NULL;
    int     tmpCmdOutSize = 1024;
    char    tmpCmdOutChar[tmpCmdOutSize];

// temp ?
    if( commandOutput == NULL ){
        cmdOutSize = &tmpCmdOutSize;
        cmdOutChar = tmpCmdOutChar;
    } else {
        cmdOutSize = commandOutputSize;
        cmdOutChar = commandOutput;
    }

// clean
    memset( cmdOutChar, 0, *cmdOutSize );
    int rangeDiff = this->cmdValueMax - this->cmdValueMin;
    int healthActual;

// check
    if( rangeDiff == 0 ) rangeDiff = 1;

// open the command
    FILE* pf = popen( this->cmd, "r" );
    while( fgets(cmdOutChar, *cmdOutSize, pf) != NULL ){
        cmdOutChar[strlen(cmdOutChar)-1] = 0;
    }

// rangeDiff == 100 %
// health = ???? + min
    healthActual = atoi(cmdOutChar);

// calculate percentage
    this->cmdHealth = (healthActual-this->cmdValueMin) * 100 / rangeDiff;

// clean
    fclose(pf);

// health bigger
    if( this->cmdHealth > 100 ){
        this->cmdHealth = 100;
    }

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Command: %s Result: %s Health: %d", this->cmd, cmdOutChar, this->cmdHealth );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// update health if needed
    if( this->updateHealth != NULL ){
        this->updateHealth( this->cmdHealth );
    }

// sleep
    usleep( 1000 * this->milliseconds );


    return true;
}




#endif