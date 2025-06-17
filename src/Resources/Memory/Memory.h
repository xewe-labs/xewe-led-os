#ifndef MEMORY_H
#define MEMORY_H

#include <Preferences.h>
#include <array>
#include "../../Debug.h"
#include "../../SystemController/ControllerModule.h"

// Forward declaration
class SystemController;

/**
 * @class Memory
 * @brief Manages persistent storage (NVS) for system settings.
 * * This module saves and retrieves system state, such as color, brightness,
 * and mode, to non-volatile storage. It uses a non-blocking timer to
 * batch writes and reduce flash wear. It implements the ControllerModule
 * interface to automatically save state changes.
 */
class Memory : public ControllerModule {
public:
    /**
     * @brief Construct a new Memory object.
     * @param controller_ref A reference to the main SystemController.
     */
    Memory(SystemController& controller_ref);

    // ~~~~~~~~~~~~~~~~~~ Overridden methods from ControllerModule ~~~~~~~~~~~~~~~~~~

    /**
     * @brief Initializes the Preferences library with a given namespace.
     * @param context A void pointer expected to be a 'const char*' namespace string.
     */
    void            begin               (void* context = nullptr)               override;

    /**
     * @brief Main loop to handle delayed commits to NVS.
     */
    void            loop                ()                                      override;

    /**
     * @brief Clears all keys in the current namespace.
     */
    void            reset               ()                                      override;

    /**
     * @brief Syncs the color to NVS. The module assumes RGB format.
     */
    void            sync_color          (std::array<uint8_t, 3> color)          override;

    /**
     * @brief Syncs the brightness to NVS.
     */
    void            sync_brightness     (uint8_t brightness)                    override;

    /**
     * @brief Syncs the power state to NVS.
     */
    void            sync_state          (bool state)                            override;

    /**
     * @brief Syncs the current mode ID and name to NVS.
     */
    void            sync_mode           (uint8_t mode_id, String mode_name)     override;

    /**
     * @brief Syncs the LED strip length to NVS.
     */
    void            sync_length         (uint16_t length)                       override;

    /**
     * @brief Performs a full sync of all properties to NVS.
     */
    void            sync_all            (std::array<uint8_t, 3> color,
                                         uint8_t brightness,
                                         bool state,
                                         uint8_t mode_id,
                                         String mode_name,
                                         uint16_t length)                       override;

    // --- Generic NVS Read/Write Methods ---
    void      write_str(const char* key, const String& value);
    String    read_str(const char* key, const String& defaultValue = "");
    void      write_uint8(const char* key, uint8_t value);
    uint8_t   read_uint8(const char* key, uint8_t defaultValue = 0);
    void      write_uint16(const char* key, uint16_t value);
    uint16_t  read_uint16(const char* key, uint16_t defaultValue = 0);
    void      write_bool(const char* key, bool value);
    bool      read_bool(const char* key, bool defaultValue = false);

    bool      is_initialized() const { return initialized; }

private:
    /**
     * @brief Schedules a delayed commit to NVS.
     */
    void schedule_commit();
    /**
     * @brief Commit to NVS.
     */
    void commit();

    Preferences preferences;
    const char* nvsNamespace = nullptr;
    bool initialized = false;
    bool dirty = false; // Flag to indicate pending changes
    unsigned long commit_time = 0; // Timestamp for the next scheduled commit

    // Delay in milliseconds before writing to flash to batch changes
    static constexpr unsigned long COMMIT_DELAY_MS = 1000;
};

#endif // MEMORY_H
