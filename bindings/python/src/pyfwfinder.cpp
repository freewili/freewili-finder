#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/vector.h>

#include <fwfinder.hpp>

#include <stdexcept>
#include <string>
#include <optional>
#include <cstdint>
#include <iostream>

namespace nb = nanobind;

auto find_all() -> Fw::FreeWiliDevices {
    if (auto devicesResult = Fw::find_all(); !devicesResult.has_value()) {
        PyErr_SetString(PyExc_RuntimeError, devicesResult.error().c_str());
        throw nb::python_error();
    } else {
        return devicesResult.value();
    }
}

NB_MODULE(pyfwfinder, m) {
    nb::enum_<Fw::USBDeviceType>(m, "USBDeviceType")
        .value("Hub", Fw::USBDeviceType::Hub)
        .value("Serial", Fw::USBDeviceType::Serial)
        .value("SerialMain", Fw::USBDeviceType::SerialMain)
        .value("SerialDisplay", Fw::USBDeviceType::SerialDisplay)
        .value("MassStorage", Fw::USBDeviceType::MassStorage)
        .value("ESP32", Fw::USBDeviceType::ESP32)
        .value("FTDI", Fw::USBDeviceType::FTDI)
        .value("Other", Fw::USBDeviceType::Other)
        .value("_MaxValue", Fw::USBDeviceType::_MaxValue)
        .export_values();

    nb::enum_<Fw::DeviceType>(m, "DeviceType")
        .value("Unknown", Fw::DeviceType::Unknown)
        .value("FreeWili", Fw::DeviceType::FreeWili)
        .value("DefCon2024Badge", Fw::DeviceType::DefCon2024Badge)
        .value("DefCon2025FwBadge", Fw::DeviceType::DefCon2025FwBadge)
        .value("Winky", Fw::DeviceType::Winky)
        .value("UF2", Fw::DeviceType::UF2)
        .export_values();

    nb::class_<Fw::USBDevice>(m, "USBDevice")
        .def(nb::init<>())
        .def(
            "__str__",
            [](const Fw::USBDevice& self) {
                return Fw::getUSBDeviceTypeName(self.kind) + " " + self.name + " " + self.serial;
            }
        )
        .def_ro("kind", &Fw::USBDevice::kind)
        .def_ro("vid", &Fw::USBDevice::vid)
        .def_ro("pid", &Fw::USBDevice::pid)
        .def_ro("name", &Fw::USBDevice::name)
        .def_ro("serial", &Fw::USBDevice::serial)
        .def_ro("location", &Fw::USBDevice::location)
        .def_ro("paths", &Fw::USBDevice::paths)
        .def_ro("port", &Fw::USBDevice::port)
        .def_ro("_raw", &Fw::USBDevice::_raw);

    nb::class_<Fw::FreeWiliDevice>(m, "FreeWiliDevice")
        .def(nb::init<>())
        .def(
            "__str__",
            [](const Fw::FreeWiliDevice& self) { return self.name + " " + self.serial; }
        )
        .def_ro("device_type", &Fw::FreeWiliDevice::deviceType)
        .def_ro("name", &Fw::FreeWiliDevice::name)
        .def_ro("serial", &Fw::FreeWiliDevice::serial)
        .def_ro("usb_devices", &Fw::FreeWiliDevice::usbDevices)
        .def("get_usb_devices", &Fw::FreeWiliDevice::getUSBDevices);

    m.def("find_all", &find_all);
}
