
#include "evillib_depends.h"
#include "memory/etList.h"
#include "jansson.h"


// callbackfunction
typedef int (*psSubscriberMessage)( const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, void* userdata );
typedef int (*psSubscriberJsonMessage)( json_t* jsonObject, void* userdata );




class psBus {

private:
    void**          psSubscriberArray;
    size_t          psSubscriberArrayLen;
    int             psSubscriberArrayLock;
    const char*     localNodeName;

public:
    static psBus*	inst;
    typedef enum e_state {
        NEXT_SUBSCRIBER = 1,     // Message finished, next subscriber can have it
        END = 2                  // Message finished, nobody else need that
    } t_state;


public:
    psBus();
    ~psBus();

    static void     toJson( json_t** jsonObject, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload );
    static bool     fromJson( json_t* jsonObject, const char** id, const char** nodeSource, const char** nodeTarget, const char** group, const char** command, const char** payload );

    void            subscribe( const char* nodeTarget, const char* group, void* userdata, psSubscriberMessage callback, psSubscriberJsonMessage jsonCallback );
    void            publish( const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload );
    void            publish( json_t* jsonObject );
    void            publishOrSubscribe( json_t* jsonObject, void* userdata, psSubscriberMessage callback, psSubscriberJsonMessage jsonCallback );

};