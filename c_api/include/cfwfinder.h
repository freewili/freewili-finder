#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
    //  Microsoft
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
#else
    //  do nothing and hope for the best?
    #define EXPORT
    #define IMPORT
    #pragma warning Unknown dynamic link import / export semantics.
#endif

#ifdef CFW_FINDER_BUILD_STATIC
    #define CFW_FINDER_API
#else
    #ifdef CFW_FINDER_BUILD_DYNAMIC
        #define CFW_FINDER_API EXPORT
    #else
        #define CFW_FINDER_API IMPORT
    #endif // CFW_FINDER_BUILD_DYNAMIC
#endif // CFW_FINDER_BUILD_STATIC

typedef enum _fw_error_t {
    fw_error_success = 0,
    fw_error_invalid_parameter,
    fw_error_invalid_device,
    fw_error_internal_error,
    fw_error_memory,
    fw_error_no_more_devices,
    fw_error_none,

    // Keep this at the end
    fw_error__maxvalue,
} _fw_error_t;
typedef uint32_t fw_error_t;

/// Type of USB Device
typedef enum _fw_usbdevicetype_t {
    /// USB Hub, every FreeWili has one as the parent
    fw_usbdevicetype_hub,
    /// Serial Port (COMx) on windows
    fw_usbdevicetype_serial,
    /// Serial Port MainCPU
    fw_usbdevicetype_serialmain,
    /// Serial Port DisplayCPU
    fw_usbdevicetype_serialdisplay,
    /// Mass Storage Device
    fw_usbdevicetype_massstorage,
    /// ESP32 USB (JTAG/RTT)
    fw_usbdevicetype_esp32,
    /// FTDI / FPGA
    fw_usbdevicetype_ftdi,
    /// Some other USB device attached to the same hub
    fw_usbdevicetype_other,

    // Keep this at the end
    fw_usbdevicetype__maxvalue,
} _fw_usbdevicetype_t;
// Define the type for USB device type
typedef uint32_t fw_usbdevicetype_t;

typedef enum _fw_stringtype_t {
    fw_stringtype_name,
    fw_stringtype_serial,
    fw_stringtype_path, // USB Device For Mass Storage devices
    fw_stringtype_port, // USB Device For Serial devices
    fw_stringtype_raw, // USB Device Internal system path of the USBDevice
    fw_stringtype_type, // USB Device USB device type

    // Keep this at the end
    fw_stringtype__maxvalue,
} _fw_stringtype_t;
typedef uint32_t fw_stringtype_t;

typedef enum _fw_inttype_t {
    fw_inttype_vid,
    fw_inttype_pid,
    fw_inttype_location,

    // Keep this at the end
    fw_inttype__maxvalue,
} _fw_inttype_t;
typedef uint32_t fw_inttype_t;

/**
 * @brief Opaque type for a C++ FreeWiliDevice.
 *
 * This type is used to represent a FreeWiLi device in the C API.
 * It is an opaque type, meaning its internal structure is not exposed.
 */
typedef struct fw_usbdevice_t fw_usbdevice_t;

/**
 * @brief Opaque type for a C++ USB device.
 *
 * This type is used to represent a USB device in the C API.
 * It is an opaque type, meaning its internal structure is not exposed.
 */
typedef struct fw_freewili_device_t fw_freewili_device_t;

/**
 * @brief Finds all available FreeWiLi devices.
 *
 * This function scans for all connected FreeWiLi devices and populates the provided
 * array with pointers to the detected devices. The total number of devices found
 * is returned via the count parameter.
 *
 * @param[out] devices Pointer to an array of fw_freewili_device_t pointers that will be allocated and filled with the found devices.
 *                     The caller is responsible for freeing the allocated memory.
 * @param[out] count   Pointer to a uint32_t that will be set to the number of devices found.
 *
 * @param[out] error_message Pointer to a buffer where an error message can be stored if the operation fails.
 *
 * @param[out] error_message_size The size of the error_message buffer in bytes. If the operation fails, an error message will be written to this buffer.
 *
 * @return fw_error_t
 *
 * @see fw_device_free
 */
CFW_FINDER_API fw_error_t fw_device_find_all(
    fw_freewili_device_t** devices,
    uint32_t* count,
    char* const error_message,
    uint32_t* error_message_size
);

/**
 * @brief Checks if a FreeWiLi device is valid.
 *
 * This function checks if the provided FreeWiLi device pointer is valid and initialized.
 * It returns true if the device is valid, false otherwise.
 *
 * @param device Pointer to the fw_freewili_device_t to be checked.
 *
 * @return true if the device is valid, false otherwise.
 */
CFW_FINDER_API bool fw_device_is_valid(fw_freewili_device_t* device);

/**
 * @brief Frees the memory allocated for the FreeWiLi devices.
 *
 * This function frees the memory allocated for the array of fw_freewili_device_t pointers
 * and each individual device within that array.
 *
 * @param devices Pointer to an array of fw_freewili_device_t pointers to be freed.
 * @param count   The number of devices in the array.
 */
CFW_FINDER_API fw_error_t fw_device_free(fw_freewili_device_t** devices, uint32_t count);

/**
 * @brief Retrieves a string value from a FreeWiLi device.
 *
 * This function retrieves a string value of the specified type from the given FreeWiLi device.
 * The retrieved value is stored in the provided buffer, which must be large enough to hold the string.
 *
 * @param device   Pointer to the fw_freewili_device_t from which to retrieve the string.
 * @param str_type The type of string to retrieve (e.g., name, serial).
 * @param value    Pointer to a buffer where the retrieved string will be stored.
 * @param value_size     The size of the buffer in bytes.
 *
 * @return true if the operation was successful, false otherwise.
 */
CFW_FINDER_API fw_error_t fw_device_get_str(
    fw_freewili_device_t* device,
    fw_stringtype_t str_type,
    char* const value,
    uint32_t value_size
);

/**
    * @brief Begins the USB device enumeration for a FreeWiLi device.
    *
    * This function initializes the USB device enumeration process for the specified FreeWiLi device.
    * It prepares the device to retrieve USB devices of a specific type in subsequent calls.
    *
    * @param device Pointer to the fw_freewili_device_t for which to begin USB device enumeration.
    *
    * @return fw_error_success on success, or an error code on failure.
    *
    * @note This function must be called before calling fw_usb_device_next to retrieve USB devices.
    *       It sets up the internal state of the device to start enumerating USB devices.
    *
    * @see fw_usb_device_next
    * @see fw_usb_device_get_str
    * @see fw_usb_device_get_int
 */
CFW_FINDER_API fw_error_t fw_usb_device_begin(fw_freewili_device_t* device);

/**
    * @brief Retrieves the next USB device from a FreeWiLi device.
    *
    * @param device Pointer to the fw_freewili_device_t for which to begin USB device enumeration.
    *
    * @return fw_error_success on success, or an error code on failure. fw_error_no_more_devices if there are no more USB devices to enumerate.
    *
    * @note This function should be called after fw_usb_device_begin to retrieve USB devices one by one.
    *       It will return the next USB device of the specified type, or an error if there are no more devices.
    *
    * @see fw_usb_device_begin
    * @see fw_usb_device_get_str
    * @see fw_usb_device_get_int
 */
CFW_FINDER_API fw_error_t fw_usb_device_next(fw_freewili_device_t* device);

/**
 * @brief Provides the count of USB devices found.
 *
 * @param device Pointer to the fw_freewili_device_t from which to retrieve the next USB device.
 * @param count[out]  Pointer to a uint32_t that will be updated with the number of devices found.
 *
 * @return fw_error_success on success, or an error code on failure.
 */
CFW_FINDER_API fw_error_t fw_usb_device_count(fw_freewili_device_t* device, uint32_t* count);

/**
 * @brief Retrieves the type of a USB device.
 *
 * This function retrieves the type of USB device being enumerated.
 * The type is returned as a fw_usbdevicetype_t value.
 *
 * @param device Pointer to the fw_freewili_device_t from which to retrieve the type of USB device.
 * @param usb_device_type[out] Pointer to a fw_usbdevicetype_t where the USB device type will be stored.
 *
 * @return fw_error_success on success, or an error code on failure.
 */
CFW_FINDER_API fw_error_t
fw_usb_device_get_type(fw_freewili_device_t* device, fw_usbdevicetype_t* usb_device_type);

/**
 * @brief Retrieves a string value from a USB device.
 *
 * This function retrieves a string value of the specified type from the given USB device.
 * The retrieved value is stored in the provided buffer, which must be large enough to hold the string.
 *
 * @param usb_device Pointer to the fw_usbdevice_t from which to retrieve the string.
 * @param str_type   The type of string to retrieve (e.g., name, serial).
 * @param value      Pointer to a buffer where the retrieved string will be stored.
 * @param size       The size of the buffer in bytes.
 *
 * @return true if the operation was successful, false otherwise.
 */
CFW_FINDER_API fw_error_t fw_usb_device_get_str(
    fw_freewili_device_t* device,
    fw_stringtype_t str_type,
    char* const value,
    uint32_t size
);

/**
 * @brief Retrieves an integer value from a USB device.
 *
 * This function retrieves an integer value of the specified type from the given USB device.
 * The retrieved value is stored in the provided pointer.
 *
 * @param usb_device Pointer to the fw_usbdevice_t from which to retrieve the integer.
 * @param int_type   The type of integer to retrieve (e.g., VID, PID, location).
 * @param value      Pointer to a uint32_t where the retrieved integer will be stored.
 *
 * @return true if the operation was successful, false otherwise.
 */
CFW_FINDER_API fw_error_t
fw_usb_device_get_int(fw_freewili_device_t* device, fw_inttype_t int_type, uint32_t* value);

#ifdef __cplusplus
}
#endif
