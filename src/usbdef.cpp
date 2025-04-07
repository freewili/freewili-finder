#include "usbdef.hpp"

#include <cstdint>
#include <algorithm>

auto Fw::is_vid_pid_whitelisted(uint16_t vid, uint16_t pid) -> bool {
    //auto values = Fw::WhitelistVIDPID;
    if (auto vidIter = Fw::WhitelistVIDPID.find(vid); vidIter != Fw::WhitelistVIDPID.end()) {
        auto pids = vidIter->second;
        return std::find(pids.begin(), pids.end(), pid) != pids.end();
    }
    return false;
}
