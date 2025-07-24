class Module {

public:
    Module(SystemController& controller_ref, enabled, can_be_disabled, nvs_key) : controller(controller_ref), enabled(enabled), can_be_disabled(can_be_disabled), nvs_key(nvs_key)  {}

    virtual ~Module()     {}

    virtual void begin              (void* context = nullptr, const String& device_name = "") = 0;
    virtual void loop               ()                                      = 0;

    virtual void enable              ()                                      = 0;
    virtual void disable              ()                                      = 0;
    virtual void reset              ()                                      = 0;

    virtual char* status             ()                                      = 0;

private:
    const char* nvs_key;

    const uint8_t             CMD_COUNT                                   = 0;
    CommandParser::Command          commands         [CMD_COUNT];
    CommandParser::CommandGroup     commands_group;

    bool enabled, can_be_disabled;



};