// LovyanGFX device for the 240x240 GC9A01 round panel.
#pragma once

#include <LovyanGFX.hpp>

#include "config.h"

class Display : public lgfx::LGFX_Device {
 public:
  Display() {
    auto bus = bus_.config();
    bus.spi_host = SPI2_HOST;
    bus.spi_mode = 0;
    bus.freq_write = cfg::kLcdSpiHz;
    bus.freq_read = 16000000;
    bus.spi_3wire = true;  // write-only panel, no MISO
    bus.use_lock = true;
    bus.dma_channel = SPI_DMA_CH_AUTO;
    bus.pin_sclk = cfg::kPinLcdSclk;
    bus.pin_mosi = cfg::kPinLcdMosi;
    bus.pin_miso = -1;
    bus.pin_dc = cfg::kPinLcdDc;
    bus_.config(bus);
    panel_.setBus(&bus_);

    auto panel = panel_.config();
    panel.pin_cs = cfg::kPinLcdCs;
    panel.pin_rst = cfg::kPinLcdRst;
    panel.pin_busy = -1;
    panel.panel_width = 240;
    panel.panel_height = 240;
    panel.invert = true;  // GC9A01 modules show inverted colours otherwise
    panel.readable = false;
    panel.bus_shared = false;
    panel_.config(panel);

    auto light = light_.config();
    light.pin_bl = cfg::kPinLcdBacklight;
    light.invert = false;
    light.freq = 44100;
    light.pwm_channel = 7;
    light_.config(light);
    panel_.setLight(&light_);

    setPanel(&panel_);
  }

 private:
  lgfx::Bus_SPI bus_;
  lgfx::Panel_GC9A01 panel_;
  lgfx::Light_PWM light_;
};
