#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
from typing import List

OUT_PATH: Path | None = Path("XeWeFastLEDUtils.h")  # set to None to print

CLOCKLESS_TAGS: List[str] = [
    "APA104",
    "APA106",
    "GE8822",
    "GS1903",
    "GW6205",
    "GW6205_400",
    "LPD1886",
    "LPD1886_8BIT",
    "NEOPIXEL",
    "PL9823",
    "SK6812",
    "SK6822",
    "SM16703",
    "SM16824E",
    "TM1803",
    "TM1804",
    "TM1809",
    "TM1812",
    "TM1829",
    "UCS1903",
    "UCS1903B",
    "UCS1904",
    "UCS1912",
    "UCS2903",
    "WS2811",
    "WS2811_400",
    "WS2812",
    "WS2812B",
    "WS2813",
    "WS2815",
    "WS2816",
    "WS2852",
]

CLOCKED_TAGS: List[str] = [
    "APA102",
    "APA102HD",
    "DOTSTAR",
    "DOTSTARHD",
    "HD107",
    "HD107HD",
    "LPD6803",
    "LPD8806",
    "P9813",
    "SK9822",
    "SK9822HD",
    "SM16716",
    "WS2801",
    "WS2803",
]

ALL_TAGS: List[str] = CLOCKLESS_TAGS + CLOCKED_TAGS


def gen_text() -> str:
    lines: List[str] = []

    lines += [
        "#pragma once",
        "",
        "#include <cstdint>",
        "#include <FastLED.h>",
        "",
        "#include <Config.h>",
        "",
        "namespace xewe::led {",
        "",
        "enum class Chipset : std::uint8_t {",
    ]

    for t in CLOCKLESS_TAGS:
        lines.append(f"    {t},")
    lines.append("")
    for t in CLOCKED_TAGS:
        lines.append(f"    {t},")
    lines += [
        "};",
        "",
        "struct ChipsetEntry {",
        "    std::uint8_t id;",
        "    Chipset value;",
        "    const char* name;",
        "};",
        "",
        "inline constexpr ChipsetEntry kChipsetTable[] = {",
    ]

    for i, t in enumerate(ALL_TAGS):
        lines.append(f'    {{{i}, Chipset::{t}, "{t}"}},')

    lines += [
        "};",
        "",
        "inline void initFastLED_Generated(const Chipset chipset) {",
        "    switch (chipset) {",
    ]

    # Exact alignment: spaces after ':' are (16 - len(tag))
    def case_line(tag: str, body: str) -> str:
        pad = " " * (16 - len(tag))
        return f"        case Chipset::{tag}:{pad}{body} return;"

    for t in CLOCKLESS_TAGS:
        if t == "NEOPIXEL":
            body = "FastLED.addLeds<NEOPIXEL, LED_PIN_DATA>(leds, LED_STRIP_NUM_LEDS_MAX);"
        else:
            body = f"FastLED.addLeds<{t}, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX);"
        lines.append(case_line(t, body))

    lines.append("")

    for t in CLOCKED_TAGS:
        body = f"FastLED.addLeds<{t}, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX);"
        lines.append(case_line(t, body))

    lines += [
        "",
        "        default:",
        "            return;",
        "    }",
        "}",
        "",
        "} // namespace xewe::led",
        "",
    ]

    return "\n".join(lines)


def main() -> None:
    out = gen_text()
    if OUT_PATH is None:
        print(out, end="")
    else:
        OUT_PATH.write_text(out, encoding="utf-8")


if __name__ == "__main__":
    main()