#include "ui.h"

#include <math.h>
#include <string.h>

#include "fonts/font_big.h"
#include "fonts/font_clock.h"
#include "fonts/font_text.h"
#include "format.h"
#include "text_fit.h"

namespace {

constexpr int kSize = 240;
constexpr int kCenter = kSize / 2;
constexpr int kEdgeMargin = 6;
constexpr int kIconScaled = 96;  // 64 px source at 1.5x

constexpr uint16_t kFg = 0xFFFF;
constexpr uint16_t kBg = 0x0000;
constexpr uint16_t kDim = 0x9CD3;   // grey
constexpr uint16_t kWarn = 0xFD20;  // amber

constexpr size_t kTextBuf = proto::kMaxInstrLen + 4;  // room for "..."

// Usable width of the round panel for text occupying rows [y - h/2, y + h/2].
int chordWidth(int y, int h) {
  const int dy = abs(y - kCenter) + h / 2;
  if (dy >= kCenter) return 0;
  return 2 * static_cast<int>(sqrtf(static_cast<float>(kCenter * kCenter - dy * dy))) -
         2 * kEdgeMargin;
}

int measure(const char* s, size_t len, void* ctx) {
  char tmp[kTextBuf];
  if (len >= sizeof tmp) len = sizeof tmp - 1;
  memcpy(tmp, s, len);
  tmp[len] = '\0';
  return static_cast<LGFX_Sprite*>(ctx)->textWidth(tmp);
}

}  // namespace

bool Ui::begin() {
  lcd_.init();
  lcd_.setBrightness(cfg::kBacklightLevel);
  lcd_.fillScreen(kBg);
  canvas_.setPsram(true);
  canvas_.setColorDepth(16);
  ready_ = canvas_.createSprite(kSize, kSize) != nullptr;
  if (ready_) canvas_.setTextDatum(textdatum_t::middle_center);
  return ready_;
}

bool Ui::render(const app::PodState& s, uint32_t now_ms) {
  if (!ready_) return false;
  const app::Screen screen = s.screen(now_ms);
  const int32_t sec = s.localSecondsOfDay(now_ms);
  const int32_t minute = sec < 0 ? -1 : sec / 60;
  if (drawn_once_ && s.revision() == last_rev_ && screen == last_screen_ &&
      minute == last_minute_) {
    return false;
  }

  canvas_.fillScreen(kBg);
  switch (screen) {
    case app::Screen::Clock: drawClockScreen(s, now_ms); break;
    case app::Screen::Nav: drawNavScreen(s, now_ms); break;
    case app::Screen::NoData: drawNoDataScreen(s, now_ms); break;
    case app::Screen::Call: drawCallScreen(s); break;
  }
  canvas_.pushSprite(0, 0);

  drawn_once_ = true;
  last_rev_ = s.revision();
  last_screen_ = screen;
  last_minute_ = minute;
  return true;
}

void Ui::drawClockScreen(const app::PodState& s, uint32_t now_ms) {
  char buf[16];
  app::formatClock(s.localSecondsOfDay(now_ms), buf, sizeof buf);
  canvas_.setFont(&kFontClock);
  canvas_.setTextColor(kFg);
  drawCentered(buf, kCenter - 10);

  canvas_.setFont(&kFontText);
  if (!s.connected()) {
    canvas_.setTextColor(kWarn);
    drawCentered("No phone", 170);
  } else if (s.phoneBattPct() <= 100) {
    snprintf(buf, sizeof buf, "Phone %u%%", static_cast<unsigned>(s.phoneBattPct()));
    canvas_.setTextColor(kDim);
    drawCentered(buf, 170);
  }
}

void Ui::drawNavScreen(const app::PodState& s, uint32_t now_ms) {
  const proto::NavText& nav = s.nav();
  char dist[16], eta[16];
  app::formatDistance(nav.dist_m, dist, sizeof dist);
  app::formatDuration(nav.eta_min, eta, sizeof eta);

  drawSmallClock(s, now_ms);

  // With an arrow, the instruction gets two lines; text-only mode gets three.
  const bool icon = s.iconReady();
  if (icon) drawIcon(s.icon(), kCenter - kIconScaled / 2, 26);

  canvas_.setFont(&kFontBig);
  canvas_.setTextColor(kFg);
  drawCentered(dist, icon ? 146 : 72);

  canvas_.setFont(&kFontText);
  if (icon) {
    drawWrapped(nav.instr, 180, 22, 2);
  } else {
    drawWrapped(nav.instr, 112, 24, 3);
  }

  canvas_.setTextColor(kDim);
  drawCentered(eta, icon ? 224 : 196);
}

void Ui::drawNoDataScreen(const app::PodState& s, uint32_t now_ms) {
  drawSmallClock(s, now_ms);
  canvas_.setFont(&kFontText);
  canvas_.setTextColor(kWarn);
  drawCentered("No nav data", 110);
  canvas_.setTextColor(kDim);
  drawCentered("Check Maps", 140);
}

void Ui::drawCallScreen(const app::PodState& s) {
  canvas_.setFont(&kFontText);
  canvas_.setTextColor(kWarn);
  drawCentered("Incoming call", 80);
  drawWrapped(s.caller()[0] ? s.caller() : "Unknown", 124, 26, 2);
}

void Ui::drawSmallClock(const app::PodState& s, uint32_t now_ms) {
  const int32_t sec = s.localSecondsOfDay(now_ms);
  if (sec < 0) return;
  char buf[8];
  app::formatClock(sec, buf, sizeof buf);
  canvas_.setFont(&kFontText);
  canvas_.setTextColor(kDim);
  drawCentered(buf, 12);
}

void Ui::drawIcon(const uint8_t* bmp, int x, int y) {
  // Nearest-neighbour 1.5x upscale of the 64x64 1-bpp bitmap (protocol: row-major,
  // MSB = leftmost pixel, 1 = foreground).
  for (int dy = 0; dy < kIconScaled; ++dy) {
    const uint8_t* row = bmp + (dy * 2 / 3) * 8;
    int run_start = -1;
    for (int dx = 0; dx <= kIconScaled; ++dx) {
      const int sx = dx * 2 / 3;
      const bool on = dx < kIconScaled && (row[sx >> 3] & (0x80 >> (sx & 7)));
      if (on && run_start < 0) run_start = dx;
      if (!on && run_start >= 0) {
        canvas_.drawFastHLine(x + run_start, y + dy, dx - run_start, kFg);
        run_start = -1;
      }
    }
  }
}

void Ui::drawCentered(const char* text, int y) { canvas_.drawString(text, kCenter, y); }

void Ui::drawWrapped(const char* text, int first_y, int line_h, int max_lines) {
  const size_t len = strlen(text);
  size_t pos = 0;
  char line[kTextBuf];
  for (int i = 0; i < max_lines && pos < len; ++i) {
    const int y = first_y + i * line_h;
    const int width = chordWidth(y, canvas_.fontHeight());
    app::Line l = app::wrapLine(text + pos, len - pos, width, measure, &canvas_);
    const char* src = text + pos + l.start;
    size_t n = l.len;
    const bool ellipsis = i == max_lines - 1 && pos + l.next < len;
    if (ellipsis) {
      const int room = width - canvas_.textWidth("...");
      n = app::fitPrefix(src, len - pos - l.start, room, measure, &canvas_);
      while (n > 0 && src[n - 1] == ' ') --n;
    }
    memcpy(line, src, n);
    memcpy(line + n, ellipsis ? "..." : "", ellipsis ? 4 : 1);
    drawCentered(line, y);
    pos += l.next;
  }
}
