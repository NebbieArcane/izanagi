#include "nebbie/special_proc_catalog.hpp"

#include <cctype>

namespace nebbie {

namespace {

std::string to_lower_ascii(std::string text) {
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

bool names_equal_insensitive(const std::string& left, const std::string& right) {
    return to_lower_ascii(left) == to_lower_ascii(right);
}

bool list_contains_insensitive(const std::vector<std::string>& names, const std::string& procedure) {
    for (const auto& name : names) {
        if (names_equal_insensitive(name, procedure)) {
            return true;
        }
    }
    return false;
}

} // namespace

bool special_proc_names_equal(const std::string& left, const std::string& right) {
    return names_equal_insensitive(left, right);
}

const std::vector<std::string>& special_proc_names_for_type(const char type) {
    switch (std::tolower(static_cast<unsigned char>(type))) {
    case 'r':
        return room_special_proc_names();
    case 'm':
    case 'o':
    default:
        return mob_object_special_proc_names();
    }
}

bool is_known_special_proc(const char type, const std::string& procedure) {
    if (procedure.empty()) {
        return false;
    }
    return list_contains_insensitive(special_proc_names_for_type(type), procedure);
}

} // namespace nebbie
