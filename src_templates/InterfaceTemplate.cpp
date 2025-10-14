//// src/Interfaces/InterfaceName/InterfaceName.cpp
//
//#include "InterfaceName.h"
//#include "../../../SystemController/SystemController.h"
//
//
//InterfaceName::InterfaceName(SystemController& controller)
//      : Interface(controller,
//               /* module_name         */ "",
//               /* module_description  */ "",
//               /* nvs_key             */ "",
//               /* requires_init_setup */ false,
//               /* can_be_disabled     */ false,
//               /* has_cli_cmds        */ false)
//{}
//
////  begin(cfg):
////      begin_routines_required(cfg)
////      if first_time_startup
////           begin_routines_init(cfg)
////      else
////          begin_routines_regular(cfg)
////      begin_routines_common(cfg)
//
//void InterfaceName::begin_routines_required (const InterfaceConfig& cfg) {
////    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
//    // do your custom routines here
//}
//
//void InterfaceName::begin_routines_init (const InterfaceConfig& cfg) {
////    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
//    // do your custom routines here
//}
//
//void InterfaceName::begin_routines_regular (const InterfaceConfig& cfg) {
////    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
//    // do your custom routines here
//}
//
//void InterfaceName::begin_routines_common (const InterfaceConfig& cfg) {
////    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
//    // do your custom routines here
//}
//
//void InterfaceName::loop () {
//    // do your custom routines here
//}
//
//void InterfaceName::reset (const bool verbose) {
//    Module::reset(verbose);
//    // do your custom routines here
//}
//
//bool InterfaceName::enable (const bool verbose) {
//    return Module::enable(verbose);
//    // do your custom routines here
//}
//
//bool InterfaceName::disable (const bool verbose) {
//    return Module::disable(verbose);
//    // do your custom routines here
//}
//
//std::string InterfaceName::status (const bool verbose) const {
//    return Module::status(verbose);
//    // do your custom routines here
//}
