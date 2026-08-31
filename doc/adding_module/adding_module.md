# Adding and Integrating a New Module

This guide details the technical steps to create a new module (`ModuleName`) and integrate it into the `ModuleController`.

## 1. Module Creation

Determine if the module interacts primarily with **Hardware** or **Software**. Create the directory structure in `src/Modules/<Type>/ModuleName`.

**Example (Software Module):**

* `src/Modules/Software/ModuleName/ModuleName.cpp`
* `src/Modules/Software/ModuleName/ModuleName.h`
* `src/Modules/Software/ModuleName/README.md` (Recommended)

### Using Templates

Use the existing templates in `doc/src_templates` as a base:

* Copy `doc/src_templates/ModuleTemplate.h` -> `ModuleName.h`
* Copy `doc/src_templates/ModuleTemplate.cpp` -> `ModuleName.cpp`

## 2. Implementation Details

Your class `ModuleName` inherits from `Module`. You must define a constructor. Other function overrides are optional.

**Important:** When you override a parent method, call the parent method from your implementation.

```cpp
void ModuleName::disable (const bool verbose, const bool do_restart) {
    // Custom disable routines here
    Module::disable(verbose, do_restart);
}
```

### Constructor Configuration

Define module properties in the constructor. Do not run functional logic here. Put functional logic in the `begin` routines.

```cpp
ModuleName::ModuleName(ModuleController& controller)
      : Module(controller,
               /* module_name         */ "ModuleName",
               /* module_description  */ "Brief description",
               /* nvs_key             */ "key", // ~3 chars ideally
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}
```

#### Configuration Flags

* **`requires_init_setup`**: If `true`, runs `begin_routines_init()` once on the first boot after upload.
* **`can_be_disabled`**: If `true`, the module supports enable and disable functionality.
* **`has_cli_cmds`**: If `true`, enables CLI support.
* By default, adds `status` and `reset` commands.
* If `can_be_disabled` is also `true`, adds `enable` and `disable` commands.

### Defining CLI Commands

Define custom commands in the constructor body with `commands_storage`.

**Command Structure:**

```cpp
struct Command {
    string              name;
    string              description;
    string              sample_usage;
    size_t              arg_count;
    command_function_t  function;
};
```

**Implementation Example:**

```cpp
commands_storage.push_back({
    "add",
    "Add a button mapping: <pin> \"<$cmd ...>\" [pullup|pulldown] [on_press|on_release|on_change] [debounce_ms]",
    std::string("$") + lower(module_name) + " add 9 \"$system reboot\" pullup on_press 50",
    5,
    [this](std::string_view args){ button_add_cli(args); }
});
```

## 3. Lifecycle Logic

There are four initialization phases.

1. **`begin_routines_required`**: Runs every boot.
2. **`begin_routines_init`**: Runs on first boot or after `$enable`.
3. **`begin_routines_regular`**: Runs on regular boot.
4. **`begin_routines_common`**: Runs at the end of the boot process.

![begin_flow.png](../static/media/resources/readme/begin_flow.png)

> **Note:** `begin` methods are called even if the module is disabled. This keeps pointers valid for other modules.

You can pass custom parameters to `begin()` with `ModuleNameConfig`.

## 4. Loop and Custom Functions

### Loop

Put routine execution code here. Do not use blocking functions such as `delay()`. Blocking functions affect the entire system.

### Custom Function Guidelines

If the module can be disabled, check its state at the start of each public custom function.

Other modules can call these functions while the module is disabled.

```cpp
void ModuleName::custom_function () {
    // Safety check to prevent bugs when accessed by other modules
    if (is_disabled()) return;
    
    // Custom logic here
}
```

---

## 5. Integrating the New Module

Follow these steps to register `ModuleName` with the `ModuleController`.

### Step 1: Update ModuleController.h

In `src/Modules/Module/ModuleController.h`:

1. Include the header:

```cpp
#include "../<Hardware|Software>/ModuleName/ModuleName.h"
```

2. Declare the member variable:

```cpp
ModuleName module_name;
```

### Step 2: Update ModuleController.cpp

In `src/Modules/Module/ModuleController.cpp`:

**In `ModuleController::ModuleController()`:**

1. Initialize the module in the constructor list:

```cpp
, module_name(*this)
```

2. Add the module to the modules array:

```cpp
modules.push_back(&module_name);
```

**In `ModuleController::begin()`:**

1. Define dependencies before initialization.

Example if `ModuleName` requires WiFi:

```cpp
module_name.add_requirement(wifi);
```

2. Call `begin()`:

```cpp
module_name.begin(ModuleNameConfig {});
```

**Ordering Note:** Initialize the command executor after modules that register CLI commands.

```cpp
command_executor.begin(CommandExecutorConfig {});
```

The module is now integrated.