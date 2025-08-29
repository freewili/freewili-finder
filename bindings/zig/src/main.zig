const std = @import("std");
const fw = @import("freewili_finder");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer {
        _ = gpa.deinit();
    }

    const devices = fw.FreeWiliDevice.find_all(allocator) catch |err| {
        std.debug.print("Error finding devices: {}\n", .{
            err,
        });
        return; // Exit the test early if device finding fails
    };
    defer allocator.free(devices);
    std.debug.print("Found {} device(s)\n", .{devices.len});
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

        var usb_devices_buf: [10]fw.USBDevice = undefined;
        const usb_devices = device.get_usb_devices(usb_devices_buf[0..]) catch |err| {
            std.debug.print("Error getting USB devices: {}\n", .{
                err,
            });
            return;
        };
        std.debug.print("{}. Found device: {}\n", .{ i, device });
        std.debug.print("  type: {}\n", .{dev_type});
        std.debug.print("  type name: {s}\n", .{type_name});
        std.debug.print("  name: {s}\n", .{name});
        std.debug.print("  serial: {s}\n", .{serial});
        std.debug.print("  unique ID: 0x{X}\n", .{unique_id});
        std.debug.print("  standalone: {}\n", .{standalone});
        std.debug.print("  USB devices ({}):\n", .{usb_devices.len});
        for (usb_devices, 1..) |usb_device, count| {
            std.debug.print("    {}: {s}\n", .{ count, usb_device.kind_name });
            std.debug.print("      name: {s}\n", .{usb_device.name});
            std.debug.print("      serial: {s}\n", .{usb_device.serial});
            std.debug.print("      VID: 0x{X} PID: 0x{X}\n", .{ usb_device.vid, usb_device.pid });
            std.debug.print("      location: {}\n", .{usb_device.location});
            std.debug.print("      port chain: {any}\n", .{usb_device.port_chain[0..usb_device.port_chain_size]});
            if (usb_device.path) |path| {
                std.debug.print("      path: {s}\n", .{path});
            }
            if (usb_device.port) |port| {
                std.debug.print("      port: {s}\n", .{port});
            }
        }

        var error_message: [255]u8 = [_]u8{0} ** 255;
        var error_message_size: u32 = error_message.len;
        if (device.get_main_usb_device(&error_message[0], &error_message_size)) |main_usb_device| {
            std.debug.print("  Main USB device: {s}\n", .{main_usb_device.kind_name});
        } else |err| {
            std.debug.print("  Error getting main USB device: {}: {s}\n", .{ err, error_message[0..error_message_size] });
        }

        if (device.get_display_usb_device(&error_message[0], &error_message_size)) |display_usb_device| {
            std.debug.print("  Display USB device: {s}\n", .{display_usb_device.kind_name});
        } else |err| {
            std.debug.print("  Error getting display USB device: {}: {s}\n", .{ err, error_message[0..error_message_size] });
        }

        if (device.get_fpga_usb_device(&error_message[0], &error_message_size)) |fpga_usb_device| {
            std.debug.print("  FPGA USB device: {s}\n", .{fpga_usb_device.kind_name});
        } else |err| {
            std.debug.print("  Error getting FPGA USB device: {}: {s}\n", .{ err, error_message[0..error_message_size] });
        }

        if (device.get_hub_usb_device(&error_message[0], &error_message_size)) |hub_usb_device| {
            std.debug.print("  HUB USB device: {s}\n", .{hub_usb_device.kind_name});
        } else |err| {
            std.debug.print("  Error getting HUB USB device: {}: {s}\n", .{ err, error_message[0..error_message_size] });
        }
    }
}
