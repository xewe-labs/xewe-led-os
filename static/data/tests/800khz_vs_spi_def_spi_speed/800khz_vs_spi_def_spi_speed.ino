#include <FastLED.h>
#include <Preferences.h>
#include <esp_system.h>
#include <cstring>

// =========================
// Hardware / benchmark settings
// =========================
#define SERIAL_BAUD         115200

#define BRIGHTNESS          10
#define MAX_LEDS            2000
#define RUN_DURATION_MS     5000
#define RUNS_PER_CASE       3

// Used only on first boot (or after NVS erase / missing first_boot key).
#define STARTING_TEST_ID    0

// Use pins valid for your board.
#define LED_PIN_DATA        1
#define LED_PIN_CLOCK       2

// Pick one or more sizes to benchmark.
static const uint16_t kLedCounts[] = {100, 500, 1000};
constexpr uint8_t NUM_LED_COUNTS = sizeof(kLedCounts) / sizeof(kLedCounts[0]);

// =========================
// Chipset selection
// =========================

enum class LEDChipset : uint8_t {
  APA102 = 0,
  APA102HD,
  APA104,
  APA106,
  DOTSTAR,
  DOTSTARHD,
  GE8822,
  GS1903,
  GW6205,
  GW6205_400_KHZ,
  HD107,
  HD107HD,
  LPD1886,
  LPD1886_8BIT,
  LPD6803,
  LPD8806,
  NEOPIXEL,
  P9813,
  PL9823,
  SK6812,
  SK6822,
  SK9822,
  SK9822HD,
  SM16703,
  SM16716,
  SM16824E,
  TM1803,
  TM1804,
  TM1809,
  TM1812,
  TM1829,
  UCS1903,
  UCS1903B,
  UCS1904,
  UCS1912,
  UCS2903,
  WS2801,
  WS2803,
  WS2811,
  WS2811_400_KHZ,
  WS2812,
  WS2812B,
  WS2813,
  WS2815,
  WS2816,
  WS2852
};

struct LEDChipsetEntry {
  LEDChipset chipset;
  const char *name;
  const char *protocol;
};

inline static constexpr LEDChipsetEntry kChipsets[] = {
  { LEDChipset::APA102          , "APA102"        , "SPI"     },
  { LEDChipset::APA102HD        , "APA102HD"      , "SPI"     },
  { LEDChipset::APA104          , "APA104"        , "1-wire"  },
  { LEDChipset::APA106          , "APA106"        , "1-wire"  },
  { LEDChipset::DOTSTAR         , "DOTSTAR"       , "SPI"     },
  { LEDChipset::DOTSTARHD       , "DOTSTARHD"     , "SPI"     },
  { LEDChipset::GE8822          , "GE8822"        , "1-wire"  },
  { LEDChipset::GS1903          , "GS1903"        , "1-wire"  },
  { LEDChipset::GW6205          , "GW6205"        , "1-wire"  },
  { LEDChipset::GW6205_400_KHZ  , "GW6205_400KHZ" , "400kHz"  },
  { LEDChipset::HD107           , "HD107"         , "SPI"     },
  { LEDChipset::HD107HD         , "HD107HD"       , "SPI"     },
  { LEDChipset::LPD1886         , "LPD1886"       , "1-wire"  },
  { LEDChipset::LPD1886_8BIT    , "LPD1886_8BIT"  , "1-wire"  },
  { LEDChipset::LPD6803         , "LPD6803"       , "SPI"     },
  { LEDChipset::LPD8806         , "LPD8806"       , "SPI"     },
  { LEDChipset::NEOPIXEL        , "NEOPIXEL"      , "1-wire"  },
  { LEDChipset::P9813           , "P9813"         , "SPI"     },
  { LEDChipset::PL9823          , "PL9823"        , "1-wire"  },
  { LEDChipset::SK6812          , "SK6812"        , "1-wire"  },
  { LEDChipset::SK6822          , "SK6822"        , "1-wire"  },
  { LEDChipset::SK9822          , "SK9822"        , "SPI"     },
  { LEDChipset::SK9822HD        , "SK9822HD"      , "SPI"     },
  { LEDChipset::SM16703         , "SM16703"       , "1-wire"  },
  { LEDChipset::SM16716         , "SM16716"       , "SPI"     },
  { LEDChipset::SM16824E        , "SM16824E"      , "1-wire"  },
  { LEDChipset::TM1803          , "TM1803"        , "1-wire"  },
  { LEDChipset::TM1804          , "TM1804"        , "1-wire"  },
  { LEDChipset::TM1809          , "TM1809"        , "1-wire"  },
  { LEDChipset::TM1812          , "TM1812"        , "1-wire"  },
  { LEDChipset::TM1829          , "TM1829"        , "1-wire"  },
  { LEDChipset::UCS1903         , "UCS1903"       , "1-wire"  },
  { LEDChipset::UCS1903B        , "UCS1903B"      , "1-wire"  },
  { LEDChipset::UCS1904         , "UCS1904"       , "1-wire"  },
  { LEDChipset::UCS1912         , "UCS1912"       , "1-wire"  },
  { LEDChipset::UCS2903         , "UCS2903"       , "1-wire"  },
  { LEDChipset::WS2801          , "WS2801"        , "SPI"     },
  { LEDChipset::WS2803          , "WS2803"        , "SPI"     },
  { LEDChipset::WS2811          , "WS2811"        , "1-wire"  },
  { LEDChipset::WS2811_400_KHZ  , "WS2811_400KHZ" , "400kHz"  },
  { LEDChipset::WS2812          , "WS2812"        , "1-wire"  },
  { LEDChipset::WS2812B         , "WS2812B"       , "1-wire"  },
  { LEDChipset::WS2813          , "WS2813"        , "1-wire"  },
  { LEDChipset::WS2815          , "WS2815"        , "1-wire"  },
  { LEDChipset::WS2816          , "WS2816"        , "1-wire"  },
  { LEDChipset::WS2852          , "WS2852"        , "1-wire"  },
};

constexpr uint16_t NUM_CHIPSETS = sizeof(kChipsets) / sizeof(kChipsets[0]);
constexpr uint16_t NUM_PERMS    = NUM_CHIPSETS * NUM_LED_COUNTS;
constexpr uint16_t TOTAL_TESTS  = NUM_PERMS * RUNS_PER_CASE;

// =========================
// Globals
// =========================

Preferences prefs;
CRGB leds[MAX_LEDS];

struct RunMetrics {
  uint32_t frames;
  uint32_t elapsedUs;
  uint64_t renderSumUs;
  uint64_t showSumUs;
  float avgRenderUs;
  float avgShowUs;
  float avgTotalUs;
  float fps;
};

struct SummaryState {
  uint16_t permIndex;
  uint16_t runsDone;
  uint64_t totalFrames;
  uint64_t totalElapsedUs;
  uint64_t totalRenderUs;
  uint64_t totalShowUs;
};

const LEDChipsetEntry &chipsetEntry(LEDChipset c) {
  for (uint16_t i = 0; i < NUM_CHIPSETS; ++i) {
    if (kChipsets[i].chipset == c) {
      return kChipsets[i];
    }
  }
  return kChipsets[0];
}

const char *chipsetName(LEDChipset c) {
  return chipsetEntry(c).name;
}

const char *chipsetProtocol(LEDChipset c) {
  return chipsetEntry(c).protocol;
}

void decodeTestId(uint16_t testId,
                  LEDChipset &chip,
                  uint16_t &ledCount,
                  uint8_t &runIndex,
                  uint16_t &permIndex) {
  permIndex = testId / RUNS_PER_CASE;
  runIndex  = testId % RUNS_PER_CASE;

  const uint16_t chipIndex     = permIndex / NUM_LED_COUNTS;
  const uint16_t ledCountIndex = permIndex % NUM_LED_COUNTS;

  chip     = kChipsets[chipIndex].chipset;
  ledCount = kLedCounts[ledCountIndex];
}

// Missing key => false => treat as "not initialized yet".
// After seeding NVS once, set first_boot=true so later boots resume from NVS.
void initializeTestStateOnFirstBoot() {
  const bool firstBootInitialized = prefs.getBool("first_boot", false);
  if (firstBootInitialized) {
    return;
  }

  uint16_t startId = STARTING_TEST_ID;

  if (TOTAL_TESTS == 0) {
    startId = 0;
  } else if (startId >= TOTAL_TESTS) {
    startId = TOTAL_TESTS - 1;
  }

  prefs.putUInt("testId", startId);
  prefs.putBool("done", false);
  prefs.remove("summary");

  // Clean up obsolete keys from the old one-shot start-id logic.
  prefs.remove("start_id_applied");
  prefs.remove("applied_start_id");

  prefs.putBool("first_boot", true);

  Serial.print("Initialized starting test id: ");
  Serial.println(startId);
}

bool initController(LEDChipset chip, uint16_t ledCount) {
  FastLED.clear(true);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(0);

  switch (chip) {
    case LEDChipset::APA102:          FastLED.addLeds<APA102,       LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::APA102HD:        FastLED.addLeds<APA102HD,     LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::APA104:          FastLED.addLeds<APA104,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::APA106:          FastLED.addLeds<APA106,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::DOTSTAR:         FastLED.addLeds<DOTSTAR,      LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::DOTSTARHD:       FastLED.addLeds<DOTSTARHD,    LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::GE8822:          FastLED.addLeds<GE8822,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::GS1903:          FastLED.addLeds<GS1903,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::GW6205:          FastLED.addLeds<GW6205,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::GW6205_400_KHZ:  FastLED.addLeds<GW6205_400,   LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::HD107:           FastLED.addLeds<HD107,        LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::HD107HD:         FastLED.addLeds<HD107HD,      LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::LPD1886:         FastLED.addLeds<LPD1886,      LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::LPD1886_8BIT:    FastLED.addLeds<LPD1886_8BIT, LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::LPD6803:         FastLED.addLeds<LPD6803,      LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::LPD8806:         FastLED.addLeds<LPD8806,      LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::NEOPIXEL:        FastLED.addLeds<NEOPIXEL,     LED_PIN_DATA>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::P9813:           FastLED.addLeds<P9813,        LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::PL9823:          FastLED.addLeds<PL9823,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::SK6812:          FastLED.addLeds<SK6812,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::SK6822:          FastLED.addLeds<SK6822,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::SK9822:          FastLED.addLeds<SK9822,       LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::SK9822HD:        FastLED.addLeds<SK9822HD,     LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::SM16703:         FastLED.addLeds<SM16703,      LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::SM16716:         FastLED.addLeds<SM16716,      LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::SM16824E:        FastLED.addLeds<SM16824E,     LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::TM1803:          FastLED.addLeds<TM1803,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::TM1804:          FastLED.addLeds<TM1804,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::TM1809:          FastLED.addLeds<TM1809,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::TM1812:          FastLED.addLeds<TM1812,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::TM1829:          FastLED.addLeds<TM1829,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::UCS1903:         FastLED.addLeds<UCS1903,      LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::UCS1903B:        FastLED.addLeds<UCS1903B,     LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::UCS1904:         FastLED.addLeds<UCS1904,      LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::UCS1912:         FastLED.addLeds<UCS1912,      LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::UCS2903:         FastLED.addLeds<UCS2903,      LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2801:          FastLED.addLeds<WS2801,       LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2803:          FastLED.addLeds<WS2803,       LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2811:          FastLED.addLeds<WS2811,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2811_400_KHZ:  FastLED.addLeds<WS2811_400,   LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2812:          FastLED.addLeds<WS2812,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2812B:         FastLED.addLeds<WS2812B,      LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2813:          FastLED.addLeds<WS2813,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2815:          FastLED.addLeds<WS2815,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2816:          FastLED.addLeds<WS2816,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    case LEDChipset::WS2852:          FastLED.addLeds<WS2852,       LED_PIN_DATA, RGB>(leds, ledCount).setCorrection(TypicalLEDStrip); return true;
    default: return false;
  }
}

void renderPattern(CRGB *buf, uint16_t count, uint32_t frameIndex) {
  for (uint16_t i = 0; i < count; ++i) {
    const uint8_t hue = static_cast<uint8_t>((i * 3U) + (frameIndex * 7U));
    const uint8_t val = sin8(static_cast<uint8_t>((i * 5U) + (frameIndex * 11U)));
    buf[i] = CHSV(hue, 255, val);
  }
}

RunMetrics runOneCase(uint16_t ledCount, uint8_t runIndex) {
  uint64_t renderSumUs = 0;
  uint64_t showSumUs   = 0;
  uint32_t frames      = 0;

  const uint32_t runStartUs = micros();
  uint32_t nowUs = runStartUs;

  while ((uint32_t)(nowUs - runStartUs) < (RUN_DURATION_MS * 1000UL)) {
    const uint32_t t0 = micros();
    renderPattern(leds, ledCount, (uint32_t)(runIndex * 100000UL + frames));
    const uint32_t t1 = micros();
    FastLED.show(BRIGHTNESS);
    const uint32_t t2 = micros();

    renderSumUs += (uint32_t)(t1 - t0);
    showSumUs   += (uint32_t)(t2 - t1);
    frames++;

    if ((frames & 63U) == 0U) {
      yield();
    }

    nowUs = micros();
  }

  const uint32_t elapsedUs = (uint32_t)(nowUs - runStartUs);

  RunMetrics m{};
  m.frames      = frames;
  m.elapsedUs   = elapsedUs;
  m.renderSumUs = renderSumUs;
  m.showSumUs   = showSumUs;

  if (frames > 0) {
    m.avgRenderUs = (float)renderSumUs / (float)frames;
    m.avgShowUs   = (float)showSumUs / (float)frames;
    m.avgTotalUs  = (float)elapsedUs / (float)frames;
    m.fps         = (1000000.0f * (float)frames) / (float)elapsedUs;
  }

  return m;
}

void printCsvHeader() {
  Serial.println();
  Serial.println("chipset,protocol,leds,run,frames,elapsed_us,avg_render_us,avg_show_us,avg_total_us,fps");
}

void printRunCsv(LEDChipset chip, uint16_t ledCount, uint8_t runIndex, const RunMetrics &m) {
  Serial.print(chipsetName(chip));      Serial.print(",");
  Serial.print(chipsetProtocol(chip));  Serial.print(",");
  Serial.print(ledCount);               Serial.print(",");
  Serial.print(runIndex + 1);           Serial.print(",");
  Serial.print(m.frames);               Serial.print(",");
  Serial.print(m.elapsedUs);            Serial.print(",");
  Serial.print(m.avgRenderUs, 2);       Serial.print(",");
  Serial.print(m.avgShowUs, 2);         Serial.print(",");
  Serial.print(m.avgTotalUs, 2);        Serial.print(",");
  Serial.println(m.fps, 3);
}

void loadSummary(SummaryState &s) {
  memset(&s, 0, sizeof(s));
  if (prefs.getBytesLength("summary") == sizeof(SummaryState)) {
    prefs.getBytes("summary", &s, sizeof(SummaryState));
  } else {
    s.permIndex = 0xFFFF;
  }
}

void saveSummary(const SummaryState &s) {
  prefs.putBytes("summary", &s, sizeof(SummaryState));
}

void resetSummaryForPerm(uint16_t permIndex) {
  SummaryState s{};
  s.permIndex = permIndex;
  s.runsDone  = 0;
  saveSummary(s);
}

void updateSummary(uint16_t permIndex, const RunMetrics &m) {
  SummaryState s;
  loadSummary(s);

  if (s.permIndex != permIndex) {
    resetSummaryForPerm(permIndex);
    loadSummary(s);
  }

  s.runsDone++;
  s.totalFrames    += m.frames;
  s.totalElapsedUs += m.elapsedUs;
  s.totalRenderUs  += m.renderSumUs;
  s.totalShowUs    += m.showSumUs;

  saveSummary(s);
}

void printSummaryIfComplete(uint16_t permIndex, LEDChipset chip, uint16_t ledCount) {
  SummaryState s;
  loadSummary(s);

  if (s.permIndex != permIndex || s.runsDone != RUNS_PER_CASE || s.totalFrames == 0 || s.totalElapsedUs == 0) {
    return;
  }

  const float meanRenderUs = (float)s.totalRenderUs / (float)s.totalFrames;
  const float meanShowUs   = (float)s.totalShowUs / (float)s.totalFrames;
  const float meanTotalUs  = (float)s.totalElapsedUs / (float)s.totalFrames;
  const float meanFps      = (1000000.0f * (float)s.totalFrames) / (float)s.totalElapsedUs;

  Serial.println();
  Serial.print("SUMMARY ");
  Serial.print(chipsetName(chip));
  Serial.print(" ");
  Serial.print(ledCount);
  Serial.println(" LEDs");

  Serial.print("  total_frames:    ");
  Serial.println((uint32_t)s.totalFrames);

  Serial.print("  mean_render_us:  ");
  Serial.println(meanRenderUs, 2);

  Serial.print("  mean_show_us:    ");
  Serial.println(meanShowUs, 2);

  Serial.print("  mean_total_us:   ");
  Serial.println(meanTotalUs, 2);

  Serial.print("  effective_fps:   ");
  Serial.println(meanFps, 3);
}

void markBenchmarkDone() {
  prefs.putBool("done", true);
}

void scheduleNextPermOrFinish(uint16_t permIndex) {
  const uint16_t nextTestId = (uint16_t)((permIndex + 1) * RUNS_PER_CASE);

  if (nextTestId >= TOTAL_TESTS) {
    markBenchmarkDone();
    Serial.println();
    Serial.println("Benchmark complete.");
    return;
  }

  prefs.putUInt("testId", nextTestId);

  Serial.println();
  Serial.print("Next test id: ");
  Serial.println(nextTestId);
  Serial.println("Rebooting...");

  delay(50);
  ESP.restart();
}

void runRemainingRunsForCurrentPerm(LEDChipset chip,
                                    uint16_t ledCount,
                                    uint8_t startRunIndex,
                                    uint16_t permIndex) {
  SummaryState s;
  loadSummary(s);

  if (startRunIndex == 0) {
    resetSummaryForPerm(permIndex);
  } else {
    if (s.permIndex != permIndex || s.runsDone != startRunIndex) {
      Serial.println("Summary mismatch, restarting this perm from run 1.");
      resetSummaryForPerm(permIndex);
      startRunIndex = 0;
      prefs.putUInt("testId", (uint16_t)(permIndex * RUNS_PER_CASE));
    }
  }

  for (uint8_t runIndex = startRunIndex; runIndex < RUNS_PER_CASE; ++runIndex) {
    const uint16_t currentTestId = (uint16_t)(permIndex * RUNS_PER_CASE + runIndex);

    Serial.println();
    Serial.print("test_id: ");   Serial.println(currentTestId);
    Serial.print("chipset: ");   Serial.println(chipsetName(chip));
    Serial.print("protocol: ");  Serial.println(chipsetProtocol(chip));
    Serial.print("leds: ");      Serial.println(ledCount);
    Serial.print("run: ");       Serial.print(runIndex + 1);
    Serial.print("/");           Serial.println(RUNS_PER_CASE);

    const RunMetrics metrics = runOneCase(ledCount, runIndex);
    printRunCsv(chip, ledCount, runIndex, metrics);

    updateSummary(permIndex, metrics);

    // checkpoint after each run so power loss resumes correctly
    prefs.putUInt("testId", (uint16_t)(currentTestId + 1));

    delay(10);
  }

  printSummaryIfComplete(permIndex, chip, ledCount);
  scheduleNextPermOrFinish(permIndex);
}

void setup() {
  delay(250);
  Serial.setTxTimeoutMs(0);
  Serial.begin(SERIAL_BAUD);
  delay(250);

  prefs.begin("ledbench", false);
  initializeTestStateOnFirstBoot();

  const uint16_t testId = prefs.getUInt("testId", 0);
  const bool done       = prefs.getBool("done", false);

  printCsvHeader();

  if (done || testId >= TOTAL_TESTS) {
    Serial.println("Benchmark already complete.");
    return;
  }

  LEDChipset chip;
  uint16_t ledCount;
  uint8_t runIndex;
  uint16_t permIndex;
  decodeTestId(testId, chip, ledCount, runIndex, permIndex);

  if (ledCount > MAX_LEDS) {
    Serial.println("Configured ledCount exceeds MAX_LEDS.");
    return;
  }

  if (!initController(chip, ledCount)) {
    Serial.println("Failed to init FastLED controller.");
    return;
  }

  fill_solid(leds, ledCount, CRGB::Black);
  FastLED.show(BRIGHTNESS);
  delay(2);

  runRemainingRunsForCurrentPerm(chip, ledCount, runIndex, permIndex);
}

void loop() {
}