#include <wups.h>

#include "engine.h"
#include "logger.h"

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

WUPS_USE_WUT_DEVOPTAB();    // Use the wut devoptabs
WUPS_USE_STORAGE(APP_NAME); // Unique id for the storage api

/**
    Gets called when an application starts.
**/
ON_APPLICATION_START() {
    initLogging();
    engine_start();
}

/**
 * Gets called when an application actually ends
 */
ON_APPLICATION_ENDS() {
    engine_stop();
    deinitLogging();
}