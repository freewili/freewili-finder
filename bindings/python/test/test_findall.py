import sys
sys.path.append('/home/drebbe/dev/freewili-finder/build/bindings/python')
import pyfwfinder


def test_findall():
    devices = pyfwfinder.find_all()
    print(f"Found {len(devices)} FreeWili(s)...")
    for device in devices:
        print(device.name, device.serial)
        for usb_device in device.usb_devices:
            print('\t', usb_device.kind)
            print(f'\t\t{usb_device.name} {usb_device.serial}')
            print(f'\t\tVID: {hex(usb_device.vid)} PID: {hex(usb_device.pid)}')
            print(f'\t\tLocation: {usb_device.location}')
            print(f'\t\t{usb_device._raw}')

if __name__ == "__main__":
    test_findall()
