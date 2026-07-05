#include "memory/memory.h"
#include "process/process_manager.h"
#include <cstdint>
#include <cstdio>
#include <string>

static std::string read_il2cpp_string(uint64_t str) {
    if (!str) return "";

    uint32_t len = memory_utils::read<uint32_t>(str + 0x10);
    if (!len || len > 1024) return "";

    std::string out;
    out.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        uint16_t ch = memory_utils::read<uint16_t>(str + 0x14 + i * 2);
        out.push_back(ch <= 0x7F ? static_cast<char>(ch) : '?');
    }

    return out;
}

static std::string get_token(uint64_t base) {
    printf("[*] base: 0x%llx\n", (unsigned long long)base);

    uint64_t typeinfoOffset = 151923536; // GGEHEAEABCHCAGB_TypeInfo
    uint64_t typeinfo = base + typeinfoOffset;
    printf("[*] typeinfo: 0x%llx\n", (unsigned long long)typeinfo);

    // il2cpp.h -> Il2CppClass::static_fields = 0xB8
    uint64_t staticfields = memory_utils::read<uint64_t>(typeinfo + 0xB8);
    printf("[*] staticfields: 0x%llx\n", (unsigned long long)staticfields);
    if (!staticfields) return "";

    // GGEHEAEABCHCAGB_StaticFields { GGEHEAEABCHCAGB_o* CFAAGAEAAEEGCEC; }
    uint64_t authmanager = memory_utils::read<uint64_t>(staticfields + 0x0);
    printf("[*] authmanager: 0x%llx\n", (unsigned long long)authmanager);
    if (!authmanager) return "";

    // dump (2).cs: PlayerApiState-like holder at 0xB0
    uint64_t stateHolder = memory_utils::read<uint64_t>(authmanager + 0xB0);
    printf("[*] stateHolder: 0x%llx\n", (unsigned long long)stateHolder);
    if (!stateHolder) return "";

    // direct try
    uint64_t tokenPtr = memory_utils::read<uint64_t>(stateHolder + 0x28);
    uint32_t len = tokenPtr ? memory_utils::read<uint32_t>(tokenPtr + 0x10) : 0;
    printf("[*] direct tokenPtr: 0x%llx len=%u\n", (unsigned long long)tokenPtr, len);

    if (tokenPtr && len > 0 && len <= 1024) {
        return read_il2cpp_string(tokenPtr);
    }

    // unwrap wrapper/container if needed
    const uint64_t unwrapOffsets[] = {0x10, 0x18, 0x20, 0x28, 0x30};

    for (uint64_t unwrap : unwrapOffsets) {
        uint64_t state = memory_utils::read<uint64_t>(stateHolder + unwrap);
        printf("[*] stateHolder+0x%llx => 0x%llx\n",
               (unsigned long long)unwrap,
               (unsigned long long)state);

        if (!state) continue;

        tokenPtr = memory_utils::read<uint64_t>(state + 0x28);
        len = tokenPtr ? memory_utils::read<uint32_t>(tokenPtr + 0x10) : 0;

        printf("[*] state=0x%llx tokenPtr=0x%llx len=%u\n",
               (unsigned long long)state,
               (unsigned long long)tokenPtr,
               len);

        if (tokenPtr && len > 0 && len <= 1024) {
            return read_il2cpp_string(tokenPtr);
        }
    }

    return "";
}

int main() {
    process_manager proc("com.axlebolt.standoff2");
    if (!proc.initialize()) {
        printf("[!] process_manager::initialize() failed\n");
        return -1;
    }

    memory_utils::initialize(proc.get_pid());

    uint64_t base = proc.get_libunity_base();
    printf("[*] selected base = 0x%llx\n", (unsigned long long)base);
    if (!base) {
        printf("[!] libunity base not found\n");
        return -1;
    }

    std::string token = get_token(base);
    if (token.empty()) {
        printf("[!] token not found\n");
        return -1;
    }

    printf("[+] token: %s\n", token.c_str());
    return 0;
}
