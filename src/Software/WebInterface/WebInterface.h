#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <WebServer.h>
#include <WebSocketsServer.h>
#include <array>
#include "../../Debug.h"
#include "../../SystemController/ControllerModule.h"

// Forward declaration
class SystemController;

/**
 * @class WebInterface
 * @brief Manages the web-based user interface and WebSocket communication.
 * * This module serves a web page for real-time control and uses WebSockets
 * to keep the UI in sync with the device's state. It implements the
 * ControllerModule interface to receive state updates from the SystemController.
 */
class WebInterface : public ControllerModule {
public:
    /**
     * @brief Construct a new WebInterface object.
     * @param controller_ref A reference to the main SystemController.
     */
    WebInterface(SystemController& controller_ref);

    // ~~~~~~~~~~~~~~~~~~ Overridden methods from ControllerModule ~~~~~~~~~~~~~~~~~~

    /**
     * @brief Initializes the WebServer routes and WebSocket server.
     * @param context A void pointer expected to be a WebServer instance.
     */
    void            begin               (void* context = nullptr)               override;

    /**
     * @brief Main loop for the WebSocket server. Must be called repeatedly.
     */
    void            loop                ()                                      override;

    /**
     * @brief Resets the WebSocket connections.
     */
    void            reset               ()                                      override;

    /**
     * @brief Syncs the RGB color to all connected web clients.
     */
    void            sync_color            (std::array<uint8_t, 3> color)            override;

    /**
     * @brief Syncs the brightness to all connected web clients.
     */
    void            sync_brightness     (uint8_t brightness)                    override;

    /**
     * @brief Syncs the power state (on/off) to all connected web clients.
     */
    void            sync_state          (bool state)                            override;

    /**
     * @brief Syncs the current mode to all connected web clients.
     */
    void            sync_mode           (uint8_t mode_id, String mode_name)     override;

    /**
     * @brief Syncs the strip length. (Not used by the web interface).
     */
    void            sync_length         (uint16_t length)                       override;

    /**
     * @brief Performs a full sync of all properties to all connected web clients.
     */
    void            sync_all            (std::array<uint8_t, 3> color,
                                         uint8_t brightness,
                                         bool state,
                                         uint8_t mode_id,
                                         String mode_name,
                                         uint16_t length)                       override;

private:
    WebServer* webServer = nullptr;      // Pointer to the main WebServer instance
    WebSocketsServer  webSocket{81};            // WebSocket server runs on port 81

    // --- WebSocket Event Handler ---
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

    // --- HTTP Route Handlers ---
    void serveMainPage();
    void handleSetRequest();
    void handleGetStateRequest();
    void handleSetStateShortcut();

    // --- Helper for broadcasting messages ---
    void broadcast(const char* payload, size_t length);
};

#endif // WEB_INTERFACE_H
