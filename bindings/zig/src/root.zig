//! By convention, root.zig is the root source file when making a library. If
//! you are making an executable, the convention is to delete this file and
//! start with main.zig instead.
const std = @import("std");
const testing = std.testing;

// Import the C API header
const fw = @cImport({
    @cInclude("cfwfinder.h");
});

const FreeWiliError = error{
    InvalidParameter,
    InvalidDevice,
    InternalError,
    Memory,
    NoMoreDevices,
    None,

    InvalidErrorCode,
    OutOfMemory,
};

const DeviceType = enum(c_int) {
    Unknown,
    FreeWili,
    DEFCON2024Badge,
    DEFCON2025FwBadge,
    UF2,
    Winky,
};

pub fn check_fw_error(code: fw.fw_error_t) FreeWiliError!void {
    return switch (code) {
        fw.fw_error_success => {}, // Success = no error
        fw.fw_error_invalid_parameter => FreeWiliError.InvalidParameter,
        fw.fw_error_invalid_device => FreeWiliError.InvalidDevice,
        fw.fw_error_internal_error => FreeWiliError.InternalError,
        fw.fw_error_memory => FreeWiliError.Memory,
        fw.fw_error_no_more_devices => FreeWiliError.NoMoreDevices,
        fw.fw_error_none => FreeWiliError.None,
        else => FreeWiliError.InvalidErrorCode,
    };
}

const USBDeviceType = enum(c_int) { Hub, Serial, SerialMain, SerialDisplay, MassStorage, ESP32, FTDI, Other, _MaxValue };

pub const USBDevice = struct {
    const Self = @This();

    kind: USBDeviceType,
    kind_name: [64:0]u8,

    vid: u16,
    pid: u16,
    name: [128:0]u8,
    serial: [64:0]u8,
    location: u32,
    port_chain: [10:0]u32,
    port_chain_size: u32,

    path: ?[256:0]u8,
    port: ?[256:0]u8,

    pub fn from_device(device: ?*fw.fw_freewili_device_t) FreeWiliError!Self {
        var usb_device_type: fw.fw_usbdevicetype_t = undefined;
        var result = fw.fw_usb_device_get_type(device, &usb_device_type);
        try check_fw_error(result);

        var kind_name_buf: [64:0]u8 = undefined;
        var kind_name_size: u32 = kind_name_buf.len;
        result = fw.fw_usb_device_get_type_name(usb_device_type, &kind_name_buf[0], &kind_name_size);
        try check_fw_error(result);

        var vid: u32 = undefined;
        result = fw.fw_usb_device_get_int(device, fw.fw_inttype_vid, &vid);
        try check_fw_error(result);

        var pid: u32 = undefined;
        result = fw.fw_usb_device_get_int(device, fw.fw_inttype_pid, &pid);
        try check_fw_error(result);

        var location: u32 = undefined;
        result = fw.fw_usb_device_get_int(device, fw.fw_inttype_location, &location);
        try check_fw_error(result);

        var name_buf: [128:0]u8 = undefined;
        var name_size: u32 = name_buf.len;
        result = fw.fw_usb_device_get_str(device, fw.fw_stringtype_name, &name_buf[0], &name_size);
        try check_fw_error(result);

        var serial_buf: [64:0]u8 = undefined;
        var serial_size: u32 = serial_buf.len;
        result = fw.fw_usb_device_get_str(device, fw.fw_stringtype_serial, &serial_buf[0], &serial_size);
        try check_fw_error(result);

        var path_buf: [256:0]u8 = [_:0]u8{0} ** 256;
        var path_size: u32 = path_buf.len;
        result = fw.fw_usb_device_get_str(device, fw.fw_stringtype_path, &path_buf[0], &path_size);
        if (result == fw.fw_error_none) {
            path_buf[0] = 0; // empty string
            path_size = 0;
        } else {
            try check_fw_error(result);
        }

        var port_buf: [256:0]u8 = [_:0]u8{0} ** 256;
        var port_size: u32 = port_buf.len;
        result = fw.fw_usb_device_get_str(device, fw.fw_stringtype_port, &port_buf[0], &port_size);
        if (result == fw.fw_error_none) {
            port_buf[0] = 0; // empty string
            port_size = 0;
        } else {
            try check_fw_error(result);
        }

        var port_chain_buf: [10:0]u32 = [_:0]u32{0} ** 10;
        var port_chain_size: u32 = port_chain_buf.len;
        result = fw.fw_usb_device_get_port_chain(device, &port_chain_buf[0], &port_chain_size);
        try check_fw_error(result);

        // Build the USBDevice instance and copy buffer contents into its fixed arrays.
        var dev: USBDevice = undefined;
        dev.kind = @enumFromInt(usb_device_type);
        dev.kind_name = kind_name_buf;
        dev.vid = @intCast(vid);
        dev.pid = @intCast(pid);
        dev.name = name_buf;
        dev.serial = serial_buf;
        dev.location = location;
        dev.port_chain = port_chain_buf;
        dev.port_chain_size = port_chain_size;
        if (path_size != 0) {
            dev.path = path_buf;
        } else {
            dev.path = null;
        }
        if (port_size != 0) {
            dev.port = port_buf;
        } else {
            dev.port = null;
        }

        return dev;
    }
};

pub const FreeWiliDevice = struct {
    const Self = @This();

    handle: ?*fw.fw_freewili_device_t,

    pub fn find_all(allocator: std.mem.Allocator) FreeWiliError![]Self {
        const MAX_SIZE: u32 = 64;
        var devices: [MAX_SIZE]?*fw.fw_freewili_device_t = [_]?*fw.fw_freewili_device_t{null} ** MAX_SIZE;
        var count: u32 = devices.len;
        var error_message: [256]u8 = [_]u8{0} ** 256;
        var error_message_size: u32 = error_message.len;

        const result = fw.fw_device_find_all(&devices, &count, &error_message, &error_message_size);
        try check_fw_error(result);

        var fw_devices = allocator.alloc(Self, count) catch |err| switch (err) {
            error.OutOfMemory => return FreeWiliError.OutOfMemory,
        };
        for (devices[0..count], 0..) |device, i| {
            fw_devices[i] = Self{
                .handle = device,
            };
        }

        return fw_devices;
    }

    pub fn device_type(self: *const Self) FreeWiliError!DeviceType {
        var dev_type: fw.fw_devicetype_t = undefined;
        const result = fw.fw_device_get_type(self.handle, &dev_type);
        try check_fw_error(result);
        return @enumFromInt(dev_type);
    }

    pub fn device_type_name(self: *const Self, buffer: []u8) FreeWiliError![]const u8 {
        var dev_type: fw.fw_devicetype_t = undefined;
        const type_result = fw.fw_device_get_type(self.handle, &dev_type);
        try check_fw_error(type_result);

        var buffer_size: u32 = @intCast(buffer.len);

        // Get the device type name
        const result = fw.fw_device_get_type_name(dev_type, buffer.ptr, &buffer_size);
        try check_fw_error(result);

        return buffer[0..buffer_size];
    }

    fn device_get_string(self: *const Self, string_type: fw.fw_stringtype_t, buffer: []u8) FreeWiliError![]const u8 {
        var buffer_size: u32 = @intCast(buffer.len);

        // Get the device type name
        const result = fw.fw_device_get_str(self.handle, string_type, buffer.ptr, &buffer_size);
        try check_fw_error(result);

        return buffer[0..buffer_size];
    }

    pub fn name(self: *const Self, buffer: []u8) FreeWiliError![]const u8 {
        return self.device_get_string(fw.fw_stringtype_name, buffer);
    }

    pub fn serial(self: *const Self, buffer: []u8) FreeWiliError![]const u8 {
        return self.device_get_string(fw.fw_stringtype_serial, buffer);
    }

    pub fn unique_id(self: *const Self) FreeWiliError!u64 {
        var uid: u64 = 0;
        const result = fw.fw_device_unique_id(self.handle, &uid);
        try check_fw_error(result);
        return uid;
    }

    pub fn standalone(self: *const Self) FreeWiliError!bool {
        var is_standalone: bool = false;
        const result = fw.fw_device_is_standalone(self.handle, &is_standalone);
        try check_fw_error(result);
        return is_standalone;
    }

    fn usb_device_get_string(self: *const Self, string_type: fw.fw_stringtype_t, buffer: []u8) FreeWiliError![]const u8 {
        var buffer_size: u32 = @intCast(buffer.len);

        // Get the USB device string
        const result = fw.fw_usb_device_get_str(self.handle, string_type, buffer.ptr, &buffer_size);
        try check_fw_error(result);

        return buffer[0..buffer_size];
    }

    pub fn get_usb_devices(self: *const Self, usb_devices: []USBDevice) FreeWiliError![]USBDevice {
        var result = fw.fw_usb_device_begin(self.handle);
        try check_fw_error(result);

        var usb_device_count: usize = 0;
        while (true) {
            if (usb_devices.len == usb_device_count) break; // Prevent overflow

            const usb_device = try USBDevice.from_device(self.handle);

            usb_devices[usb_device_count] = usb_device;
            usb_device_count += 1;

            // Get the next USBDevice
            result = fw.fw_usb_device_next(self.handle);
            if (result == fw.fw_error_no_more_devices) break;
            try check_fw_error(result);
        }
        return usb_devices[0..usb_device_count];
    }

    pub fn get_main_usb_device(
        self: *const Self,
        error_message: [*c]u8,
        error_message_size: *u32,
    ) FreeWiliError!USBDevice {
        const result = fw.fw_usb_device_set(self.handle, fw.fw_usbdevice_iter_main, error_message, error_message_size);
        try check_fw_error(result);

        const usb_device = try USBDevice.from_device(self.handle);
        return usb_device;
    }

    pub fn get_display_usb_device(
        self: *const Self,
        error_message: [*c]u8,
        error_message_size: *u32,
    ) FreeWiliError!USBDevice {
        const result = fw.fw_usb_device_set(self.handle, fw.fw_usbdevice_iter_display, error_message, error_message_size);
        try check_fw_error(result);

        const usb_device = try USBDevice.from_device(self.handle);
        return usb_device;
    }

    pub fn get_fpga_usb_device(
        self: *const Self,
        error_message: [*c]u8,
        error_message_size: *u32,
    ) FreeWiliError!USBDevice {
        const result = fw.fw_usb_device_set(self.handle, fw.fw_usbdevice_iter_fpga, error_message, error_message_size);
        try check_fw_error(result);

        const usb_device = try USBDevice.from_device(self.handle);
        return usb_device;
    }

    pub fn get_hub_usb_device(
        self: *const Self,
        error_message: [*c]u8,
        error_message_size: *u32,
    ) FreeWiliError!USBDevice {
        const result = fw.fw_usb_device_set(self.handle, fw.fw_usbdevice_iter_hub, error_message, error_message_size);
        try check_fw_error(result);

        const usb_device = try USBDevice.from_device(self.handle);
        return usb_device;
    }
};

test "basic functionality" {
    const devices = FreeWiliDevice.find_all(std.testing.allocator) catch |err| {
        std.debug.print("Error finding devices: {}\n", .{
            err,
        });
        return; // Exit the test early if device finding fails
    };
    defer std.testing.allocator.free(devices);
    for (devices, 1..) |device, i| {
        const dev_type = device.device_type() catch |err| {
            std.debug.print("Error getting device type: {}\n", .{
                err,
            });
            return;
        };
        var type_name_buf: [128:0]u8 = [_:0]u8{0} ** 128;
        const type_name = device.device_type_name(&type_name_buf) catch |err| {
            std.debug.print("Error getting device type name: {}\n", .{
                err,
            });
            return;
        };

        var name_buf: [128:0]u8 = [_:0]u8{0} ** 128;
        const name = device.name(&name_buf) catch |err| {
            std.debug.print("Error getting device name: {}\n", .{
                err,
            });
            return;
        };
        var serial_buf: [128:0]u8 = [_:0]u8{0} ** 128;
        const serial = device.serial(&serial_buf) catch |err| {
            std.debug.print("Error getting device serial: {}\n", .{
                err,
            });
            return;
        };

        const unique_id = device.unique_id() catch |err| {
            std.debug.print("Error getting device unique ID: {}\n", .{
                err,
            });
            return;
        };

        const standalone = device.standalone() catch |err| {
            std.debug.print("Error checking if device is standalone: {}\n", .{
                err,
            });
            return;
        };

        var usb_devices_buf: [10]USBDevice = undefined;
        const usb_devices = device.get_usb_devices(usb_devices_buf[0..]) catch |err| {
            std.debug.print("Error getting USB devices: {}\n", .{
                err,
            });
            return;
        };
        std.debug.print("{}. Found device: {}\n", .{ i, device });
        std.debug.print("\ttype: {}\n", .{dev_type});
        std.debug.print("\ttype name: {s}\n", .{type_name});
        std.debug.print("\tname: {s}\n", .{name});
        std.debug.print("\tserial: {s}\n", .{serial});
        std.debug.print("\tunique ID: {}\n", .{unique_id});
        std.debug.print("\tstandalone: {}\n", .{standalone});
        std.debug.print("\tUSB devices ({}):\n", .{usb_devices.len});
        for (usb_devices, 1..) |usb_device, count| {
            std.debug.print("\t\t{}: {s}\n", .{ count, usb_device.kind_name });
            std.debug.print("\t\t\tname: {s}\n", .{usb_device.name});
            std.debug.print("\t\t\tserial: {s}\n", .{usb_device.serial});
            std.debug.print("\t\t\tVID: {} PID: {}\n", .{ usb_device.vid, usb_device.pid });
            std.debug.print("\t\t\tlocation: {}\n", .{usb_device.location});
            std.debug.print("\t\t\tport chain: {any}\n", .{usb_device.port_chain[0..usb_device.port_chain_size]});
            if (usb_device.path) |path| {
                std.debug.print("\t\t\tpath: {s}\n", .{path});
            }
            if (usb_device.port) |port| {
                std.debug.print("\t\t\tport: {s}\n", .{port});
            }
        }

        var error_message: [255]u8 = [_]u8{0} ** 255;
        var error_message_size: u32 = error_message.len;
        if (device.get_main_usb_device(&error_message[0], &error_message_size)) |main_usb_device| {
            std.debug.print("\tMain USB device: {s}\n", .{main_usb_device.kind_name});
        } else |err| {
            std.debug.print("\tError getting main USB device: {}: {s}\n", .{ err, error_message[0..error_message_size] });
        }

        if (device.get_display_usb_device(&error_message[0], &error_message_size)) |display_usb_device| {
            std.debug.print("\tDisplay USB device: {s}\n", .{display_usb_device.kind_name});
        } else |err| {
            std.debug.print("\tError getting display USB device: {}: {s}\n", .{ err, error_message[0..error_message_size] });
        }

        if (device.get_fpga_usb_device(&error_message[0], &error_message_size)) |fpga_usb_device| {
            std.debug.print("\tFPGA USB device: {s}\n", .{fpga_usb_device.kind_name});
        } else |err| {
            std.debug.print("\tError getting FPGA USB device: {}: {s}\n", .{ err, error_message[0..error_message_size] });
        }

        if (device.get_hub_usb_device(&error_message[0], &error_message_size)) |hub_usb_device| {
            std.debug.print("\tHUB USB device: {s}\n", .{hub_usb_device.kind_name});
        } else |err| {
            std.debug.print("\tError getting HUB USB device: {}: {s}\n", .{ err, error_message[0..error_message_size] });
        }
    }
}
