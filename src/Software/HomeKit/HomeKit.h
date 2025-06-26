#ifndef HOMEKIT_H
#define HOMEKIT_H

#include <array>
#include "HomeSpan.h"
#include "../../Debug.h"
#include "../../SystemController/ControllerModule.h"

// Forward declaration for the SystemController
class SystemController;

// Forward declaration for the internal service struct
struct NeoPixel_RGB;

/**
 * @class HomeKit
 * @brief Manages integration with Apple HomeKit using the HomeSpan library.
 * * This class creates a virtual LightBulb accessory that can be controlled
 * by Apple Home devices. It overrides the ControllerModule interface to sync
 * the accessory's state with the main system state.
 */
class HomeKit : public ControllerModule {
public:
    /**
     * @brief Construct a new HomeKit object.
     * @param controller_ref A reference to the main SystemController.
     */
    HomeKit(SystemController& controller_ref);

    // ~~~~~~~~~~~~~~~~~~ Overridden methods from ControllerModule ~~~~~~~~~~~~~~~~~~

    /**
     * @brief Initializes the HomeSpan service.
     * @param context This parameter is ignored by the HomeKit module.
     */
    void            begin               (void* context = nullptr, const String& device_name = "") override;

    /**
     * @brief Main loop for the HomeSpan service. Must be called repeatedly.
     */
    void            loop                ()                                      override;

    /**
     * @brief Resets the HomeKit accessory and WiFi credentials.
     */
    void            reset               ()                                      override;
    void            status               ()                                      override;

    /**
     * @brief Syncs the RGB color from the system to the HomeKit accessory.
     * @param rgb An array containing the R, G, and B values.
     */
    void            sync_color            (std::array<uint8_t, 3> rgb)            override;

    /**
     * @brief Syncs the brightness from the system to the HomeKit accessory.
     * @param brightness The brightness value (0-255).
     */
    void            sync_brightness     (uint8_t brightness)                    override;

    /**
     * @brief Syncs the power state (on/off) from the system to the HomeKit accessory.
     * @param state The power state (true for on, false for off).
     */
    void            sync_state          (bool state)                            override;

    /**
     * @brief Syncs the current mode from the system. (Not used by HomeKit).
     */
    void            sync_mode           (uint8_t mode_id, String mode_name)     override;

    /**
     * @brief Syncs the strip length from the system. (Not used by HomeKit).
     */
    void            sync_length         (uint16_t length)                       override;

    /**
     * @brief Performs a full sync of all properties from the system to HomeKit.
     */
    void            sync_all            (std::array<uint8_t, 3> color,
                                         uint8_t brightness,
                                         bool state,
                                         uint8_t mode_id,
                                         String mode_name,
                                         uint16_t length)                       override;


private:
    NeoPixel_RGB* device = nullptr; // Pointer to the HomeKit LightBulb service
};

#endif // HOMEKIT_H
