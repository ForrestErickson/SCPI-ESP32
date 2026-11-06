#include <cassert>
#include <string>

#include "scpi/parser.h"

int main() {
    scpi::Parser parser;
    scpi::CommandRequest request;

    assert(parser.parse("MEASure:VOLTage 12.5", request) == scpi::ParseResult::Ok);
    assert(request.command_path == "MEASure:VOLTage");
    assert(!request.is_query);
    assert(request.parameter_text == "12.5");

    assert(parser.parse("  meas:volt?  ", request) == scpi::ParseResult::Ok);
    assert(request.command_path == "meas:volt");
    assert(request.is_query);
    assert(request.parameter_text.empty());

    assert(parser.parse("SOURce:VOLTage  3.3  ", request) == scpi::ParseResult::Ok);
    assert(request.parameter_text == "3.3");

    assert(parser.parse("   ", request) == scpi::ParseResult::EmptyMessage);
    assert(parser.parse("? 1", request) == scpi::ParseResult::MissingHeader);
    assert(parser.parse(":MEASure", request) == scpi::ParseResult::InvalidHeader);
    assert(parser.parse("MEASure::VOLTage", request) == scpi::ParseResult::InvalidHeader);
    assert(parser.parse("MEASure?x", request) == scpi::ParseResult::InvalidHeader);
    assert(parser.parse("MEASure:VOLTage;CURRent", request) ==
           scpi::ParseResult::MultipleCommandUnitsUnsupported);
    return 0;
}
