# Nvs

The `Nvs` module owns all persistence to the ESP32 NVS flash partition. It offers
three layers, smallest to largest:

1. **Scalar records** — `write<T>` / `read<T>` for a single typed value per key.
2. **Blob records** — `write_blob` / `read_blob` for a raw `std::vector<uint8_t>`.
3. **Typed objects** — `save<T>` / `load<T>` persist a whole C++ object (any
   `FlexData<T>`) as one blob, and serve it as JSON on demand via the object's own
   `FlexData` methods.

`remove` deletes one record; `reset_ns` wipes a namespace; `factory_reset` wipes
the entire partition.

## Typed objects (FlexData)

Any struct that derives from `FlexData<Derived>` gets object⇄JSON and object⇄blob
conversion for free — including nested structs and vectors of structs — so a
module can keep one plain C++ object in RAM and let `Nvs` handle both flash
storage and the API view.

### Data model

The C++ object in RAM is the single source of truth. JSON is produced on demand
for the API; the blob is produced on demand for storage. Neither is kept in sync
continuously — they are derived views.

```
                    obj.as_json_str() │ obj.update(json)
        ┌──────────────┐  (on demand)  ┌────────────────────┐ nvs.save() ┌───────────────┐
        │              │ ─────JSON────► │                    │ ──blob───► │               │
        │  user / API  │                │   object in RAM    │           │  NVS record   │
        │              │ ◄────JSON───── │ (source of truth)  │ ◄─blob──── │  (flash)      │
        └──────────────┘                └────────────────────┘ nvs.load() └───────────────┘
                    obj.update(json)  │ obj.as_json_str()
```

* **JSON** (text) is the wire format for the API. Generated only when asked
  (`as_json_str`) and parsed back with a partial merge (`update` — only keys
  present in the JSON are overwritten).
* **Blob** (binary) is the storage format: little-endian, length-prefixed, with a
  one-byte version at the top level. Much smaller than JSON and cheap to
  read/write.
* **NVS** stores exactly one blob per record (`ns` + `key`).

### Components

| File           | Role                                                                                          |
|----------------|-----------------------------------------------------------------------------------------------|
| `FlexData.h`   | Pure, controller-free CRTP base. The blob codec + ArduinoJson converters live here.           |
| `Nvs.h`        | The `Module`. Public API: scalar `write`/`read`, `write_blob`/`read_blob`, typed `save`/`load`, `remove`, `reset_ns`. |
| `Nvs.tpp`      | Template bodies — `write`/`read`, and `save`/`load` (bridge object⇄blob to `write_blob`/`read_blob`). |
| `Nvs.cpp`      | Non-template bodies — blob primitives, namespace/partition management.                         |

`FlexData.h` has no dependency on the rest of `Nvs`, so data-defining modules can
include it just to declare their structs; it only pulls in ArduinoJson in TUs that
actually use `save`/`load`.

### Authoring a data struct

Derive from `FlexData<Derived>`, give it real typed members, and register them in
one `static constexpr fields()` tuple. Transient members simply stay out of
`fields()` — they are never serialized or persisted.

```cpp
#include "../Core/Nvs/FlexData.h"

struct LedCfg : FlexData<LedCfg> {
    uint8_t     brightness = 128;
    std::string name       = "strip";

    static constexpr auto fields() {
        return std::make_tuple(
            fld("brightness", &LedCfg::brightness),
            fld("name",       &LedCfg::name));
    }
};
```

That is all the struct needs. It now supports the full API below.

### The three flows

```
        ┌──────────────────────────── object in RAM (LedCfg) ────────────────────────────┐
        │                                                                                 │
  JSON  │  cfg.as_json_str()          ──►  {"brightness":128,"name":"strip"}              │
        │  cfg.update(json)           ◄──  partial merge (only present keys overwritten)  │
        │                                                                                 │
  BLOB  │  nvs.save(ns,key,cfg)       ──►  [ver][u8][len|bytes...]  ──►  NVS record       │
        │  nvs.load(ns,key,cfg)       ◄──  NVS record  ──►  [ver][u8][len|bytes...]       │
        │                                                                                 │
  FIELD │  cfg.set_field("name","x")  ──►  one field by name                              │
        │  cfg.get_field("name")      ◄──  one field as JSON text                         │
        └─────────────────────────────────────────────────────────────────────────────────┘
```

#### JSON (API in / out)

```cpp
std::string json = cfg.as_json_str();          // object -> JSON text
cfg.update(R"({"brightness":255})");            // JSON -> object (partial merge)
```

#### Blob + NVS (persistence)

```cpp
nvs.save("led", "cfg", cfg);      // object -> blob -> NVS record
LedCfg restored;
if (nvs.load("led", "cfg", restored)) { /* ... */ }  // NVS -> blob -> object
nvs.remove("led", "cfg");         // delete the record
```

`load` returns `false` (and leaves the target untouched) on a missing key or a
corrupt / version-mismatched blob.

#### One field by name

```cpp
cfg.set_field("name", "kitchen");          // returns false if no such field
std::string v = cfg.get_field("name");     // "\"kitchen\""  (JSON-encoded)
```

### Nested structs

A field may itself be a `FlexData` struct, or a `std::vector` of them. The codec
and the JSON converters recurse automatically, so a parent record persists and
serializes its whole tree in one blob.

```cpp
struct ChildCfg : FlexData<ChildCfg> {
    uint8_t     pin     = 0;
    std::string command = "";
    static constexpr auto fields() {
        return std::make_tuple(fld("pin", &ChildCfg::pin),
                               fld("command", &ChildCfg::command));
    }
};

struct ParentCfg : FlexData<ParentCfg> {
    uint32_t              revision = 0;
    std::vector<ChildCfg> children;          // vector of nested records
    static constexpr auto fields() {
        return std::make_tuple(fld("revision", &ParentCfg::revision),
                               fld("children", &ParentCfg::children));
    }
};

nvs.save("app", "parent", parent);           // whole tree -> one blob
```

### Blob format

```
 top level ── to_blob()
 ┌──────┬────────────────────────── fields, in fields() order ──────────────────────────┐
 │ ver  │  arithmetic: raw LE bytes                                                       │
 │ (u8) │  std::string: [u32 length][raw bytes]                                           │
 │      │  std::vector<T>: [u32 count][element][element]...                               │
 │      │  nested FlexData: its fields written in order (no per-element version byte)      │
 └──────┴─────────────────────────────────────────────────────────────────────────────┘
```

Only the top-level record carries the version byte; nested structs are written
field-by-field. Bump `FlexData::kBlobVersion` when a struct's on-disk layout
changes — `from_blob` rejects a mismatched version rather than misreading old
data.

### Reusing this from a module

Because `Nvs` is a registered core module (`controller.nvs`), any other module can
persist its config in a couple of lines:

```cpp
LedCfg cfg;
if (!controller.nvs.load("led", "cfg", cfg)) {
    // first boot: defaults already in cfg, save them
    controller.nvs.save("led", "cfg", cfg);
}
```

See `src/Tests/NvsFlex/NvsFlexTester.cpp` for a full round-trip test suite
(`$flex_test run`), including the nested-vector path.

## Scalar records

Pattern: `write<T>(namespace, key, value)` then `read<T>(namespace, key, default_value)`.

* `std::string_view` and `const char*` are write input types only; read them back
  as `std::string`.
* `String` is available only when `Arduino.h` is available.
* Integral support is width-based: signed/unsigned 1, 2, 4, and 8 byte types.
* Floating-point values are stored as blobs using `sizeof(T)`.

```cpp
nvs.write<bool>("demo", "bool", true);
bool    b   = nvs.read<bool>("demo", "bool", false);

nvs.write<int32_t>("demo", "int32", -32);
int32_t i32 = nvs.read<int32_t>("demo", "int32", 0);

nvs.write<uint32_t>("demo", "uint32", 32u);
uint32_t u32 = nvs.read<uint32_t>("demo", "uint32", 0);

nvs.write<float>("demo", "float", 3.14f);
float f = nvs.read<float>("demo", "float", 0.0f);

nvs.write<std::string>("demo", "std_string", std::string("hello"));
std::string s = nvs.read<std::string>("demo", "std_string", "");

// write input types read back as std::string
nvs.write<std::string_view>("demo", "sv", std::string_view("hello"));
nvs.write<const char*>("demo", "cstr", "hello");
std::string sv   = nvs.read<std::string>("demo", "sv", "");
std::string cstr = nvs.read<std::string>("demo", "cstr", "");
```
