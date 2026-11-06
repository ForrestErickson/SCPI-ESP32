#include "transport/message_framer.h"
#include <assert.h>
#include <stdint.h>
#include <iostream>

using scpi::transport::MessageFramer;

static void expectMessage(const MessageFramer::Result& r, const char* expected,
                          MessageFramer::Terminator terminator) {
    assert(r.type == MessageFramer::MESSAGE_READY);
    assert(r.message == expected);
    assert(r.terminator == terminator);
}

int main() {
    // LF terminates; empty and repeated terminators do not dispatch.
    MessageFramer lf;
    assert(lf.feed('A', 1).type == MessageFramer::NO_EVENT);
    expectMessage(lf.feed('\n', 2), "A", MessageFramer::TERMINATED_BY_LF);
    assert(lf.feed('\n', 3).type == MessageFramer::NO_EVENT);

    // CR-LF is one boundary, including when the pair spans feed calls.
    MessageFramer crlf;
    crlf.feed('B', 10);
    assert(crlf.feed('\r', 11).type == MessageFramer::NO_EVENT);
    expectMessage(crlf.feed('\n', 12), "B", MessageFramer::TERMINATED_BY_CRLF);

    // A CR not followed by LF is data, not a message terminator.
    crlf.feed('C', 13);
    crlf.feed('\r', 14);
    assert(crlf.poll(14).type == MessageFramer::NO_EVENT);
    expectMessage(crlf.feed('\n', 15), "C\r", MessageFramer::TERMINATED_BY_LF);

    // A non-LF byte after CR remains in the same message.
    crlf.feed('D', 16);
    crlf.feed('\r', 17);
    crlf.feed('E', 18);
    expectMessage(crlf.feed('\n', 19), "D\rE", MessageFramer::TERMINATED_BY_LF);

    // Inactivity timeout and timer rollover.
    MessageFramer timed(MessageFramer::Config(32, 20));
    timed.feed('T', 100);
    assert(timed.poll(119).type == MessageFramer::NO_EVENT);
    expectMessage(timed.poll(120), "T", MessageFramer::TERMINATED_BY_TIMEOUT);
    timed.feed('R', UINT32_MAX - 5);
    assert(timed.poll(10).type == MessageFramer::NO_EVENT); // 16 ms elapsed
    expectMessage(timed.poll(14), "R", MessageFramer::TERMINATED_BY_TIMEOUT);

    // Timeout restarts on each byte; disabled timeout leaves data buffered.
    timed.feed('X', 200);
    timed.feed('Y', 215);
    assert(timed.poll(230).type == MessageFramer::NO_EVENT);
    expectMessage(timed.poll(235), "XY", MessageFramer::TERMINATED_BY_TIMEOUT);
    MessageFramer no_timeout(MessageFramer::Config(32, 0));
    no_timeout.feed('N', 1);
    assert(no_timeout.poll(1000).type == MessageFramer::NO_EVENT);

    // Overflow is reported once; bytes are discarded through the next boundary.
    MessageFramer bounded(MessageFramer::Config(3, 0));
    bounded.feed('1', 1); bounded.feed('2', 2); bounded.feed('3', 3);
    assert(bounded.feed('4', 4).type == MessageFramer::MESSAGE_OVERFLOW);
    assert(bounded.feed('5', 5).type == MessageFramer::NO_EVENT);
    assert(bounded.feed('\n', 6).type == MessageFramer::NO_EVENT);
    bounded.feed('O', 7);
    expectMessage(bounded.feed('\n', 8), "O", MessageFramer::TERMINATED_BY_LF);

    std::cout << "MessageFramer tests passed\n";
    return 0;
}
