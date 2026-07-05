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
