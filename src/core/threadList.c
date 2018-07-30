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

#ifndef threadList_C
#define threadList_C

#ifdef __cplusplus
extern "C" {
#endif

#include "threadList.h"

#define _GNU_SOURCE
#include <pthread.h>
extern int pthread_setname_np (pthread_t __target_thread, const char *__name) __THROW __nonnull ((2));


etID_STATE      etThreadListAlloc( threadList_t** threadList ){

// vars
    threadList_t*	newThreadList = NULL;

// allocate the list itselfe
    etMemoryAlloc( newThreadList, sizeof(threadList_t) );

// setup
    newThreadList->count = 0;
    newThreadList->items = NULL;

    *threadList = newThreadList;
    return etID_YES;
}


etID_STATE      etThreadListFree( threadList_t** p_threadList ){
    etDebugCheckNull( p_threadList );

// vars
    int						index = 0;
    threadList_t*			threadList = *p_threadList;
    threadListItem_t*		threadListItem;



    for( index = 0; index < threadList->count; index++ ){

    // get element
        threadListItem = threadList->items[index];
        if( threadListItem == NULL ) continue;

    // call
        if( threadListItem->functionCancel != NULL ){
            threadListItem->functionCancel( threadListItem->functionData );
        }

    // release memory
        etMemoryRelease( threadListItem );
        threadList->items[index] = NULL;
    }

// relase the list
    etMemoryRelease( threadList->items );
    __etMemoryRelease( (void**)threadList );

    return etID_YES;
}


etID_STATE      etThreadListAdd( threadList_t* threadList, const char* name, threadFunctionStart startFunction, threadFunctionStop stopFunction, void* userdata ){

// vars
    threadListItem_t*	newThreadListItem;
    threadListItem_t**	newThreadListItemArray;
    int					newThreadListItemsCount;

// list-item: alloc and set
    etMemoryAlloc( newThreadListItem, sizeof(threadListItem_t) );
    newThreadListItem->serviceName = NULL;
    newThreadListItem->functionStart = startFunction;
    newThreadListItem->functionCancel = stopFunction;
    newThreadListItem->functionData = userdata;
    newThreadListItem->cancelRequest = 0;

// list-array: alloc and copy
    newThreadListItemsCount = threadList->count + 1;
    etMemoryAlloc( newThreadListItemArray, sizeof(threadListItem_t*) * newThreadListItemsCount );
    memcpy( newThreadListItemArray, threadList->items, sizeof(threadListItem_t*) * threadList->count );

// list-array: set new list-item
    newThreadListItemArray[threadList->count] = newThreadListItem;

// list-array: replace
    if( threadList->items != NULL )	etMemoryRelease( threadList->items );
    threadList->items = newThreadListItemArray;
    threadList->count = newThreadListItemsCount;


// start the thread
    pthread_create( &newThreadListItem->thread, NULL, startFunction, newThreadListItem );
    snprintf( newThreadListItem->threadName, 16, "%s\0", name );
    pthread_setname_np( newThreadListItem->thread, newThreadListItem->threadName );
    pthread_detach( newThreadListItem->thread );

    return etID_YES;
}


etID_STATE      etThreadListCancelAll( threadList_t* threadList ){

// vars
    int                     index = 0;
    threadListItem_t*       threadListItem;


    for( index = 0; index < threadList->count; index++ ){

    // get element
        threadListItem = threadList->items[index];
        if( threadListItem == NULL ) continue;

    // call
        if( threadListItem->functionCancel != NULL ){
            threadListItem->functionCancel( threadListItem->functionData );
        }

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Thread [%s] Request cancel", threadListItem->threadName );
        etDebugMessage( etID_LEVEL_DETAIL_THREAD, etDebugTempMessage );

    // cancel thread
        //pthread_cancel( threadListItem->thread );
        threadListItem->cancelRequest = 1;

    // wait
        while( threadListItem->cancelRequest == 1 ){
            usleep( 5000 );
        }

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Thread [%s] Cancelled", threadListItem->threadName );
        etDebugMessage( etID_LEVEL_DETAIL_THREAD, etDebugTempMessage );

    // release memory
        //etMemoryRelease( threadListItem );
        threadList->items[index] = NULL;
    }

// release item-array
    etMemoryRelease( threadList->items );
    threadList->items = NULL;
    threadList->count = 0;

}


etID_STATE      etThreadListIterate( threadList_t* threadList, threadIterationFunction iteratorFunction, void* userdata ){

// vars
    int                     index = 0;
    threadListItem_t*       threadListItem;


    for( index = 0; index < threadList->count; index++ ){
    // get element
        threadListItem = threadList->items[index];
        if( threadListItem == NULL ) continue;

        iteratorFunction( threadListItem, userdata );
    }

    return etID_YES;
}



etID_STATE      etThreadListUserdataGet( threadListItem_t* threadListItem, void** userdata ){
    if( threadListItem == NULL ) return etID_FALSE;

    *userdata = threadListItem->functionData;
    return etID_YES;
}


etID_STATE      etThreadServiceNameSet( threadListItem_t* threadListItem, const char* serviceName ){

// already set
    if( threadListItem->serviceName != NULL ){
        etMemoryRelease( threadListItem->serviceName );
        threadListItem->serviceName = NULL;
    }

// remember the full name
    size_t nameSize = strlen(serviceName) * sizeof(char);
    etMemoryAlloc( threadListItem->serviceName, nameSize + sizeof(char) );
    etMemoryClean( threadListItem->serviceName );
    snprintf( threadListItem->serviceName, nameSize, "%s", serviceName );

// return
    return etID_YES;
}


etID_STATE      etThreadServiceNameGet( threadListItem_t* threadListItem, const char** serviceName ){

// get
    *serviceName = threadListItem->serviceName;

    if( threadListItem->serviceName != NULL ){
        return etID_YES;
    }

    return etID_NO;
}


etID_STATE      etThreadCancelRequestActive( threadListItem_t* threadListItem ){
    if( threadListItem->cancelRequest == 1 ){
        threadListItem->cancelRequest = 0;
        etMemoryRelease( threadListItem );
        return etID_YES;
    }

    return etID_NO;
}





#ifdef __cplusplus
}
#endif





#endif