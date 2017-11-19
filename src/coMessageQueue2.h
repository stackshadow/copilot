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

#ifndef coMessageQueue2_H
#define coMessageQueue2_H


#include <unistd.h>
#include <sys/utsname.h>
#include <string>
#include <pthread.h>
#include "semaphore.h"

#include "evillib_depends.h"
#include "memory/etList.h"
#include "string/etString.h"
#include "string/etStringChar.h"

#include "jansson.h"

#include "lockPthread.h"
#include "coMessage.h"
#include "coPlugin.h"


class coMessageQueue2 {

	private:
        lockID              messageFiFoLock;
        etList*             list;

	public:
		coMessageQueue2();
		~coMessageQueue2();

    // message fifo
        bool                add(    coPlugin*   sourcePlugin,
                                    const char* nodeNameSource,
                                    const char* nodeNameTarget,
                                    const char* group,
                                    const char* command,
                                    const char* payload );
        bool                release();
        bool                get( coMessage** p_message );


};






#endif