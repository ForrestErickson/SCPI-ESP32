/*
 * SCPI-ESP32 initial ESP32 Arduino integration example.
 *
 * Target: ESP32 Arduino core; default onboard LED GPIO 2, active high.
 * Serial SCPI transport: USB/UART at 115200 baud, CR, LF, or CRLF terminated; 1 s inactivity timeout.
 * The generic SCPI engine and HEARTBEAT service remain hardware independent.
 */

#include <Arduino.h>
#include <WiFi.h>

#include <string>

#include "scpi/engine.h"
#include "scpi/heartbeat_commands.h"
#include "scpi/identification.h"
#include "transport/framed_input.h"

#ifndef SCPI_STATUS_LED_PIN
#define SCPI_STATUS_LED_PIN 2
#endif

#ifndef SCPI_STATUS_LED_ACTIVE_HIGH
#define SCPI_STATUS_LED_ACTIVE_HIGH 1
#endif

#ifndef SCPI_INPUT_TIMEOUT_MS
#define SCPI_INPUT_TIMEOUT_MS 1000
#endif

#ifndef SCPI_FIRMWARE_REVISION
#define SCPI_FIRMWARE_REVISION "0.1.0"
#endif

class Esp32LedOutput : public services::LogicalLedOutput {
public:
    Esp32LedOutput() : initialized_(false) {}

    void begin(bool setup_indicator_on) {
        pinMode(SCPI_STATUS_LED_PIN, OUTPUT);
        initialized_ = true;
        setLedOn(setup_indicator_on);
    }

    void setLedOn(bool on) {
        if (!initialized_) return;
#if SCPI_STATUS_LED_ACTIVE_HIGH
        digitalWrite(SCPI_STATUS_LED_PIN, on ? HIGH : LOW);
#else
        digitalWrite(SCPI_STATUS_LED_PIN, on ? LOW : HIGH);
#endif
    }

private:
    bool initialized_;
};

class SerialResponseSink : public scpi::ResponseSink {
public:
    void writeResponse(const std::string& response) {
        Serial.println(response.c_str());
    }
};

static Esp32LedOutput led_output;
static services::Heartbeat heartbeat(led_output);

static uint32_t monotonicMilliseconds(void*) {
    return static_cast<uint32_t>(millis());
}

static scpi::CommandRegistry command_registry;
static scpi::ErrorQueue error_queue;
static SerialResponseSink response_sink;
static scpi::Engine scpi_engine(command_registry, error_queue, response_sink);
static scpi::HeartbeatCommands heartbeat_commands(
    heartbeat, monotonicMilliseconds);
static scpi::transport::FramedInput framed_input(
    scpi_engine, scpi::transport::MessageFramer::Config(256, SCPI_INPUT_TIMEOUT_MS));

void setup() {
    // The status LED is the setup-running indicator. The heartbeat service's
    // constructor cannot touch the GPIO before begin() initializes the pin.
    led_output.begin(true);

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    // Read the ESP32 station MAC and use it as the instrument serial number.
    // WiFi is placed in station mode but is not asked to join an access point.
    const String mac_address = WiFi.macAddress();
    static scpi::Identification identification(
        "! Amused Scientist",
        "SCPI-ESP32",
        std::string(mac_address.c_str()),
        SCPI_FIRMWARE_REVISION);

    const bool registration_ok =
        identification.registerWith(command_registry) &&
        heartbeat_commands.registerWith(command_registry);

    if (!registration_ok) {
        Serial.println("# ERROR: SCPI command registration failed");
    }

    // Setup is complete. HEARTBEAT itself starts on the first loop() call.
    led_output.setLedOn(false);
}

void loop() {
    static bool heartbeat_started = false;
    const uint32_t now = static_cast<uint32_t>(millis());

    if (!heartbeat_started) {
        heartbeat.setEnabled(true, now);
        heartbeat_started = true;
    }

    while (Serial.available() > 0) {
        const int incoming = Serial.read();
        if (incoming >= 0) {
            const scpi::transport::MessageFramer::Result result =
                framed_input.feed(static_cast<uint8_t>(incoming),
                                  static_cast<uint32_t>(millis()));
            if (result.type ==
                scpi::transport::MessageFramer::MESSAGE_OVERFLOW) {
                Serial.println("# ERROR: SCPI input message too long");
            }
        }
    }

    framed_input.poll(static_cast<uint32_t>(millis()));
    heartbeat.service(static_cast<uint32_t>(millis()));
}
