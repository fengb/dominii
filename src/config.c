#include <malloc.h>
#include <stdio.h>
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>

#include "config.h"
#include "logger.h"

#define LOG_FS_OPEN_CONFIG_ID "logFSOpen"

static struct config s_config = {};

void logFSOpenChanged(ConfigItemBoolean *item, bool newValue) {
    DEBUG_FUNCTION_LINE_INFO("New value in logFSOpenChanged: %d", newValue);
    s_config.logFSOpen = newValue;
    // If the value has changed, we store it in the storage.
    WUPSStorageAPI_StoreBool(NULL, LOG_FS_OPEN_CONFIG_ID, s_config.logFSOpen);
}

WUPSConfigAPICallbackStatus
ConfigMenuOpenedCallback(WUPSConfigCategoryHandle root) {
    {
        // Let's create a new category called "Settings"
        WUPSConfigCategoryHandle settingsCategory;
        WUPSConfigAPICreateCategoryOptionsV1 settingsCategoryOptions = {
            .name = "Settings"};
        if (WUPSConfigAPI_Category_Create(settingsCategoryOptions,
                                          &settingsCategory) !=
            WUPSCONFIG_API_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to create settings category");
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        }

        // Add a new item to this settings category
        if (WUPSConfigItemBoolean_AddToCategory(
                settingsCategory, LOG_FS_OPEN_CONFIG_ID, "Log FSOpen calls",
                true, s_config.logFSOpen,
                &logFSOpenChanged) != WUPSCONFIG_API_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to add item to category");
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        }

        // Add the category to the root.
        if (WUPSConfigAPI_Category_AddCategory(root, settingsCategory) !=
            WUPSCONFIG_API_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to add category to root item");
        }
    }
    {
        // We can also have categories inside categories!
        WUPSConfigCategoryHandle categoryLevel1;
        WUPSConfigAPICreateCategoryOptionsV1 catLev1Options = {
            .name = "Category with subcategory"};
        if (WUPSConfigAPI_Category_Create(catLev1Options, &categoryLevel1) !=
            WUPSCONFIG_API_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to create categoryLevel1");
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        }
        WUPSConfigCategoryHandle categoryLevel2;
        WUPSConfigAPICreateCategoryOptionsV1 catLev2Options = {
            .name = "Category inside category"};
        if (WUPSConfigAPI_Category_Create(catLev2Options, &categoryLevel2) !=
            WUPSCONFIG_API_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to create categoryLevel1");
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        }
        if (WUPSConfigItemBoolean_AddToCategory(
                categoryLevel2, "stubInsideCategory",
                "This is stub item inside a nested category", false, false,
                NULL) != WUPSCONFIG_API_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to add stub item to root category");
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        }

        // add categoryLevel2 to categoryLevel1
        if (WUPSConfigAPI_Category_AddCategory(categoryLevel1,
                                               categoryLevel2) !=
            WUPSCONFIG_API_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to add category to root item");
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        }

        // add categoryLevel2 to categoryLevel1
        if (WUPSConfigAPI_Category_AddCategory(root, categoryLevel1) !=
            WUPSCONFIG_API_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to add category to root item");
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        }
    }
    {
        // We can also directly add items to the root category
        if (WUPSConfigItemStub_AddToCategory(
                root, "This is stub item without category") !=
            WUPSCONFIG_API_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to add stub item to root category");
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        }
        ConfigItemMultipleValuesPair values[10];
        int numOfElements = sizeof(values) / sizeof(values[0]);
        for (int i = 0; i < numOfElements; i++) {
#define STR_SIZE 10
            char *str = (char *)malloc(STR_SIZE);
            if (!str) {
                OSFatal("Failed to allocate memory");
            }
            snprintf(str, STR_SIZE, "%d", i);
            values[i].value = i;
            values[i].valueName = str;
        }
        WUPSConfigAPIStatus multValuesRes =
            WUPSConfigItemMultipleValues_AddToCategory(
                root, "multival", "Multiple values", 0, 0, values,
                numOfElements, NULL);
        for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
            free((void *)values[i].valueName);
        }
        if (multValuesRes != WUPSCONFIG_API_RESULT_SUCCESS) {
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        }
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() { WUPSStorageAPI_SaveStorage(false); }

void config_init() {
    WUPSConfigAPIOptionsV1 configOptions = {.name = APP_NAME};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback,
                           ConfigMenuClosedCallback) !=
        WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }

    WUPSStorageError storageRes;
    // Try to get value from storage
    if ((storageRes = WUPSStorageAPI_GetBool(NULL, LOG_FS_OPEN_CONFIG_ID,
                                             &s_config.logFSOpen)) ==
        WUPS_STORAGE_ERROR_NOT_FOUND) {
        // Add the value to the storage if it's missing.
        if (WUPSStorageAPI_StoreBool(NULL, LOG_FS_OPEN_CONFIG_ID,
                                     s_config.logFSOpen) !=
            WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to store bool");
        }
    } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get bool %s (%d)",
                                WUPSConfigAPI_GetStatusStr(storageRes),
                                storageRes);
    } else {
        DEBUG_FUNCTION_LINE_ERR(
            "Successfully read the value from storage: %d %s (%d)",
            s_config.logFSOpen, WUPSConfigAPI_GetStatusStr(storageRes),
            storageRes);
    }
    WUPSStorageAPI_SaveStorage(false);
}