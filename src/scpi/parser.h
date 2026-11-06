#ifndef SCPI_ESP32_SCPI_PARSER_H
#define SCPI_ESP32_SCPI_PARSER_H

#include <cctype>
#include <string>

#include "scpi/command_registry.h"

namespace scpi {

// Initial parser scope: one subsystem command unit per program message.
// Common commands (*IDN?, etc.) and semicolon-separated units are not yet
// supported. Parameter grammar is intentionally left to command handlers.
enum class ParseResult {
    Ok,
    EmptyMessage,
    MissingHeader,
    InvalidHeader,
    MultipleCommandUnitsUnsupported
};

class Parser {
public:
    ParseResult parse(const std::string& message,
                      CommandRequest& request) const {
        request = CommandRequest();

        std::string input = trim(message);
        if (input.empty()) {
            return ParseResult::EmptyMessage;
        }

        if (input.find(';') != std::string::npos) {
            return ParseResult::MultipleCommandUnitsUnsupported;
        }

        std::string::size_type split = input.find_first_of(" \t\r\n");
        std::string header = input.substr(0, split);
        std::string parameters =
            split == std::string::npos ? "" : trim(input.substr(split));

        bool query = false;
        std::string::size_type question = header.find('?');
        if (question != std::string::npos) {
            if (question != header.size() - 1 ||
                header.find('?', question + 1) != std::string::npos) {
                return ParseResult::InvalidHeader;
            }
            query = true;
            header.erase(question);
        }

        if (header.empty()) {
            return ParseResult::MissingHeader;
        }

        if (!validHeader(header)) {
            return ParseResult::InvalidHeader;
        }

        request = CommandRequest(header, query, parameters);
        return ParseResult::Ok;
    }

private:
    static std::string trim(const std::string& value) {
        std::string::size_type first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        std::string::size_type last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    static bool validHeader(const std::string& header) {
        if (header[0] == ':' || header[header.size() - 1] == ':') {
            return false;
        }

        bool segment_has_character = false;
        for (std::string::size_type i = 0; i < header.size(); ++i) {
            const unsigned char ch = static_cast<unsigned char>(header[i]);
            if (ch == ':') {
                if (!segment_has_character) return false;
                segment_has_character = false;
            } else if (std::isalnum(ch)) {
                segment_has_character = true;
            } else {
                return false;
            }
        }
        return segment_has_character;
    }
};

} // namespace scpi

#endif // SCPI_ESP32_SCPI_PARSER_H
