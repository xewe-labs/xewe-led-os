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
#define STARTING_TEST_ID    563

// Use pins valid for your board.
#define LED_PIN_DATA        6
#define LED_PIN_CLOCK       7

// Pick one or more sizes to benchmark.
static const uint16_t kLedCounts[] = {100, 500, 1000};
constexpr uint8_t NUM_LED_COUNTS = sizeof(kLedCounts) / sizeof(kLedCounts[0]);

// Pick SPI clock rates to benchmark.
// If you change these, keep the clock cases in initController in sync.
static const uint8_t kSpiClockMHz[] = {2, 6, 12, 20, 40};
constexpr uint8_t NUM_SPI_CLOCKS = sizeof(kSpiClockMHz) / sizeof(kSpiClockMHz[0]);

// =========================
// Chipset selection (SPI only)
// =========================

enum class LEDChipset : uint8_t {
  APA102 = 0,
  APA102HD,
  DOTSTAR,
  DOTSTARHD,
  HD107,
  HD107HD,
  LPD6803,
  LPD8806,
  P9813,
  SK9822,
  SK9822HD,
  SM16716,
  WS2801,
  WS2803
};

struct LEDChipsetEntry {
  LEDChipset chipset;
  const char *name;
  const char *protocol;
};

inline static constexpr LEDChipsetEntry kChipsets[] = {
  { LEDChipset::APA102    , "APA102"    , "SPI" },
  { LEDChipset::APA102HD  , "APA102HD"  , "SPI" },
  { LEDChipset::DOTSTAR   , "DOTSTAR"   , "SPI" },
  { LEDChipset::DOTSTARHD , "DOTSTARHD" , "SPI" },
  { LEDChipset::HD107     , "HD107"     , "SPI" },
  { LEDChipset::HD107HD   , "HD107HD"   , "SPI" },
  { LEDChipset::LPD6803   , "LPD6803"   , "SPI" },
  { LEDChipset::LPD8806   , "LPD8806"   , "SPI" },
  { LEDChipset::P9813     , "P9813"     , "SPI" },
  { LEDChipset::SK9822    , "SK9822"    , "SPI" },
  { LEDChipset::SK9822HD  , "SK9822HD"  , "SPI" },
  { LEDChipset::SM16716   , "SM16716"   , "SPI" },
  { LEDChipset::WS2801    , "WS2801"    , "SPI" },
  { LEDChipset::WS2803    , "WS2803"    , "SPI" },
};

constexpr uint16_t NUM_CHIPSETS = sizeof(kChipsets) / sizeof(kChipsets[0]);
constexpr uint16_t NUM_PERMS    = NUM_CHIPSETS * NUM_LED_COUNTS * NUM_SPI_CLOCKS;
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
                  uint8_t &clockMHz,
                  uint8_t &runIndex,
                  uint16_t &permIndex) {
  permIndex = testId / RUNS_PER_CASE;
  runIndex  = testId % RUNS_PER_CASE;

  const uint16_t permsPerChip = NUM_LED_COUNTS * NUM_SPI_CLOCKS;
  const uint16_t chipIndex    = permIndex / permsPerChip;
  const uint16_t rem          = permIndex % permsPerChip;
  const uint16_t ledIndex     = rem / NUM_SPI_CLOCKS;
  const uint16_t clockIndex   = rem % NUM_SPI_CLOCKS;

  chip     = kChipsets[chipIndex].chipset;
  ledCount = kLedCounts[ledIndex];
  clockMHz = kSpiClockMHz[clockIndex];
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

#define SPI_CHIP_LIST(X, MHZ) \
  X(APA102,    MHZ)           \
  X(APA102HD,  MHZ)           \
  X(DOTSTAR,   MHZ)           \
  X(DOTSTARHD, MHZ)           \
  X(HD107,     MHZ)           \
  X(HD107HD,   MHZ)           \
  X(LPD6803,   MHZ)           \
  X(LPD8806,   MHZ)           \
  X(P9813,     MHZ)           \
  X(SK9822,    MHZ)           \
  X(SK9822HD,  MHZ)           \
  X(SM16716,   MHZ)           \
  X(WS2801,    MHZ)           \
  X(WS2803,    MHZ)

#define INIT_ONE_SPI(CHIP, MHZ)                                                      \
  case LEDChipset::CHIP:                                                              \
    FastLED.addLeds<CHIP, LED_PIN_DATA, LED_PIN_CLOCK, RGB, DATA_RATE_MHZ(MHZ)>(     \
      leds, ledCount                                                                  \
    ).setCorrection(TypicalLEDStrip);                                                 \
    return true;

bool initController(LEDChipset chip, uint16_t ledCount, uint8_t clockMHz) {
  FastLED.clear(true);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(0);

  switch (clockMHz) {
    case 2:
      switch (chip) {
        SPI_CHIP_LIST(INIT_ONE_SPI, 2)
        default: return false;
      }

    case 6:
      switch (chip) {
        SPI_CHIP_LIST(INIT_ONE_SPI, 6)
        default: return false;
      }

    case 12:
      switch (chip) {
        SPI_CHIP_LIST(INIT_ONE_SPI, 12)
        default: return false;
      }

    case 20:
      switch (chip) {
        SPI_CHIP_LIST(INIT_ONE_SPI, 20)
        default: return false;
      }

    case 40:
      switch (chip) {
        SPI_CHIP_LIST(INIT_ONE_SPI, 40)
        default: return false;
      }

    default:
      return false;
  }
}

#undef INIT_ONE_SPI
#undef SPI_CHIP_LIST

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

    nowUs = t2;
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
  Serial.println("chipset,protocol,spi_mhz,leds,run,frames,elapsed_us,avg_render_us,avg_show_us,avg_show_us_per_led,avg_total_us,fps");
}

void printRunCsv(LEDChipset chip, uint8_t clockMHz, uint16_t ledCount, uint8_t runIndex, const RunMetrics &m) {
  const float showUsPerLed = (ledCount > 0) ? (m.avgShowUs / (float)ledCount) : 0.0f;

  Serial.print(chipsetName(chip));      Serial.print(",");
  Serial.print(chipsetProtocol(chip));  Serial.print(",");
  Serial.print(clockMHz);               Serial.print(",");
  Serial.print(ledCount);               Serial.print(",");
  Serial.print(runIndex + 1);           Serial.print(",");
  Serial.print(m.frames);               Serial.print(",");
  Serial.print(m.elapsedUs);            Serial.print(",");
  Serial.print(m.avgRenderUs, 2);       Serial.print(",");
  Serial.print(m.avgShowUs, 2);         Serial.print(",");
  Serial.print(showUsPerLed, 4);        Serial.print(",");
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

void printSummaryIfComplete(uint16_t permIndex, LEDChipset chip, uint16_t ledCount, uint8_t clockMHz) {
  SummaryState s;
  loadSummary(s);

  if (s.permIndex != permIndex || s.runsDone != RUNS_PER_CASE || s.totalFrames == 0 || s.totalElapsedUs == 0) {
    return;
  }

  const float meanRenderUs  = (float)s.totalRenderUs / (float)s.totalFrames;
  const float meanShowUs    = (float)s.totalShowUs / (float)s.totalFrames;
  const float meanShowUsLed = (ledCount > 0) ? (meanShowUs / (float)ledCount) : 0.0f;
  const float meanTotalUs   = (float)s.totalElapsedUs / (float)s.totalFrames;
  const float meanFps       = (1000000.0f * (float)s.totalFrames) / (float)s.totalElapsedUs;

  Serial.println();
  Serial.print("SUMMARY ");
  Serial.print(chipsetName(chip));
  Serial.print(" @ ");
  Serial.print(clockMHz);
  Serial.print(" MHz, ");
  Serial.print(ledCount);
  Serial.println(" LEDs");

  Serial.print("  total_frames:         ");
  Serial.println((uint32_t)s.totalFrames);

  Serial.print("  mean_render_us:       ");
  Serial.println(meanRenderUs, 2);

  Serial.print("  mean_show_us:         ");
  Serial.println(meanShowUs, 2);

  Serial.print("  mean_show_us_per_led: ");
  Serial.println(meanShowUsLed, 4);

  Serial.print("  mean_total_us:        ");
  Serial.println(meanTotalUs, 2);

  Serial.print("  effective_fps:        ");
  Serial.println(meanFps, 3);
}

void markBenchmarkDone() {
  prefs.putBool("done", true);
}

void scheduleNextTestOrFinish(uint16_t currentTestId) {
  const uint16_t nextTestId = currentTestId + 1;

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

  Serial.flush();
  ESP.restart();
}

void setup() {
  delay(1000);
  Serial.begin(SERIAL_BAUD);

  uint32_t waitStart = millis();
  while (!Serial && (millis() - waitStart < 4000)) {
    delay(10);
  }

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
  uint8_t clockMHz;
  uint8_t runIndex;
  uint16_t permIndex;
  decodeTestId(testId, chip, ledCount, clockMHz, runIndex, permIndex);

  if (ledCount > MAX_LEDS) {
    Serial.println("Configured ledCount exceeds MAX_LEDS.");
    return;
  }

  if (runIndex == 0) {
    resetSummaryForPerm(permIndex);
  }

  Serial.println();
  Serial.print("test_id: ");   Serial.println(testId);
  Serial.print("chipset: ");   Serial.println(chipsetName(chip));
  Serial.print("protocol: ");  Serial.println(chipsetProtocol(chip));
  Serial.print("spi_mhz: ");   Serial.println(clockMHz);
  Serial.print("leds: ");      Serial.println(ledCount);
  Serial.print("run: ");       Serial.print(runIndex + 1);
  Serial.print("/");           Serial.println(RUNS_PER_CASE);

  if (!initController(chip, ledCount, clockMHz)) {
    Serial.println("Failed to init FastLED controller.");
    return;
  }

  fill_solid(leds, ledCount, CRGB::Black);
  FastLED.show(BRIGHTNESS);

  const RunMetrics metrics = runOneCase(ledCount, runIndex);
  printRunCsv(chip, clockMHz, ledCount, runIndex, metrics);

  updateSummary(permIndex, metrics);

  if ((runIndex + 1) == RUNS_PER_CASE) {
    printSummaryIfComplete(permIndex, chip, ledCount, clockMHz);
  }

  scheduleNextTestOrFinish(testId);
}

void loop() {
}