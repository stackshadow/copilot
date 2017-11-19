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

#ifndef lockPthread_H
#define lockPthread_H

#include <sys/syscall.h>
#include <sys/types.h>

#define lockID pid_t

#define lockMyPthread() \
	pid_t myThreadTIDLock = syscall(SYS_gettid); \
	while( this->threadLock != myThreadTIDLock && this->threadLock != 0 ){ \
		usleep( 10000 ); \
	} \
	this->threadLock = myThreadTIDLock; \

#define unlockMyPthread() \
	pid_t myThreadTIDUnLock = syscall(SYS_gettid); \
	while( this->threadLock != myThreadTIDUnLock && this->threadLock != 0 ){ \
		usleep( 10000 ); \
	} \
	this->threadLock = 0; \



#define lockPthread( threadLock ) \
	pid_t myThreadTIDLock = syscall(SYS_gettid); \
	while( threadLock != myThreadTIDLock && threadLock != 0 ){ \
		usleep( 10000 ); \
	} \
	threadLock = myThreadTIDLock; \

#define unlockPthread( threadLock ) \
	pid_t myThreadTIDUnLock = syscall(SYS_gettid); \
	while( threadLock != myThreadTIDUnLock && threadLock != 0 ){ \
		usleep( 10000 ); \
	} \
	threadLock = 0; \


#endif