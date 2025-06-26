//#pragma once
//
//#include "ControllerModule.h"
//
///**
// * @class ControllerModuleTemplate
// * @brief A template for a simple module that does not need synchronization.
// *
// * This class only inherits from ControllerModule. It has a lifecycle and
// * commands but does not implement the ControllerInterface.
// */
//class ControllerModuleTemplate : public ControllerModule {
//public:
//    /**
//     * @brief Construct a new ControllerModuleTemplate object.
//     * @param controller_ref A reference to the main system controller.
//     */
//    ControllerModuleTemplate(SystemController& controller_ref);
//
//    /**
//     * @brief Virtual destructor.
//     */
//    virtual ~ControllerModuleTemplate() override;
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
//private:
//    // --- Member Variables ---
//    unsigned long loop_counter;
//
//    // --- Command and Memory Key Definitions ---
//    static const String MODULE_MEM_KEY;
//    static const CommandParser::Command COMMANDS[];
//    static const size_t NUM_COMMANDS;
//};