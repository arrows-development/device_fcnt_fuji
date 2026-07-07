/*
 * SPDX-FileCopyrightText: 2025-2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MemtrackDevicePvr.h"

#include <android-base/logging.h>

#include <cctype>
#include <fstream>
#include <sstream>

namespace aidl {
namespace android {
namespace hardware {
namespace memtrack {

bool MemtrackDevicePvr::getMemory(int pid, MemtrackRecord& record) {
    std::ifstream ifs(mPath);
    std::string line, token;
    unsigned int client_pid;

    if (!ifs.is_open()) {
        return false;
    }

    // First column is the client pid, followed by the memory sizes
    // for different types, so sum them up.
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);

        if (!std::getline(iss, token, ',')) continue;

        if (token.empty() || !std::isdigit(token[0])) {
            continue;
        }

        client_pid = std::stoi(token);

        if (client_pid == pid || pid == 0) {
            while (std::getline(iss, token, ',')) {
                if (token.empty()) continue;
                LOG(DEBUG) << "Accounting memory allocated by PID " << pid << ": "
                           << std::stoul(token);
                record.sizeInBytes += std::stoul(token);
            }
        }
    }

    LOG(DEBUG) << "Total memory allocated by PID " << pid << ": " << record.sizeInBytes;

    record.flags = MemtrackRecord::FLAG_SMAPS_UNACCOUNTED | MemtrackRecord::FLAG_PRIVATE |
                   MemtrackRecord::FLAG_NONSECURE;
    return true;
}

}  // namespace memtrack
}  // namespace hardware
}  // namespace android
}  // namespace aidl
