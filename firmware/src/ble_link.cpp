#include "ble_link.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <atomic>

#include "config.h"

namespace ble_link {
namespace {

// UUIDs are fixed for protocol v1; see docs/protocol.md.
constexpr char kServiceUuid[] = "5211c17e-1a40-45ab-9bb1-023f349e963f";
constexpr char kNavCharUuid[] = "993d9a6c-8219-4ced-a1f0-3c0984fd6723";
constexpr char kAckCharUuid[] = "15f299a0-a841-4116-9dbb-30b57dafe4ef";

QueueHandle_t queue = nullptr;
std::atomic<uint32_t> drop_count{0};

void push(const LinkEvent& ev) {
  if (xQueueSend(queue, &ev, 0) != pdTRUE) drop_count.fetch_add(1);
}

void pushKind(LinkEvent::Kind kind) {
  LinkEvent ev;
  ev.kind = kind;
  ev.len = 0;
  ev.rx_us = micros();
  push(ev);
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo&) override { pushKind(LinkEvent::Connected); }

  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
    pushKind(LinkEvent::Disconnected);
  }
};

class NavCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo&) override {
    LinkEvent ev;
    ev.rx_us = micros();
    const NimBLEAttValue& v = chr->getValue();
    if (v.size() > sizeof ev.data) {
      drop_count.fetch_add(1);
      return;
    }
    ev.kind = LinkEvent::Data;
    ev.len = static_cast<uint8_t>(v.size());
    memcpy(ev.data, v.data(), v.size());
    push(ev);
  }
};

ServerCallbacks server_callbacks;
NavCallbacks nav_callbacks;

}  // namespace

void begin() {
  queue = xQueueCreate(cfg::kLinkQueueDepth, sizeof(LinkEvent));

  NimBLEDevice::init(cfg::kBleName);
  NimBLEDevice::setMTU(cfg::kBleMtu);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(&server_callbacks);
  server->advertiseOnDisconnect(true);

  NimBLEService* service = server->createService(kServiceUuid);
  NimBLECharacteristic* nav = service->createCharacteristic(
      kNavCharUuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  nav->setCallbacks(&nav_callbacks);
  // Reserved for acks (protocol v1); created now so the GATT table is stable.
  service->createCharacteristic(kAckCharUuid, NIMBLE_PROPERTY::NOTIFY);

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(kServiceUuid);
  adv->setName(cfg::kBleName);
  adv->enableScanResponse(true);
  adv->start();
}

bool poll(LinkEvent& out) { return xQueueReceive(queue, &out, 0) == pdTRUE; }

uint32_t dropped() { return drop_count.load(); }

}  // namespace ble_link
