//#pragma once
//
//#include "ControllerModule.h"
//#include "ControllerInterface.h"
//
///**
// * @class ControllerInterfaceTemplate
// * @brief A template for a complete module that also implements the sync interface.
// *
// * This class inherits from both ControllerModule to get its core lifecycle
// * (begin, loop, etc.) and command structure, and from ControllerInterface
// * to receive state synchronization commands (sync_color, sync_state, etc.).
// */
//class ControllerInterfaceTemplate : public ControllerModule, public ControllerInterface {
//public:
//    /**
//     * @brief Construct a new ControllerInterfaceTemplate object.
//     * @param controller_ref A reference to the main system controller.
//     */
//    ControllerInterfaceTemplate(SystemController& controller_ref);
//
//    /**
//     * @brief Virtual destructor.
//     */
//    virtual ~ControllerInterfaceTemplate() override;
//
//    // --- ControllerModule Contract Implementation ---
//
//    bool begin(void* context = nullptr, const String& device_name = "") override;
//    bool loop() override;
//    bool reset() override;
//    bool status() override;
//    bool enable() override;
//    bool disable() override;
//
//protected:
//    const CommandParser::Command* get_commands() const override;
//    size_t get_commands_num() const override;
//    const String& get_mem_key() const override;
//
//    // --- ControllerInterface Contract Implementation ---
//
//public:
//    void sync_color(std::array<uint8_t, 3> color) override;
//    void sync_brightness(uint8_t brightness) override;
//    void sync_state(bool state) override;
//    void sync_mode(uint8_t mode_id, String mode_name) override;
//    void sync_length(uint16_t length) override;
//    void sync_all(std::array<uint8_t, 3> color, uint8_t brightness, bool state, uint8_t mode_id, String mode_name, uint16_t length) override;
//
//private:
//    // --- Member Variables ---
//    std::array<uint8_t, 3> current_color;
//    uint8_t current_brightness;
//    bool is_on;
//
//    // --- Command and Memory Key Definitions ---
//    static const String MODULE_MEM_KEY;
//    static const CommandParser::Command COMMANDS[];
//    static const size_t NUM_COMMANDS;
//};