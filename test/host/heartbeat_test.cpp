#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "services/heartbeat.h"

class FakeLed : public services::LogicalLedOutput {
public:
    FakeLed() : on(false) {}
    bool on;
    std::vector<bool> changes;

    void setLedOn(bool value) {
        on = value;
        changes.push_back(value);
    }
};

int main() {
    FakeLed led;
    services::Heartbeat heartbeat(led);

    assert(!heartbeat.enabled());
    assert(!led.on);
    assert(heartbeat.periodMilliseconds() == 2000);
    assert(heartbeat.dutyCyclePercent() == 25);

    // Enabling starts a fresh 2 s cycle: 500 ms ON, then 1500 ms OFF.
    heartbeat.setEnabled(true, 1000);
    assert(led.on);
    heartbeat.service(1499);
    assert(led.on);
    heartbeat.service(1500);
    assert(!led.on);
    heartbeat.service(2999);
    assert(!led.on);
    heartbeat.service(3000);
    assert(led.on);

    // A period update is configured for the next enable, not the active run.
    assert(heartbeat.setPeriodMilliseconds(4000));
    heartbeat.service(3500); // active period remains 2000 ms
    assert(!led.on);
    heartbeat.setEnabled(false, 3600);
    assert(!led.on);
    heartbeat.setEnabled(true, 5000);
    assert(led.on);
    heartbeat.service(6000); // 25% of 4000 ms = 1000 ms ON
    assert(!led.on);

    // Duty change while enabled immediately restarts the active cycle.
    assert(heartbeat.setDutyCyclePercent(50, 7000));
    assert(led.on);
    heartbeat.service(8999);
    assert(led.on);
    heartbeat.service(9000);
    assert(!led.on);

    // Zero and 100 percent are steady states while enabled.
    assert(heartbeat.setDutyCyclePercent(0, 9100));
    assert(!led.on);
    heartbeat.service(20000);
    assert(!led.on);
    assert(heartbeat.setDutyCyclePercent(100, 21000));
    assert(led.on);
    heartbeat.service(50000);
    assert(led.on);

    // Invalid settings are rejected without changing the prior configuration.
    assert(!heartbeat.setPeriodMilliseconds(0));
    assert(!heartbeat.setPeriodMilliseconds(86400001));
    assert(heartbeat.periodMilliseconds() == 4000);
    assert(!heartbeat.setDutyCyclePercent(101, 51000));
    assert(heartbeat.dutyCyclePercent() == 100);

    // Disabling is immediate, including in the middle of an ON interval.
    heartbeat.setEnabled(false, 52000);
    assert(!led.on);

    // Unsigned elapsed-time arithmetic works across uint32_t rollover.
    assert(heartbeat.setPeriodMilliseconds(2000));
    assert(heartbeat.setDutyCyclePercent(25, 0));
    heartbeat.setEnabled(true, UINT32_MAX - 249U);
    assert(led.on);
    heartbeat.service(249U); // 499 ms elapsed
    assert(led.on);
    heartbeat.service(250U); // 500 ms elapsed
    assert(!led.on);

    // Calling service repeatedly without time advancing causes no extra writes.
    const size_t change_count = led.changes.size();
    heartbeat.service(250U);
    assert(led.changes.size() == change_count);

    std::cout << "Heartbeat tests passed\n";
    return 0;
}
