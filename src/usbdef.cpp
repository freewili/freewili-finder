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

auto Fw::is_freewili_hub(uint16_t vid, uint16_t pid) -> bool {
    return (vid == Fw::USB_VID_FW_HUB && pid == Fw::USB_PID_FW_HUB)
        || (vid == Fw::USB_VID_FW2_HUB && pid == Fw::USB_PID_FW2_HUB);
}
