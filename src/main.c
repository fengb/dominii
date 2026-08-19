#include <coreinit/thread.h>
#include <wups.h>

#include "config.h"
#include "engine.h"
#include "logger.h"

static OSThread s_mdns_thread;
static uint8_t s_mdns_thread_stack[65536]; // 64KB stack (adjust as needed)

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME(APP_NAME);
WUPS_PLUGIN_DESCRIPTION("Description");
WUPS_PLUGIN_VERSION("v0.1");
WUPS_PLUGIN_AUTHOR("fengb");
WUPS_PLUGIN_LICENSE("MIT");


/**
    All of this defines can be used in ANY file.
    It's possible to split it up into multiple files.

**/

WUPS_USE_WUT_DEVOPTAB();            // Use the wut devoptabs
WUPS_USE_STORAGE(APP_NAME); // Unique id for the storage api

/**
    Gets called ONCE when the plugin was loaded.
**/
INITIALIZE_PLUGIN() {
    initLogging();
    DEBUG_FUNCTION_LINE("INITIALIZE_PLUGIN");
    config_init(); 
}

/**
    Gets called when an application starts.
**/
ON_APPLICATION_START() {
    initLogging();
    bool success = OSCreateThread(
        &s_mdns_thread,                  // Thread object
        engine_start,                    // Entry function
        0,                               // argc
        NULL,                            // argv
        s_mdns_thread_stack + sizeof(s_mdns_thread_stack), // Stack top
        sizeof(s_mdns_thread_stack),     // Stack size
        16,                              // Priority (lower number = higher priority, 16 is safe)
        OS_THREAD_ATTRIB_DETACHED        // Attributes
    );

    if (success) {
        OSResumeThread(&s_mdns_thread);
    }
}

/**
 * Gets called when an application actually ends
 */
ON_APPLICATION_ENDS() {
    engine_stop();
    deinitLogging();
}