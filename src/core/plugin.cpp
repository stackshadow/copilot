
#include <dlfcn.h>
#include "plugin.h"
#include "core/etDebug.h"


static etList* pluginList = NULL;
static int     g_argc;
static char**  g_argv;



void*   initPluginThread( void* pluginData_v ){

// vars
    pluginData_t*       pluginData = (pluginData_t*)pluginData_v;
    pluginInit_tf       pluginInit = NULL;

// init plugin
    pluginInit = (pluginInit_tf)dlsym( pluginData->dlHandle, "pluginInit" );
    if (!pluginInit) {
        fprintf(stderr, "%s\n", dlerror());
        return NULL;
    }

// call init
    optind = 1;
    pluginInit( g_argc, g_argv );

    pluginData->finishWithInit = true;
    return NULL;
}


void*   runPluginThread( void* pluginData_v ){

// vars
    pluginData_t*       pluginData = (pluginData_t*)pluginData_v;
    pluginRun_tf        pluginRun = NULL;

// init plugin
    pluginRun = (pluginRun_tf)dlsym( pluginData->dlHandle, "pluginRun" );
    if (!pluginRun) {
        fprintf(stderr, "%s\n", dlerror());
        return NULL;
    }

// wait until finish
    while( pluginData->finishWithInit == false ){
        usleep(5000);
    }

// call init
    pluginRun();

    return NULL;
}



bool    loadPlugin( const char* fileName ){


    void*               handle = NULL;
    char*               error;
    pluginData_s*       pluginData = NULL;
    pluginGetData_tf    pluginGetData = NULL;

// init list if needed
    if( pluginList == NULL ){
        etListAlloc( pluginList );
    }

// open plugin
    std::string     fillFileName = PLUGINPATH;
    fillFileName += "/";
    fillFileName += fileName;
    fillFileName += ".so";
    handle = dlopen( fillFileName.c_str(), RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }

// load pluginData
    fprintf( stdout, "Try to load pluginData..." );
    pluginGetData = (pluginGetData_tf)dlsym( handle, "pluginGetData" );
    if (!pluginGetData) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    fprintf( stdout, "ok\n" );
    pluginData = pluginGetData();
    fprintf( stdout, "Loaded Plugin '%s', Version: %i \n", pluginData->pluginName, pluginData->version );

// check version of plugin
    if( pluginData->version > pluginVersion ) {
        etDebugMessage( etID_LEVEL_ERR, "Plugin contains an newer Version" );
        dlclose( handle );
        return false;
    }

// save handle
    pluginData->dlHandle = handle;
    pluginData->finishWithInit = false;

// clear any error
    dlerror();

// add plugin to the list
    etListAppend( pluginList, (void*)pluginData );

    return true;
}


bool    initAllPlugins( int argc, char *argv[] ){

//vars
    void*               pluginListIterator = NULL;
    pluginData_s*       pluginData = NULL;

// remember argc / argv
    g_argc = argc;
    g_argv = argv;

// iterate and init
    etListIterate( pluginList, pluginListIterator );
    while( etListIterateNext( pluginListIterator, pluginData ) == etID_YES ){

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Init Plugin '%s'", pluginData->pluginName );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

    // create thread
        pthread_t   initThread;
        pthread_create( &initThread, NULL, initPluginThread, pluginData );
        pthread_detach( initThread );

    }

// return
    return true;
}


bool    runAllPlugins(){

//vars
    void*               pluginListIterator = NULL;
    pluginData_s*       pluginData = NULL;

// iterate and init
    etListIterate( pluginList, pluginListIterator );
    while( etListIterateNext( pluginListIterator, pluginData ) == etID_YES ){

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Run Plugin '%s'", pluginData->pluginName );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

    // create thread
        pthread_t   initThread;
        pthread_create( &initThread, NULL, runPluginThread, pluginData );
        pthread_detach( initThread );

    }

// return
    return true;
}



