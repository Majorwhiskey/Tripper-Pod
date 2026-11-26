// Board wiring and tunables. Pins are for ESP32-S3-DevKitC-1 N8R8 (decision D9);
// change only this file when the real board arrives.
#pragma once

#include <stdint.h>

namespace cfg {

// GC9A01 on SPI2. Avoids strapping pins (0, 3, 45, 46), the octal-PSRAM pins
// (33-37) and USB (19, 20).
constexpr int kPinLcdSclk = 12;
constexpr int kPinLcdMosi = 11;
constexpr int kPinLcdCs = 10;
constexpr int kPinLcdDc = 9;
constexpr int kPinLcdRst = 14;
constexpr int kPinLcdBacklight = 13;

// 40 MHz rather than the panel's 80 MHz maximum: margin for wiring that will
// vibrate on the handlebar. A full frame still takes ~23 ms.
constexpr uint32_t kLcdSpiHz = 40000000;
constexpr uint8_t kBacklightLevel = 255;  // full brightness for daylight

constexpr char kBleName[] = "HunterTripper";
constexpr uint16_t kBleMtu = 247;

// Packets buffered between the BLE task and the main loop. One icon is 4 packets.
constexpr int kLinkQueueDepth = 16;

}  // namespace cfg
