// src/Modules/ModuleName/ModuleName.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Debug.h"


struct ParserConfig : public ModuleConfig {
    const CommandsGroup*        groups                      = nullptr;
    std::size_t                 group_count                 = 0;
};


class CommandParser : public Module {
public:
    explicit                    CommandParser              (SystemController& controller);

//    virtual bool                begin_routines_required     (const ModuleConfig& cfg)       override;
//    virtual bool                begin_routines_init         (const ModuleConfig& cfg)       override;
//    virtual bool                begin_routines_regular      (const ModuleConfig& cfg)       override;
//    virtual bool                begin_routines_common       (const ModuleConfig& cfg)       override;
//
//    virtual void                loop                        ()                              override;
//
//    virtual void                reset                       (const bool verbose=false)      override;
//
//    virtual bool                enable                      (const bool verbose=false)      override;
//    virtual bool                disable                     (const bool verbose=false)      override;
//
//    virtual std::string         status                      (const bool verbose=false)      const override;
//    virtual bool                is_enabled                  (const bool verbose=false)      const override;
//    virtual bool                is_disabled                 (const bool verbose=false)      const override;
//    virtual bool                init_setup_complete         (const bool verbose=false)      const override;

    // other methods

private:

};
