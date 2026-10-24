# Module contract

The full contract is `doc/CONTRACT.md` in `xewe-os-modules`. `doc/WRITING_A_MODULE.md` in this repo covers writing a module step by step, so only the rules that break silently are here.

- `id` is at most 15 characters, is both the CLI group and the NVS namespace, and never changes. A change orphans stored settings.
- The class derives from `xewe::Module`, lives in the global namespace and includes `<XeWeCore.h>`.
- The constructor's Os parameter is named `host`, because a parameter named `os` hides the member.
- Lambdas capture `[this]` only, and no code writes `cli(` (Arduino core macro, see CORE.md).
- Three tests are required: `test_compiles`, `test_status` and one behaviour test. `xewe modules validate` must pass.

## Why the declare line is explicit (D13)

`declare=Time time_module(os, wifi);` in `module.properties` is copied verbatim into the generated `src/Modules.h`. Generating the declaration from constructor signatures would need a C++ parser in the tools and would hide the one line that shows how a module is wired. An explicit line lets a module pick its variable name and dependency arguments.

The validator replaces human review. It enforces a unique slug, id, folder, class and variable, that the type equals the folder, that the first argument is `os`, and that every other argument is a declared dependency. Declare modules in dependency order.

## Project-local modules (D28)

A module in this repo's `src/<Name>/` follows the same contract and has its values in `src/<Name>/Config.h`, every macro prefixed `XEWE_<ID>_` with the id in upper case. The four product modules here are such modules. The two teaching samples, `SimpleModule` and `FullModule`, live in the xewe-os template and are not copied into this project.
