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

int add(int a, int b) {
    return a + b;
}

auto find_all() -> Fw::FreeWiliDevices {
    if (auto devicesResult = Fw::find_all(); !devicesResult.has_value()) {
        PyErr_SetString(PyExc_RuntimeError, devicesResult.error().c_str());
        throw nb::python_error();
    } else {
        return devicesResult.value();
    }
}

NB_MODULE(pyfwfinder, m) {
    m.def("add", &add);

    nb::enum_<Fw::USBDeviceType>(m, "USBDeviceType")
        .value("Hub", Fw::USBDeviceType::Hub)
        .value("Serial", Fw::USBDeviceType::Serial)
        .value("MassStorage", Fw::USBDeviceType::MassStorage)
        .value("ESP32", Fw::USBDeviceType::ESP32)
        .value("FTDI", Fw::USBDeviceType::FTDI)
        .value("Other", Fw::USBDeviceType::Other);

    nb::class_<Fw::USBDevice>(m, "USBDevice")
        .def(nb::init<>())
        .def_ro("kind", &Fw::USBDevice::kind)
        .def_ro("vid", &Fw::USBDevice::vid)
        .def_ro("pid", &Fw::USBDevice::pid)
        .def_ro("name", &Fw::USBDevice::name)
        .def_ro("serial", &Fw::USBDevice::serial)
        .def_ro("location", &Fw::USBDevice::location)
        .def_ro("paths", &Fw::USBDevice::paths)
        .def_ro("port", &Fw::USBDevice::port)
        .def_ro("_raw", &Fw::USBDevice::_raw);

    //nb::class_<Fw::USBDevices>(m, "USBDevices").def(nb::init<>());

    nb::class_<Fw::FreeWiliDevice>(m, "FreeWiliDevice")
        .def(nb::init<>())
        .def_ro("name", &Fw::FreeWiliDevice::name)
        .def_ro("serial", &Fw::FreeWiliDevice::serial)
        .def_ro("usb_hub", &Fw::FreeWiliDevice::usbHub)
        .def_ro("usb_devices", &Fw::FreeWiliDevice::usbDevices)
        .def("get_usb_devices", &Fw::FreeWiliDevice::getUSBDevices);

    //nb::class_<Fw::FreeWiliDevices>(m, "FreeWiliDevices"); //.def(nb::init<Fw::FreeWiliDevices>());

    m.def("find_all", &find_all);
}
