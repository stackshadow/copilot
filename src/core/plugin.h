
#include "core/coCore.h"
#include "core/coConfig.h"
#include "pubsub.h"


#ifdef __cplusplus
extern "C" {
#endif



typedef struct pluginData_t {

// version of pluginData
    unsigned int        version;

// common stuff
    const char*         pluginName;
    const char*         pluginDescription;
    const char*         configSectionName;

// data to handle plugin
    void*               dlHandle;
    bool                finishWithInit;

// data for plugin
    void*               userdata;

} pluginData_s;



// plugin functions
typedef pluginData_t*   (*pluginGetData_tf)();
typedef void            (*pluginInit_tf)( int argc, char *argv[] );
typedef void            (*pluginRun_tf)();



bool    loadPlugin( const char* fileName );
bool    initAllPlugins( int argc, char *argv[] );
bool    runAllPlugins();


#define pluginVersion 1
#define pluginSetVersion( pluginData ) pluginData.version = pluginVersion


#ifdef __cplusplus
}
#endif


