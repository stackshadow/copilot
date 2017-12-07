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

#ifndef sysHealthCmd_H
#define sysHealthCmd_H


#define cmdSize 2048

#include "libio.h"

typedef void sysHealthSetFct( int newHealth );

class sysHealthCmd
{
    private:
        sysHealthSetFct*    updateHealth = NULL;
        char                cmd[cmdSize];
        int                 cmdHealth;
        int                 cmdValueMin;
        int                 cmdValueMax;
    public:
        int                 milliseconds;

    public:
        sysHealthCmd( const char* command, int delay = 5000, int min = 0, int max = 100, sysHealthSetFct* updateHealthFunction = NULL );
        ~sysHealthCmd();

    public:
        bool    execute( char* commandOutput = NULL, int* commandOutputSize = NULL );

};

#endif