#include "memory/memory.h"
#include "process/process_manager.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

namespace offsets {
namespace il2cpp_string {
constexpr uint64_t length = 0x10;
constexpr uint64_t chars = 0x14;
constexpr uint32_t max_length = 1024;
}

namespace il2cpp_class {
constexpr uint64_t static_fields = 0xB8;
}

namespace auth_manager {
// dump.cs / script.json:
// GGEHEAEABCHCAGB::CFAAGAEAAEEGCEC        -> static instance
// GGEHEAEABCHCAGB::AHEBHCGGGCDEFFE        -> PlayerApiState at 0xB0
// AuthenticatedPlayerApiState::_token     -> field at 0x14 (0x28 from object)
constexpr uint64_t type_info_rva = 178334344; // 0x0AA12A88
constexpr uint64_t instance = 0x0;
constexpr uint64_t player_api_state = 0xB0;
}

namespace player_api_state {
constexpr uint64_t token = 0x28;
constexpr std::array<uint64_t, 5> unwrap_candidates = {
    0x10, 0x18, 0x20, 0x28, 0x30
};
}
} // namespace offsets

static std::string read_il2cpp_string(uint64_t str) {
    if (!str) {
        return "";
    }

    const uint32_t len =
        memory_utils::read<uint32_t>(str + offsets::il2cpp_string::length);
    if (!len || len > offsets::il2cpp_string::max_length) {
        return "";
    }

    std::string out;
    out.reserve(len);

    for (uint32_t i = 0; i < len; ++i) {
        const uint16_t ch = memory_utils::read<uint16_t>(
            str + offsets::il2cpp_string::chars + i * sizeof(uint16_t));
        out.push_back(ch <= 0x7F ? static_cast<char>(ch) : '?');
    }

    return out;
}

static bool is_valid_il2cpp_string(uint64_t str) {
    if (!str) {
        return false;
    }

    const uint32_t len =
        memory_utils::read<uint32_t>(str + offsets::il2cpp_string::length);
    return len > 0 && len <= offsets::il2cpp_string::max_length;
}

static uint64_t find_token_string(uint64_t state_holder) {
    if (!state_holder) {
        return 0;
    }

    uint64_t token_ptr =
        memory_utils::read<uint64_t>(state_holder + offsets::player_api_state::token);
    if (is_valid_il2cpp_string(token_ptr)) {
        return token_ptr;
    }

    for (const uint64_t unwrap : offsets::player_api_state::unwrap_candidates) {
        const uint64_t state = memory_utils::read<uint64_t>(state_holder + unwrap);
        std::printf("[*] stateHolder+0x%llx => 0x%llx\n",
                    static_cast<unsigned long long>(unwrap),
                    static_cast<unsigned long long>(state));

        if (!state) {
            continue;
        }

        token_ptr =
            memory_utils::read<uint64_t>(state + offsets::player_api_state::token);
        const uint32_t len = token_ptr
                                 ? memory_utils::read<uint32_t>(
                                       token_ptr + offsets::il2cpp_string::length)
                                 : 0;

        std::printf("[*] state=0x%llx tokenPtr=0x%llx len=%u\n",
                    static_cast<unsigned long long>(state),
                    static_cast<unsigned long long>(token_ptr),
                    len);

        if (is_valid_il2cpp_string(token_ptr)) {
            return token_ptr;
        }
    }

    return 0;
}

static std::string get_token(uint64_t base) {
    std::printf("[*] base: 0x%llx\n", static_cast<unsigned long long>(base));

    const uint64_t typeinfo = base + offsets::auth_manager::type_info_rva;
    std::printf("[*] typeinfo: 0x%llx\n", static_cast<unsigned long long>(typeinfo));

    const uint64_t staticfields =
        memory_utils::read<uint64_t>(typeinfo + offsets::il2cpp_class::static_fields);
    std::printf("[*] staticfields: 0x%llx\n",
                static_cast<unsigned long long>(staticfields));
    if (!staticfields) {
        return "";
    }

    const uint64_t authmanager =
        memory_utils::read<uint64_t>(staticfields + offsets::auth_manager::instance);
    std::printf("[*] authmanager: 0x%llx\n",
                static_cast<unsigned long long>(authmanager));
    if (!authmanager) {
        return "";
    }

    const uint64_t player_api_state =
        memory_utils::read<uint64_t>(authmanager + offsets::auth_manager::player_api_state);
    std::printf("[*] playerApiState: 0x%llx\n",
                static_cast<unsigned long long>(player_api_state));
    if (!player_api_state) {
        return "";
    }

    const uint64_t token_ptr = find_token_string(player_api_state);
    if (!token_ptr) {
        return "";
    }

    return read_il2cpp_string(token_ptr);
}

int main() {
    process_manager proc("com.axlebolt.standoff2");
    if (!proc.initialize()) {
        std::printf("[!] process_manager::initialize() failed\n");
        return -1;
    }

    memory_utils::initialize(proc.get_pid());

    const uint64_t base = proc.get_libunity_base();
    std::printf("[*] selected base = 0x%llx\n",
                static_cast<unsigned long long>(base));
    if (!base) {
        std::printf("[!] libunity base not found\n");
        return -1;
    }

    const std::string token = get_token(base);
    if (token.empty()) {
        std::printf("[!] token not found\n");
        return -1;
    }

    std::printf("[+] token: %s\n", token.c_str());
    return 0;
}
