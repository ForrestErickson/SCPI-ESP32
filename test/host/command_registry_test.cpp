#ifndef SCPI_ESP32_COMMAND_REGISTRY_H
#define SCPI_ESP32_COMMAND_REGISTRY_H

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "scpi/error_queue.h"
#include "scpi/response_sink.h"

namespace scpi {

// The parser will populate this structure after recognizing one command unit.
// parameter_text is the unmodified text following the command header; detailed
// parameter parsing and validation remain separate responsibilities.
struct CommandRequest {
    std::string command_path;
    bool is_query;
    std::string parameter_text;

    CommandRequest()
        : is_query(false) {}

    CommandRequest(const std::string& path, bool query,
                   const std::string& parameters)
        : command_path(path), is_query(query), parameter_text(parameters) {}
};

// Handlers are deliberately void: a handler reports processing errors through
// ErrorQueue and sends query output through ResponseSink.
typedef void (*CommandHandler)(void* user_context,
                               const CommandRequest& request,
                               ErrorQueue& errors,
                               ResponseSink& responses);

struct CommandEntry {
    std::string command_path;
    CommandHandler handler;
    void* user_context;
};

class CommandRegistry {
public:
    // Registers an exact command path. Matching is case-insensitive.
    // Returns false for an empty path, null handler, or duplicate path.
    bool registerCommand(const std::string& command_path,
                         CommandHandler handler,
                         void* user_context = 0) {
        if (command_path.empty() || handler == 0 ||
            find(command_path) != 0) {
            return false;
        }

        CommandEntry entry;
        entry.command_path = command_path;
        entry.handler = handler;
        entry.user_context = user_context;
        entries_.push_back(entry);
        return true;
    }

    // Returns the registered entry or null when no exact path is registered.
    // SCPI short/long keyword abbreviation rules are not implemented here;
    // the parser/standards work must resolve those before dispatch.
    const CommandEntry* find(const std::string& command_path) const {
        for (std::vector<CommandEntry>::const_iterator it = entries_.begin();
             it != entries_.end(); ++it) {
            if (equalsIgnoreCase(it->command_path, command_path)) {
                return &(*it);
            }
        }
        return 0;
    }

    // Dispatches a previously parsed request. Returns false if no command
    // path is registered; otherwise invokes the handler and returns true.
    bool dispatch(const CommandRequest& request,
                  ErrorQueue& errors,
                  ResponseSink& responses) const {
        const CommandEntry* entry = find(request.command_path);
        if (entry == 0) {
            return false;
        }
        entry->handler(entry->user_context, request, errors, responses);
        return true;
    }

    std::size_t size() const {
        return entries_.size();
    }

private:
    static bool equalsIgnoreCase(const std::string& left,
                                 const std::string& right) {
        if (left.size() != right.size()) {
            return false;
        }
        for (std::size_t i = 0; i < left.size(); ++i) {
            const unsigned char a = static_cast<unsigned char>(left[i]);
            const unsigned char b = static_cast<unsigned char>(right[i]);
            if (std::tolower(a) != std::tolower(b)) {
                return false;
            }
        }
        return true;
    }

    std::vector<CommandEntry> entries_;
};

} // namespace scpi

#endif // SCPI_ESP32_COMMAND_REGISTRY_H
