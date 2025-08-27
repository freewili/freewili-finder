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
        .value("DEFCON2024Badge", Fw::DeviceType::DEFCON2024Badge)
        .value("DEFCON2025FwBadge", Fw::DeviceType::DEFCON2025FwBadge)
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
        .def(
            "__eq__",
            [](const Fw::USBDevice& self, const Fw::USBDevice& other) { return self == other; }
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
        .def(
            "__str__",
            [](const Fw::FreeWiliDevice& self) { return self.name + " " + self.serial; }
        )
        .def(
            "__eq__",
            [](const Fw::FreeWiliDevice& self, const Fw::FreeWiliDevice& other) {
                return self == other;
            }
        )
        .def_ro("device_type", &Fw::FreeWiliDevice::deviceType)
        .def_ro("name", &Fw::FreeWiliDevice::name)
        .def_ro("serial", &Fw::FreeWiliDevice::serial)
        .def_ro("unique_id", &Fw::FreeWiliDevice::uniqueID)
        .def_ro("standalone", &Fw::FreeWiliDevice::standalone)
        .def_ro("usb_devices", &Fw::FreeWiliDevice::usbDevices)
        .def("get_usb_devices", [](const Fw::FreeWiliDevice& self) { return self.getUSBDevices(); })
        .def(
            "get_usb_devices",
            [](const Fw::FreeWiliDevice& self, Fw::USBDeviceType usbDeviceType) {
                return self.getUSBDevices(usbDeviceType);
            }
        )
        .def(
            "get_usb_devices",
            [](const Fw::FreeWiliDevice& self, const std::vector<Fw::USBDeviceType>& usbDeviceTypes
            ) { return self.getUSBDevices(usbDeviceTypes); }
        )
        .def(
            "get_main_usb_device",
            [](const Fw::FreeWiliDevice& self) {
                auto result = self.getMainUSBDevice();
                if (result.has_value()) {
                    return result.value();
                } else {
                    PyErr_SetString(PyExc_RuntimeError, result.error().c_str());
                    throw nb::python_error();
                }
            }
        )
        .def(
            "get_display_usb_device",
            [](const Fw::FreeWiliDevice& self) {
                auto result = self.getDisplayUSBDevice();
                if (result.has_value()) {
                    return result.value();
                } else {
                    PyErr_SetString(PyExc_RuntimeError, result.error().c_str());
                    throw nb::python_error();
                }
            }
        )
        .def(
            "get_fpga_usb_device",
            [](const Fw::FreeWiliDevice& self) {
                auto result = self.getFPGAUSBDevice();
                if (result.has_value()) {
                    return result.value();
                } else {
                    PyErr_SetString(PyExc_RuntimeError, result.error().c_str());
                    throw nb::python_error();
                }
            }
        )
        .def("get_hub_usb_device", [](const Fw::FreeWiliDevice& self) {
            auto result = self.getHubUSBDevice();
            if (result.has_value()) {
                return result.value();
            } else {
                PyErr_SetString(PyExc_RuntimeError, result.error().c_str());
                throw nb::python_error();
            }
        });

    m.def("find_all", &find_all);
    m.def("get_device_type_name", &Fw::getDeviceTypeName);
    m.def("get_usb_device_type_name", &Fw::getUSBDeviceTypeName);
}
