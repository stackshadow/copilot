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

#ifndef syslogd_H
#define syslogd_H


//#include "coPlugin.h"
#include "coCoreConfig.h"



// journal
#include <systemd/sd-journal.h>



class syslogd : public coPlugin {

    public:
        bool                    messageThreadRunning = false;
        bool                    messageThreadStopReq = false;
        bool                    messageThreadGetPrevMsg = false;

// public functions
	public:
                                syslogd();
                                ~syslogd();
        static syslogd*         ptr;

// helper
    public:
        static void             filter( char* string );

    public:
// API
		coPlugin::t_state	    onBroadcastMessage( coMessage* message );
		bool				    onExecute();

    public:
        int                     messageThreadStart();
        int                     messageThreadStop();
		static void*		    messageThread( void* void_session );



};




#endif