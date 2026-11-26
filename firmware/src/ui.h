// Draws PodState onto the round display. Frames are composed in a PSRAM sprite and
// pushed whole, so the rider never sees a half-drawn screen.
#pragma once

#include <stdint.h>

#include "display.h"
#include "pod_state.h"

class Ui {
 public:
  // False if the framebuffer could not be allocated; render() is then a no-op.
  bool begin();

  // Redraws only when something visible changed. Returns true if a frame was pushed.
  bool render(const app::PodState& state, uint32_t now_ms);

 private:
  void drawClockScreen(const app::PodState& s, uint32_t now_ms);
  void drawNavScreen(const app::PodState& s, uint32_t now_ms);
  void drawNoDataScreen(const app::PodState& s, uint32_t now_ms);
  void drawCallScreen(const app::PodState& s);
  void drawSmallClock(const app::PodState& s, uint32_t now_ms);
  void drawIcon(const uint8_t* bitmap, int x, int y);
  void drawCentered(const char* text, int y);
  void drawWrapped(const char* text, int first_y, int line_h, int max_lines);

  Display lcd_;
  LGFX_Sprite canvas_{&lcd_};
  bool ready_ = false;
  bool drawn_once_ = false;
  uint32_t last_rev_ = 0;
  app::Screen last_screen_ = app::Screen::Clock;
  int32_t last_minute_ = -1;
};
