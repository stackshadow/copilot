/*  Copyright (C) 2017 by Martin Langlotz alias stackshadow

    This file is part of doDB.

    doDB is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    doDB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with doDB.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef threadList_H
#define threadList_H

#ifdef __cplusplus
extern "C" {
#endif

#include "evillib_defines.h"
#include "memory/etMemoryBlock.h"
#include "memory/etMemory.h"
#include "core/etDebug.h"




typedef void*(*threadFunctionStart)(void *);
typedef void*(*threadFunctionStop)(void *);

typedef struct threadListItem_s {
    pthread_t               thread;
    char                    threadName[16];
    const char*             serviceName;

    threadFunctionStart     functionStart;
    threadFunctionStop      functionCancel;
    void*                   functionData;

    char                    cancelRequest;
} threadListItem_t;

typedef struct threadList_s {
    threadListItem_t**      items;
    int                     count;
} threadList_t;

// some callback functions
typedef void*(*threadIterationFunction)(threadListItem_t*,void*);



etID_STATE      etThreadListAlloc( threadList_t** threadList );


etID_STATE      etThreadListFree( threadList_t** threadList );


etID_STATE      etThreadListAdd( threadList_t* threadList, const char* name, threadFunctionStart startFunction, threadFunctionStop stopFunction, void* userdata );


etID_STATE      etThreadListCancelAll( threadList_t* threadList );


etID_STATE      etThreadListIterate( threadList_t* threadList, threadIterationFunction iteratorFunction, void* userdata );



etID_STATE      etThreadListUserdataGet( threadListItem_t* threadListItem, void** userdata );


etID_STATE      etThreadCancelRequestActive( threadListItem_t* threadListItem );





#ifdef __cplusplus
}
#endif

#endif