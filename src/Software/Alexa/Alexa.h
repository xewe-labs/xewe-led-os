#ifndef ALEXA_H
#define ALEXA_H

#include <Espalexa.h>
#include <WebServer.h>
#include "../../Debug.h"
#include "../../SystemController/ControllerModule.h"
#include <array>

// Forward declaration to prevent circular dependencies
class SystemController;

/**
 * @class Alexa
 * @brief Manages integration with Amazon Alexa using the Espalexa library.
 */
class Alexa : public ControllerModule {
public:
    /**
     * @brief Construct a new Alexa object.
     * @param controller_ref A reference to the main SystemController.
     */
    Alexa(SystemController& controller_ref);

    // ~~~~~~~~~~~~~~~~~~ Overridden methods from ControllerModule ~~~~~~~~~~~~~~~~~~

    /**
     * @brief Initializes the Espalexa service and creates the smart device.
     * @param context A void pointer expected to be a WebServer instance.
     */
    void            begin               (void* context = nullptr)           override;

    /**
     * @brief Main loop for the Espalexa service. Must be called repeatedly.
     */
    void            loop                ()                                  override;

    /**
     * @brief Resets the Alexa module state.
     */
    void            reset               ()                                  override;

    /**
     * @brief Syncs the RGB color from the system to the Alexa device.
     * @param rgb An array containing the R, G, and B values.
     */
    void            sync_color          (std::array<uint8_t, 3> color)      override;

    /**
     * @brief Syncs the brightness from the system to the Alexa device.
     * @param brightness The brightness value (0-255).
     */
    void            sync_brightness     (uint8_t brightness)                override;

    /**
     * @brief Syncs the power state (on/off) from the system to the Alexa device.
     * @param state The power state (true for on, false for off).
     */
    void            sync_state          (bool state)                        override;

    /**
     * @brief Syncs the current mode from the system. (Not used by Alexa).
     * @param mode_id The ID of the current mode.
     * @param mode_name The name of the current mode.
     */
    void            sync_mode           (uint8_t mode_id, String mode_name) override;

    /**
     * @brief Syncs the strip length from the system. (Not used by Alexa).
     * @param length The new length of the strip.
     */
    void            sync_length         (uint16_t length)                   override;

    /**
     * @brief Performs a full sync of all properties from the system to Alexa.
     */
    void            sync_all            (std::array<uint8_t, 3> color,
                                         uint8_t brightness,
                                         bool state,
                                         uint8_t mode_id,
                                         String mode_name,
                                         uint16_t length)                   override;

private:
    Espalexa        espalexa;
    EspalexaDevice* device = nullptr; // Pointer to the created Espalexa device

    /**
     * @brief Callback function triggered when a change is initiated from Alexa.
     * @param device_ptr Pointer to the device that was changed.
     */
    void            change_event        (EspalexaDevice* device_ptr);
};

#endif // ALEXA_H
