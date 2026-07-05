#include "memory/memory.h"
#include "process/process_manager.h"
#include <cstdint>
#include <cstdio>
#include <string>

std::string get_token(uint64_t il2cpp) {
    // GGEHEAEABCHCAGB_TypeInfo из script.json:
    // Address: 178334344
    uint64_t typeinfoOffset = 178334344;
    uint64_t typeinfo = memory_utils::read<uint64_t>(il2cpp + typeinfoOffset);
    if (!typeinfo) return "";

    // У IL2CPP указатель на static_fields обычно лежит по смещению 0xE8
    uint64_t staticfields = memory_utils::read<uint64_t>(typeinfo + 0xE8);
    if (!staticfields) return "";

    // В дампе у GGEHEAEABCHCAGB первое поле того же типа:
    // GGEHEAEABCHCAGB CFAAGAEAAEEGCEC; // 0x0
    // Используем его как singleton instance
    uint64_t authmanager = memory_utils::read<uint64_t>(staticfields + 0x0);
    if (!authmanager) return "";

    // По дампу у GGEHEAEABCHCAGB:
    // EHFGBCHBFHEHFAH`1<HBBEACABFGGCAEG> ... // 0xB0
    // Это самый вероятный state-holder
    uint64_t stateOffset = 0xB0;
    uint64_t state = memory_utils::read<uint64_t>(authmanager + stateOffset);
    if (!state) return "";

    // По дампу у AuthenticatedPlayerApiState:
    // string _token; // 0x28
    uint64_t tokenOffset = 0x28;
    uint64_t tokenPtr = memory_utils::read<uint64_t>(state + tokenOffset);
    if (!tokenPtr) return "";

    // IL2CPP string: длина по 0x10, данные UTF-16 с 0x14.
    // Если ваш memory_utils/read читает побайтно, лучше читать wchar/uint16_t.
    uint32_t len = memory_utils::read<uint32_t>(tokenPtr + 0x10);
    if (!len || len > 1024) return "";

    std::string token;
    token.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        uint16_t ch = memory_utils::read<uint16_t>(tokenPtr + 0x14 + i * 2);
        if (ch > 0x7F) {
            token.push_back('?');
        } else {
            token.push_back(static_cast<char>(ch));
        }
    }

    return token;
}

int main() {
    process_manager proc("com.axlebolt.standoff2");
    if (!proc.initialize()) return -1;

    memory_utils::initialize(proc.get_pid());

    // TypeInfo offset считается от базы libil2cpp, не libunity
    std::string token = get_token(proc.get_libil2cpp_base());
    if (token.empty()) return -1;

    printf("%s\n", token.c_str());
    return 0;
}
