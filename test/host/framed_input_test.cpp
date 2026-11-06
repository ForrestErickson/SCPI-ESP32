#include "transport/framed_input.h"
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

class RecordingEngine : public scpi::EngineInterface {
public:
    void processScpiMessage(const std::string& message) override {
        messages.push_back(message);
    }
    std::vector<std::string> messages;
};

int main() {
    RecordingEngine engine;
    scpi::transport::FramedInput input(
        engine, scpi::transport::MessageFramer::Config(64, 25));

    // Fragmented CRLF-terminated input is delivered once, only when complete.
    input.feed('S', 1);
    input.feed('Y', 2);
    input.feed('S', 3);
    input.feed('\r', 4);
    assert(engine.messages.empty());
    input.feed('\n', 5);
    assert(engine.messages.size() == 1);
    assert(engine.messages[0] == "SYS");

    // Back-to-back LF messages are delivered independently.
    input.feed('A', 6);
    input.feed('\n', 7);
    input.feed('B', 8);
    input.feed('\n', 9);
    assert(engine.messages.size() == 3);
    assert(engine.messages[1] == "A");
    assert(engine.messages[2] == "B");

    // CR alone is not a terminator; timeout can still complete the message.
    input.feed('C', 10);
    input.feed('\r', 11);
    assert(input.poll(36).type == scpi::transport::MessageFramer::MESSAGE_READY);
    assert(engine.messages.size() == 4);
    assert(engine.messages[3] == "C\r");

    // Empty messages and overflow never reach the engine.
    input.feed('\n', 37);
    assert(engine.messages.size() == 4);

    std::cout << "FramedInput tests passed\n";
    return 0;
}
