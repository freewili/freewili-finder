import pytest
import pyfwfinder


def test_findall() -> None:
    assert hasattr(pyfwfinder, "find_all")
    devices = pyfwfinder.find_all()

def test_usbdevicetype() -> None:
    assert pyfwfinder.USBDeviceType.Hub.value == 0
    assert pyfwfinder.USBDeviceType.Serial.value == 1
    assert pyfwfinder.USBDeviceType.SerialMain.value == 2
    assert pyfwfinder.USBDeviceType.SerialDisplay.value == 3
    assert pyfwfinder.USBDeviceType.MassStorage.value == 4
    assert pyfwfinder.USBDeviceType.ESP32.value == 5
    assert pyfwfinder.USBDeviceType.FTDI.value == 6
    assert pyfwfinder.USBDeviceType.Other.value == 7
    assert pyfwfinder.USBDeviceType._MaxValue.value == 8

def test_usbdevice() -> None:
    assert hasattr(pyfwfinder, "USBDevice")
    assert hasattr(pyfwfinder.USBDevice, "kind")
    assert hasattr(pyfwfinder.USBDevice, "vid")
    assert hasattr(pyfwfinder.USBDevice, "pid")
    assert hasattr(pyfwfinder.USBDevice, "name")
    assert hasattr(pyfwfinder.USBDevice, "serial")
    assert hasattr(pyfwfinder.USBDevice, "location")
    assert hasattr(pyfwfinder.USBDevice, "paths")
    assert hasattr(pyfwfinder.USBDevice, "port")
    assert hasattr(pyfwfinder.USBDevice, "_raw")

def test_freewilidevice() -> None:
    assert hasattr(pyfwfinder, "FreeWiliDevice")
    assert hasattr(pyfwfinder.FreeWiliDevice, "name")
    assert hasattr(pyfwfinder.FreeWiliDevice, "serial")
    assert hasattr(pyfwfinder.FreeWiliDevice, "usb_hub")
    assert hasattr(pyfwfinder.FreeWiliDevice, "usb_devices")
    assert hasattr(pyfwfinder.FreeWiliDevice, "get_usb_devices")



if __name__ == "__main__":
    test_findall()
    test_usbdevicetype()
    test_usbdevice()
    test_freewilidevice()
