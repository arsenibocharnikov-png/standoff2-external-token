#include "process_manager.h"
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <cstring>

process_manager::process_manager(const std::string& package_name)
    : package_name_(package_name), pid_(-1), libunity_base_(0), libil2cpp_base_(0) {}

bool process_manager::initialize() {
    pid_ = find_process_id();
    printf("[*] pid = %d\n", pid_);
    if (pid_ == -1) {
        return false;
    }

    libunity_base_ = find_module_base("libunity.so", 1);
    printf("[*] libunity = 0x%llx\n", (unsigned long long)libunity_base_);

    libil2cpp_base_ = find_module_base("libil2cpp.so", 1);
    printf("[*] libil2cpp = 0x%llx\n", (unsigned long long)libil2cpp_base_);

    return true;
}

int process_manager::get_pid() const {
    return pid_;
}

uint64_t process_manager::get_libunity_base() const {
    return libunity_base_;
}

uint64_t process_manager::get_libil2cpp_base() const {
    return libil2cpp_base_;
}

int process_manager::find_process_id() {
    int id = -1;
    DIR* dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        id = atoi(entry->d_name);
        if (id == 0) continue;

        char filename[64];
        sprintf(filename, "/proc/%d/cmdline", id);
        FILE* fp = fopen(filename, "r");
        if (!fp) continue;

        char cmdline[256] = {0};
        fread(cmdline, 1, sizeof(cmdline) - 1, fp);
        fclose(fp);

        if (strcmp(package_name_.c_str(), cmdline) == 0) {
            closedir(dir);
            return id;
        }
    }

    closedir(dir);
    return -1;
}

uint64_t process_manager::find_module_base(const char* module_name, int) {
    uint64_t base = 0;
    char path[64];
    sprintf(path, "/proc/%d/maps", pid_);
    FILE* maps = fopen(path, "rt");
    if (!maps) return 0;

    char line[1024];
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, module_name)) {
            uint64_t start = 0, end = 0;
            sscanf(line, "%lx-%lx", &start, &end);
            base = start;
            break;
        }
    }

    fclose(maps);
    return base;
}
