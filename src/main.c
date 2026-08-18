#include <coreinit/filesystem.h>
#include <wups.h>
#include <wups/button_combo/api.h>

#include "config.h"
#include "logger.h"

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("dominii");
WUPS_PLUGIN_DESCRIPTION("Description");
WUPS_PLUGIN_VERSION("v0.1");
WUPS_PLUGIN_AUTHOR("fengb");
WUPS_PLUGIN_LICENSE("MIT");


/**
    All of this defines can be used in ANY file.
    It's possible to split it up into multiple files.

**/

WUPS_USE_WUT_DEVOPTAB();            // Use the wut devoptabs
WUPS_USE_STORAGE("dominii"); // Unique id for the storage api

static WUPSButtonCombo_ComboHandle sPressDownButtonComboExampleHandle         = {};
static WUPSButtonCombo_ComboHandle sPressDownObserverButtonComboExampleHandle = {};
static WUPSButtonCombo_ComboHandle sHoldButtonComboExampleHandle              = {};
static WUPSButtonCombo_ComboHandle sHoldObserverExButtonComboExampleHandle    = {};
WUPSButtonCombo_Buttons DEFAULT_PRESS_DOWN_BUTTON_COMBO                       = WUPS_BUTTON_COMBO_BUTTON_L | WUPS_BUTTON_COMBO_BUTTON_R;
WUPSButtonCombo_Buttons DEFAULT_PRESS_HOLD_COMBO                              = WUPS_BUTTON_COMBO_BUTTON_L | WUPS_BUTTON_COMBO_BUTTON_R | WUPS_BUTTON_COMBO_BUTTON_DOWN;

void pressDownComboCallback(const WUPSButtonCombo_ControllerTypes triggeredBy, WUPSButtonCombo_ComboHandle, void *) {
    DEBUG_FUNCTION_LINE_INFO("Button combo has been pressed down by controller %s", WUPSButtonComboAPI_GetControllerTypeStr(triggeredBy));
}

void pressDownObserverComboCallback(const WUPSButtonCombo_ControllerTypes triggeredBy, WUPSButtonCombo_ComboHandle, void *) {
    DEBUG_FUNCTION_LINE_INFO("[OBSERVER] Button combo has been pressed down by controller %s", WUPSButtonComboAPI_GetControllerTypeStr(triggeredBy));
}

void holdComboCallback(const WUPSButtonCombo_ControllerTypes triggeredBy, WUPSButtonCombo_ComboHandle, void *) {
    DEBUG_FUNCTION_LINE_INFO("Button combo has been hold by controller %s", WUPSButtonComboAPI_GetControllerTypeStr(triggeredBy));
}

void holdObserverExComboCallback(const WUPSButtonCombo_ControllerTypes triggeredBy, WUPSButtonCombo_ComboHandle, void *) {
    DEBUG_FUNCTION_LINE_INFO("[OBSERVER] Button combo has been hold by controller %s", WUPSButtonComboAPI_GetControllerTypeStr(triggeredBy));
}

/**
    Gets called ONCE when the plugin was loaded.
**/
INITIALIZE_PLUGIN() {
    // Logging only works when compiled with `make DEBUG=1`. See the README for more information.
    initLogging();
    DEBUG_FUNCTION_LINE("INITIALIZE_PLUGIN of dominii!");

    config_init("dominii");

    // To register a button combo, we can use the C++ wrapper class "WUPSButtonComboAPI::ButtonCombo".
    // The combo will be added on construction of that wrapper, and removed again in the destructor. Use `std::move` to move it around.
    // Like the C++ config api there are two versions of all function, one that throws an exception on error and one that returns a std::optional but set an additional error parameter.

    {
        WUPSButtonCombo_ComboStatus comboStatus = WUPS_BUTTON_COMBO_COMBO_STATUS_INVALID_STATUS;
        // Create a button combo which detects if a combo has been pressed down on any controller.
        // This version will check for conflicts. It's useful to check for conflicts if you want to use that button combo for a global unique thing
        // that's always possible, like taking screenshots.
        const WUPSButtonCombo_Error err = WUPSButtonComboAPI_AddButtonComboPressDown(
                "Example Plugin: Press Down test",
                DEFAULT_PRESS_DOWN_BUTTON_COMBO, // L + R
                pressDownComboCallback,
                NULL,
                &sPressDownButtonComboExampleHandle, // We will use the handle in the config menu
                &comboStatus);
        if (err == WUPS_BUTTON_COMBO_ERROR_SUCCESS) {
            // On success, we can check if the combo is actually active by checking the combo status.
            // If there is already another combo that conflicts with us, the status will be set to WUPS_BUTTON_COMBO_COMBO_STATUS_CONFLICT
            switch (comboStatus) {
                case WUPS_BUTTON_COMBO_COMBO_STATUS_VALID:
                    DEBUG_FUNCTION_LINE_INFO("Button combo is valid and active");
                    break;
                case WUPS_BUTTON_COMBO_COMBO_STATUS_CONFLICT:
                    DEBUG_FUNCTION_LINE_INFO("Conflict detected for button combo");
                    break;
                default:
                    DEBUG_FUNCTION_LINE_ERR("Invalid combo status");
                    break;
            }
        } else {
            DEBUG_FUNCTION_LINE_ERR("Failed to add press down button combo");
        }
        // To remove that button combo, we explicitly have to call "WUPSButtonComboAPI_RemoveButtonCombo", we'll do it in DEINITIALIZE_PLUGIN
    }
    {
        // --------------------------------------------------------------------------------------------------------------------------------------------------

        // But we can also create button combos without caring about conflicts.
        // E.g. when a new Aroma update is detected, the updater can be launched by holding the PLUS button. This should always be possible.
        // If we don't want to check for conflicts, we need to create a "PressDownObserver"
        WUPSButtonCombo_ComboStatus comboStatus = WUPS_BUTTON_COMBO_COMBO_STATUS_INVALID_STATUS;
        const WUPSButtonCombo_Error err         = WUPSButtonComboAPI_AddButtonComboPressDownObserver(
                        "Example Plugin: Press Down observer test",
                        DEFAULT_PRESS_DOWN_BUTTON_COMBO, // L + R Even though this is same combo as in buttonComboPressDown an observer will ignore conflicts.
                        pressDownObserverComboCallback,
                        NULL,
                        &sPressDownObserverButtonComboExampleHandle,
                        &comboStatus); // comboStatus will always be WUPS_BUTTON_COMBO_COMBO_STATUS_VALID for observers.

        if (err == WUPS_BUTTON_COMBO_ERROR_SUCCESS) {
            // To remove that button combo, we explicitly have to call "WUPSButtonComboAPI_RemoveButtonCombo", we'll do it in DEINITIALIZE_PLUGIN
        } else {
            DEBUG_FUNCTION_LINE_ERR("Failed to add press down observer button combo");
        }
    }

    {
        // --------------------------------------------------------------------------------------------------------------------------------------------------

        // In case of a conflict, the function will return SUCCESS, but the combo status will be WUPS_BUTTON_COMBO_COMBO_STATUS_CONFLICT

        // Let's create a button combo which will lead to a conflict. This time we want to check if a combo has been hold for 500ms. Conflicts are checked across
        // non-observer combo types.
        WUPSButtonCombo_ComboStatus comboStatus = WUPS_BUTTON_COMBO_COMBO_STATUS_INVALID_STATUS;
        WUPSButtonCombo_Error err               = WUPSButtonComboAPI_AddButtonComboHold(
                              "Example Plugin: Hold test",
                              DEFAULT_PRESS_HOLD_COMBO, // L+R+DPAD+DOWN. This combo includes the combo "L+R" of the buttonComboPressDown, so this will lead to a conflict.
                              500,                      // We need to hold that combo for 500ms
                              holdComboCallback,
                              NULL,
                              &sHoldButtonComboExampleHandle,
                              &comboStatus); // comboStatus will always be WUPS_BUTTON_COMBO_COMBO_STATUS_VALID for observers.

        if (err == WUPS_BUTTON_COMBO_ERROR_SUCCESS) {
            // API returned "WUPS_BUTTON_COMBO_ERROR_SUCCESS", but we have a conflict because of the existing press down combo.
            switch (comboStatus) {
                case WUPS_BUTTON_COMBO_COMBO_STATUS_VALID:
                    DEBUG_FUNCTION_LINE_INFO("Button combo is valid and active");
                    break;
                case WUPS_BUTTON_COMBO_COMBO_STATUS_CONFLICT:
                    DEBUG_FUNCTION_LINE_INFO("Conflict detected for button combo"); // <-- this is expected to happen
                    break;
                default:
                    DEBUG_FUNCTION_LINE_ERR("Invalid combo status");
                    break;
            }

            // Once combo is in the "WUPS_BUTTON_COMBO_COMBO_STATUS_CONFLICT" state it can only be valid again, if the button combo or the controllerMask changes.
            // Other combos won't ever affect this state of this combo
            // We can easily update the button combo
            err = WUPSButtonComboAPI_UpdateButtonCombo(
                    sHoldButtonComboExampleHandle,
                    WUPS_BUTTON_COMBO_BUTTON_ZR | WUPS_BUTTON_COMBO_BUTTON_R | WUPS_BUTTON_COMBO_BUTTON_DOWN,
                    &comboStatus);
            if (err == WUPS_BUTTON_COMBO_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE_INFO("Updated button combo");
                // Check the comboStatus after updating the combo
                switch (comboStatus) {
                    case WUPS_BUTTON_COMBO_COMBO_STATUS_VALID:
                        DEBUG_FUNCTION_LINE_INFO("Button combo is valid and active"); // <-- this is expected to happen
                        break;
                    case WUPS_BUTTON_COMBO_COMBO_STATUS_CONFLICT:
                        DEBUG_FUNCTION_LINE_INFO("Conflict detected for button combo");
                        break;
                    default:
                        DEBUG_FUNCTION_LINE_ERR("Invalid combo status");
                        break;
                }
            } else {
                DEBUG_FUNCTION_LINE_INFO("Failed to update button combo");
            }
            // To remove that button combo, we explicitly have to call "WUPSButtonComboAPI_RemoveButtonCombo", we'll do it in DEINITIALIZE_PLUGIN
        } else {
            DEBUG_FUNCTION_LINE_ERR("Failed to add press down observer button combo");
        }
    }
    {
        // --------------------------------------------------------------------------------------------------------------------------------------------------

        // To register a combo for just one controller, we can't use the helper function we're using above.
        // We have fill in the options instead:
        WUPSButtonCombo_ComboOptions options;
        options.metaOptions.label        = "Combo for WPAD_0";          // can be NULL
        options.callbackOptions.callback = holdObserverExComboCallback; // must not be NULL
        options.callbackOptions.context  = NULL;                        // can be NULL
        // We want to a "hold" combination where we have to hold for 100ms. Let's create an observer to not care about conflicts for this example plugin.
        options.buttonComboOptions.type               = WUPS_BUTTON_COMBO_COMBO_TYPE_HOLD_OBSERVER;
        options.buttonComboOptions.optionalHoldForXMs = 100; // <-- will be ignored if the type is WUPS_BUTTON_COMBO_COMBO_TYPE_PRESS_DOWN*

        // Defines which button the combo is using and which controllers should be checked
        options.buttonComboOptions.basicCombo.controllerMask = WUPS_BUTTON_COMBO_CONTROLLER_WPAD_0;                     // We check for WPAD_0, but we could also do something like (WUPS_BUTTON_COMBO_CONTROLLER_WPAD_0 | WUPS_BUTTON_COMBO_CONTROLLER_VPAD_0)
        options.buttonComboOptions.basicCombo.combo          = WUPS_BUTTON_COMBO_BUTTON_A | WUPS_BUTTON_COMBO_BUTTON_B; // <-- will be ignored if the type is WUPS_BUTTON_COMBO_COMBO_TYPE_PRESS_DOWN*

        WUPSButtonCombo_ComboStatus comboStatus = WUPS_BUTTON_COMBO_COMBO_STATUS_INVALID_STATUS;
        const WUPSButtonCombo_Error err         = WUPSButtonComboAPI_AddButtonCombo(&options,
                                                                                    &sHoldObserverExButtonComboExampleHandle,
                                                                                    &comboStatus); // comboStatus will always be WUPS_BUTTON_COMBO_COMBO_STATUS_VALID for observers.

        if (err == WUPS_BUTTON_COMBO_ERROR_SUCCESS) {
            // To remove that button combo, we explicitly have to call "WUPSButtonComboAPI_RemoveButtonCombo", we'll do it in DEINITIALIZE_PLUGIN
        } else {
            DEBUG_FUNCTION_LINE_ERR("Failed to add hold observer button combo for WPAD_0");
        }
    }

    deinitLogging();
}

/**
    Gets called when the plugin will be unloaded.
**/
DEINITIALIZE_PLUGIN() {
    DEBUG_FUNCTION_LINE("DEINITIALIZE_PLUGIN of dominii!");
    WUPSButtonComboAPI_RemoveButtonCombo(sPressDownButtonComboExampleHandle);
    WUPSButtonComboAPI_RemoveButtonCombo(sPressDownObserverButtonComboExampleHandle);
    WUPSButtonComboAPI_RemoveButtonCombo(sHoldButtonComboExampleHandle);
    WUPSButtonComboAPI_RemoveButtonCombo(sHoldObserverExButtonComboExampleHandle);
    sPressDownButtonComboExampleHandle.handle         = NULL;
    sPressDownObserverButtonComboExampleHandle.handle = NULL;
    sHoldButtonComboExampleHandle.handle              = NULL;
    sHoldObserverExButtonComboExampleHandle.handle    = NULL;
}

/**
    Gets called when an application starts.
**/
ON_APPLICATION_START() {
    initLogging();

    DEBUG_FUNCTION_LINE("ON_APPLICATION_START of dominii!");
}

/**
 * Gets called when an application actually ends
 */
ON_APPLICATION_ENDS() {
    deinitLogging();
}

/**
    Gets called when an application request to exit.
**/
ON_APPLICATION_REQUESTS_EXIT() {
    DEBUG_FUNCTION_LINE_INFO("ON_APPLICATION_REQUESTS_EXIT of dominii!");
}