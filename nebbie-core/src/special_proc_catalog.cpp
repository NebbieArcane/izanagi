#include "nebbie/special_proc_catalog.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>

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

void merge_unique_insensitive(std::vector<std::string>& dest, const std::vector<std::string>& src) {
    for (const auto& name : src) {
        if (name.empty()) {
            continue;
        }
        if (!list_contains_insensitive(dest, name)) {
            dest.push_back(name);
        }
    }
    std::sort(dest.begin(), dest.end(), [](const std::string& a, const std::string& b) {
        return to_lower_ascii(a) < to_lower_ascii(b);
    });
}

struct CatalogState {
    SpecialProcCatalogConfig config;
    std::vector<std::string> server_mob_object;
    std::vector<std::string> server_room;
    std::vector<std::string> extras;
    std::vector<std::string> effective_mob_object;
    std::vector<std::string> effective_room;
    bool server_loaded = false;
    bool ready = false;
    std::string server_detail;
};

CatalogState& state() {
    static CatalogState instance;
    return instance;
}

std::vector<std::string> read_cpp_names(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return parse_special_proc_names_from_cpp(buffer.str());
}

void rebuild_effective() {
    CatalogState& st = state();
    st.effective_mob_object = embedded_mob_object_special_proc_names();
    st.effective_room = embedded_room_special_proc_names();
    merge_unique_insensitive(st.effective_mob_object, st.server_mob_object);
    merge_unique_insensitive(st.effective_room, st.server_room);
    // Allowlist names are accepted for every type (WIP / local procs).
    merge_unique_insensitive(st.effective_mob_object, st.extras);
    merge_unique_insensitive(st.effective_room, st.extras);
    st.ready = true;
}

void ensure_ready() {
    if (!state().ready) {
        rebuild_effective();
    }
}

void reload_server_tables() {
    CatalogState& st = state();
    st.server_mob_object.clear();
    st.server_room.clear();
    st.server_loaded = false;
    st.server_detail.clear();

    if (st.config.server_root.empty()) {
        st.server_detail = "server root non impostato";
        return;
    }

    const std::filesystem::path src = st.config.server_root / "src";
    const std::filesystem::path other = src / "specialproc_other.cpp";
    const std::filesystem::path room = src / "specialproc_room.cpp";

    std::error_code ec;
    if (!std::filesystem::is_regular_file(other, ec) || !std::filesystem::is_regular_file(room, ec)) {
        st.server_detail = "file specialproc_*.cpp non trovati sotto "
                           + (src.lexically_normal().string());
        return;
    }

    st.server_mob_object = read_cpp_names(other);
    st.server_room = read_cpp_names(room);
    if (st.server_mob_object.empty() && st.server_room.empty()) {
        st.server_detail = "nessun nome estratto da specialproc_*.cpp";
        return;
    }

    st.server_loaded = true;
    st.server_detail = "caricati da " + src.lexically_normal().string();
}

} // namespace

std::vector<std::string> parse_special_proc_names_from_cpp(const std::string& text) {
    std::set<std::string> names;
    std::size_t i = 0;
    while (i < text.size()) {
        const auto brace = text.find('{', i);
        if (brace == std::string::npos) {
            break;
        }
        std::size_t j = brace + 1;
        while (j < text.size()
               && (text[j] == ' ' || text[j] == '\t' || text[j] == '\n' || text[j] == '\r')) {
            ++j;
        }
        if (j >= text.size() || text[j] != '"') {
            i = brace + 1;
            continue;
        }
        ++j;
        std::string name;
        while (j < text.size() && text[j] != '"') {
            name.push_back(text[j]);
            ++j;
        }
        if (j >= text.size()) {
            break;
        }
        ++j;
        while (j < text.size()
               && (text[j] == ' ' || text[j] == '\t' || text[j] == '\n' || text[j] == '\r')) {
            ++j;
        }
        if (j < text.size() && text[j] == ',' && !name.empty()) {
            names.insert(name);
        }
        i = j;
    }
    return {names.begin(), names.end()};
}

std::filesystem::path default_nebbie_server_root() {
    if (const char* env = std::getenv("NEBBIE_SERVER_ROOT")) {
        if (env[0] != '\0') {
            return std::filesystem::path(env);
        }
    }
    const char* home = std::getenv("HOME");
    if (home != nullptr && home[0] != '\0') {
        return std::filesystem::path(home) / "NebbieArcane" / "Server";
    }
    return {};
}

bool special_proc_names_equal(const std::string& left, const std::string& right) {
    return names_equal_insensitive(left, right);
}

const std::vector<std::string>& mob_object_special_proc_names() {
    ensure_ready();
    return state().effective_mob_object;
}

const std::vector<std::string>& room_special_proc_names() {
    ensure_ready();
    return state().effective_room;
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

SpecialProcCatalogConfig special_proc_catalog_config() {
    return state().config;
}

SpecialProcCatalogStatus special_proc_catalog_status() {
    // Ensure effective lists exist.
    (void)mob_object_special_proc_names();
    (void)room_special_proc_names();
    const CatalogState& st = state();
    SpecialProcCatalogStatus status;
    status.server_loaded = st.server_loaded;
    status.server_mob_object_count = st.server_mob_object.size();
    status.server_room_count = st.server_room.size();
    status.extra_count = st.extras.size();
    status.effective_mob_object_count = st.effective_mob_object.size();
    status.effective_room_count = st.effective_room.size();
    status.server_detail = st.server_detail;
    return status;
}

void configure_special_proc_catalog(SpecialProcCatalogConfig config) {
    CatalogState& st = state();
    st.config = std::move(config);
    reload_server_tables();
    if (!st.config.extra_allowlist_path.empty()) {
        load_extra_special_procs(st.config.extra_allowlist_path);
    } else {
        st.extras.clear();
        rebuild_effective();
    }
}

std::vector<std::string> extra_special_proc_names() {
    return state().extras;
}

void set_extra_special_proc_names(std::vector<std::string> names) {
    CatalogState& st = state();
    std::vector<std::string> cleaned;
    for (auto& name : names) {
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.front()))) {
            name.erase(name.begin());
        }
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) {
            name.pop_back();
        }
        if (name.empty() || name[0] == '#') {
            continue;
        }
        if (!list_contains_insensitive(cleaned, name)) {
            cleaned.push_back(std::move(name));
        }
    }
    std::sort(cleaned.begin(), cleaned.end(), [](const std::string& a, const std::string& b) {
        return to_lower_ascii(a) < to_lower_ascii(b);
    });
    st.extras = std::move(cleaned);
    rebuild_effective();
}

bool load_extra_special_procs(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        state().extras.clear();
        rebuild_effective();
        return false;
    }
    std::vector<std::string> names;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        std::size_t start = 0;
        while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
            ++start;
        }
        if (start >= line.size() || line[start] == '#') {
            continue;
        }
        std::size_t end = line.size();
        while (end > start && std::isspace(static_cast<unsigned char>(line[end - 1]))) {
            --end;
        }
        names.emplace_back(line.substr(start, end - start));
    }
    set_extra_special_proc_names(std::move(names));
    return true;
}

bool probe_server_special_proc_tables(const std::filesystem::path& server_root,
                                      std::size_t* mob_object_count,
                                      std::size_t* room_count,
                                      std::string* detail) {
    if (mob_object_count) {
        *mob_object_count = 0;
    }
    if (room_count) {
        *room_count = 0;
    }
    if (server_root.empty()) {
        if (detail) {
            *detail = "server root non impostato";
        }
        return false;
    }
    const std::filesystem::path src = server_root / "src";
    const std::filesystem::path other = src / "specialproc_other.cpp";
    const std::filesystem::path room = src / "specialproc_room.cpp";
    std::error_code ec;
    if (!std::filesystem::is_regular_file(other, ec) || !std::filesystem::is_regular_file(room, ec)) {
        if (detail) {
            *detail = "file specialproc_*.cpp non trovati sotto " + src.lexically_normal().string();
        }
        return false;
    }
    const auto mob_names = read_cpp_names(other);
    const auto room_names = read_cpp_names(room);
    if (mob_object_count) {
        *mob_object_count = mob_names.size();
    }
    if (room_count) {
        *room_count = room_names.size();
    }
    if (detail) {
        *detail = "caricabili da " + src.lexically_normal().string();
    }
    return !mob_names.empty() || !room_names.empty();
}

bool save_extra_special_procs(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }
    std::error_code ec;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out << "# Extra special proc names accepted by Izanagi validation\n";
    out << "# One name per line. Edited from Strumenti -> Catalogo special proc.\n";
    for (const auto& name : state().extras) {
        out << name << '\n';
    }
    return static_cast<bool>(out);
}

} // namespace nebbie
