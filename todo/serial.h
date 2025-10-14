
print(
    string_view message="", /* array of lines separated with \n */
    message_width=0,        /* meaning no target width */
    edge_character="|",     /* char at the ends of the line */
    text_allign="l",        /* l, r, c allign text */
    min_margin_l=0,         /* min margin on left if text does not fit, there will be next line */
    min_margin_r=0,         /* min margin on right if text does not fit, there will be next line */
    end=""
)

println(
    // same params
    end="\n"
)

printf(
    ...// not sure how to do this one
)

print_centered()
    text_allign="c"
    min_margin_l = 4

print_tab(mesg, tab=1)
    text_allign="l"
    min_margin_l = tab * 4

print_separator(
    // line that looks like +----------+, def width = 50
)

print_spacer(
    // line that looks like |      |, def width = 50
)

print_header(
    separator() +----+
    message="header 1\sep header2"
    separator() +----+
)


/// gpt
#pragma once
#include <Arduino.h>
#include <stdarg.h>

namespace Pretty {

enum class Align : uint8_t { Left, Right, Center };

struct Options {
  int      message_width = 0;   // 0 = no wrapping/target width
  char     edge          = '|'; // single character at both ends
  Align    align         = Align::Left;
  uint8_t  min_margin_l  = 0;
  uint8_t  min_margin_r  = 0;
  const char* end        = "";  // appended after EACH emitted line
};

// ---------- helpers ----------
static inline void writeFill(Print& out, char ch, size_t n) {
  while (n--) out.write((uint8_t)ch);
}

static inline void writeChunk(Print& out, const char* s, size_t n) {
  out.write(reinterpret_cast<const uint8_t*>(s), n);
}

// Emits one “visual” line (already cut/wrapped) with edges/margins/padding.
static inline void emitLine(Print& out,
                            const char* s, size_t n,
                            const Options& o,
                            int avail /*only used when width>0*/) {
  out.write((uint8_t)o.edge);
  writeFill(out, ' ', o.min_margin_l);

  int leftPad = 0, rightPad = 0;
  if (o.message_width > 0) {
    int gap = avail - (int)n;
    if (gap < 0) gap = 0;
    switch (o.align) {
      case Align::Left:   leftPad = 0;       rightPad = gap; break;
      case Align::Right:  leftPad = gap;     rightPad = 0;   break;
      case Align::Center: leftPad = gap/2;   rightPad = gap - leftPad; break;
    }
  }
  writeFill(out, ' ', leftPad);
  writeChunk(out, s, n);
  writeFill(out, ' ', rightPad);

  writeFill(out, ' ', o.min_margin_r);
  out.write((uint8_t)o.edge);

  if (o.end && *o.end) out.print(o.end);
}

// Emits a single source line s[0..len) possibly wrapped to width.
static inline void wrapAndEmit(Print& out, const char* s, size_t len, const Options& o) {
  if (o.message_width <= 0) {
    // No target width: print raw line with edges and margins, no wrapping.
    emitLine(out, s, len, o, 0);
    return;
  }

  // Effective content width (between edges and outside margins).
  int avail = o.message_width - 2 - o.min_margin_l - o.min_margin_r;
  if (avail < 1) {
    // Degenerate width—fallback to edges only.
    emitLine(out, s, len, o, 0);
    return;
  }

  size_t i = 0;
  while (i < len) {
    size_t remain = len - i;
    size_t take = (remain <= (size_t)avail) ? remain : (size_t)avail;

    if (remain > (size_t)avail) {
      // Try to wrap on whitespace before the hard cut.
      size_t j = i + take;
      size_t back = take;
      while (back > 0) {
        char c = s[i + back - 1];
        if (c == ' ' || c == '\t' || c == '-') { j = i + back; break; }
        --back;
      }
      if (back > 0) {
        // Trim trailing space from the emitted chunk.
        size_t chunkLen = j - i;
        while (chunkLen > 0 && (s[i + chunkLen - 1] == ' ' || s[i + chunkLen - 1] == '\t'))
          --chunkLen;
        emitLine(out, s + i, chunkLen, o, avail);
        // Skip any leading spaces before next chunk.
        i = j;
        while (i < len && (s[i] == ' ' || s[i] == '\t')) ++i;
        continue;
      }
    }

    // Hard wrap.
    emitLine(out, s + i, take, o, avail);
    i += take;
    // Skip a single leading space after a hard wrap (nicety).
    if (i < len && s[i] == ' ') ++i;
  }
}

// Parse message by \n / \r\n and feed to wrap+emit.
static inline void print_core(Print& out, const char* message, const Options& o) {
  const char* p = message;
  while (*p) {
    const char* lineStart = p;
    size_t      len = 0;
    while (*p && *p != '\n' && *p != '\r') { ++p; ++len; }
    wrapAndEmit(out, lineStart, len, o);
    // Handle \r\n / \n / \r
    if (*p == '\r' && p[1] == '\n') p += 2;
    else if (*p == '\r' || *p == '\n') ++p;
  }
}

// ---------- public API ----------

// Pretty::print(Serial, "...", options)
static inline void print(Print& out, const char* message, const Options& o = Options{}) {
  print_core(out, message, o);
}

// Pretty::println(Serial, "...", options)  -> same as print but default end="\n"
static inline void println(Print& out, const char* message, Options o = Options{}) {
  o.end = "\n";
  print_core(out, message, o);
}

// Convenience overloads (optional)
static inline void print(Print& out, const String& s, const Options& o = Options{}) {
  print(out, s.c_str(), o);
}
static inline void println(Print& out, const String& s, Options o = Options{}) {
  println(out, s.c_str(), o);
}
#if __cplusplus >= 201703L
#include <string_view>
static inline void print(Print& out, std::string_view sv, const Options& o = Options{}) {
  // We’ll copy to a small temp to reuse the same machinery.
  // (You can optimize further if needed.)
  String tmp; tmp.reserve(sv.size()); tmp.concat(sv.data(), sv.size());
  print(out, tmp.c_str(), o);
}
static inline void println(Print& out, std::string_view sv, Options o = Options{}) {
  o.end = "\n";
  print(out, sv, o);
}
#endif

// printf-style: format then pretty-print.
// NOTE: opts comes BEFORE fmt because ... must be last.
#ifndef PRETTY_PRINTF_BUFFER
#define PRETTY_PRINTF_BUFFER 256
#endif

static inline void printf(Print& out, const Options& o, const char* fmt, ...) {
  char buf[PRETTY_PRINTF_BUFFER];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  print_core(out, buf, o);
}

static inline void printfln(Print& out, Options o, const char* fmt, ...) {
  o.end = "\n";
  char buf[PRETTY_PRINTF_BUFFER];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  print_core(out, buf, o);
}

} // namespace Pretty

#include "PrettySerial.h"

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  using namespace Pretty;

  Options box;
  box.message_width = 32;
  box.edge = '#';
  box.align = Align::Center;
  box.min_margin_l = 1;
  box.min_margin_r = 1;

  println(Serial, "Hello world\nThis is wrapped nicely.", box);

  // Left-aligned, pipe edges, no wrapping
  Options raw;
  raw.align = Align::Left;
  raw.end = "\n";
  print(Serial, "no width -> no wrap, but still edged", raw);

  // Right-aligned with wrapping
  Options r;
  r.message_width = 24;
  r.align = Align::Right;
  r.end = "\n";
  print(Serial, "Right aligned example to show padding and hard/soft wraps.", r);

  // printf-style
  Options p;
  p.message_width = 28;
  p.align = Align::Center;
  printfln(Serial, p, "temp=%0.1f°C  v=%dmV", 23.6, 3312);
}

void loop() {}


// ---- Add below the previous Pretty code, still inside namespace Pretty ----

constexpr int DEFAULT_RULE_WIDTH = 50;

// Centered print. Defaults to align center and min left margin = 4.
// If you pass width>0, text will be padded/centered to that width.
static inline void print_centered(Print& out, const char* message,
                                  int width = 0, Options o = Options{}) {
  o.align = Align::Center;
  if (o.min_margin_l < 4) o.min_margin_l = 4;
  if (width > 0) o.message_width = width;
  print(out, message, o);
}
static inline void println_centered(Print& out, const char* message,
                                    int width = 0, Options o = Options{}) {
  o.align = Align::Center;
  if (o.min_margin_l < 4) o.min_margin_l = 4;
  if (width > 0) o.message_width = width;
  o.end = "\n";
  print(out, message, o);
}

// Tabbed print. Each "tab" = 4 spaces on the left.
static inline void print_tab(Print& out, const char* message,
                             uint8_t tab = 1, Options o = Options{}) {
  o.align = Align::Left;
  o.min_margin_l = (uint8_t)(tab * 4);
  print(out, message, o);
}
static inline void println_tab(Print& out, const char* message,
                               uint8_t tab = 1, Options o = Options{}) {
  o.align = Align::Left;
  o.min_margin_l = (uint8_t)(tab * 4);
  o.end = "\n";
  print(out, message, o);
}

// +----------+ style horizontal rule.
static inline void print_separator(Print& out,
                                   int width = DEFAULT_RULE_WIDTH,
                                   char corner = '+',
                                   char fill   = '-',
                                   const char* end = "\n") {
  if (width < 2) width = 2;
  out.write((uint8_t)corner);
  writeFill(out, fill, (size_t)(width - 2));
  out.write((uint8_t)corner);
  if (end && *end) out.print(end);
}

// |          | blank spacer line (keeps box edges).
static inline void print_spacer(Print& out,
                                int width = DEFAULT_RULE_WIDTH,
                                char edge = '|',
                                const char* end = "\n") {
  if (width < 2) width = 2;
  out.write((uint8_t)edge);
  writeFill(out, ' ', (size_t)(width - 2));
  out.write((uint8_t)edge);
  if (end && *end) out.print(end);
}

// Header block:
//   +----------+
//   |  Title   |
//   | Subtitle |
//   +----------+
// Split multiple header lines using the literal token "\sep".
static inline void print_header(Print& out,
                                const char* message,
                                int width = DEFAULT_RULE_WIDTH,
                                char corner = '+',    // corners for top/bottom
                                char hfill  = '-',    // horizontal fill
                                char vedge  = '|',    // vertical edges for text lines
                                uint8_t inner_margin = 1) {
  // top rule
  print_separator(out, width, corner, hfill, "\n");

  // body (centered by default, with 1-space inner margins)
  Options o;
  o.message_width = width;
  o.edge = vedge;
  o.align = Align::Center;
  o.min_margin_l = inner_margin;
  o.min_margin_r = inner_margin;
  o.end = "\n";

  // Replace "\sep" tokens with actual newlines (so you can pass one string)
  String s(message);
  s.replace("\\sep", "\n");

  print(out, s.c_str(), o);

  // bottom rule
  print_separator(out, width, corner, hfill, "\n");
}


using namespace Pretty;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  // Centered line (width 50, min left margin 4 by default)
  println_centered(Serial, "Device Boot", 50);

  // Tabbed list
  println_tab(Serial, "- item A", 1);
  println_tab(Serial, "- item B", 2);

  // Rules and spacers
  print_separator(Serial);     // +-----------------------------------------------+
  print_spacer(Serial);        // |                                               |

  // Header block (multiple lines with \sep)
  print_header(Serial, "Main Menu\\sepFirmware v1.2.3", 48);

  // Combine with the earlier printf helpers if you like:
  Options p; p.message_width = 48; p.align = Align::Center;
  printfln(Serial, p, "Temp: %.1f C\\sepBattery: %u mV", 23.7, 3712);
}

void loop() {}
