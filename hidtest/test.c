/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 libusb/hidapi Team

 Copyright 2022.

 This contents of this file may be used by anyone
 for any reason without any conditions and may be
 used as a starting point for your own applications
 which use HIDAPI.
********************************************************/

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>

#include <hidapi.h>

// Headers needed for sleeping.
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Fallback/example
#ifndef HID_API_MAKE_VERSION
#define HID_API_MAKE_VERSION(mj, mn, p) (((mj) << 24) | ((mn) << 8) | (p))
#endif
#ifndef HID_API_VERSION
#define HID_API_VERSION HID_API_MAKE_VERSION(HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH)
#endif

//
// Sample using platform-specific headers
#if defined(__APPLE__) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_darwin.h>
#endif

#if defined(_WIN32) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_winapi.h>
#endif

#if defined(USING_HIDAPI_LIBUSB) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_libusb.h>
#endif
//

void print_device(struct hid_device_info* cur_dev) {
	printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
	printf("\n");
	printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
	printf("  Product:      %ls\n", cur_dev->product_string);
	printf("  Release:      %hx\n", cur_dev->release_number);
	printf("  Interface:    %d\n", cur_dev->interface_number);
	printf("  Usage (page): 0x%hx (0x%hx)\n", cur_dev->usage, cur_dev->usage_page);
	printf("  Bus type: %d\n", cur_dev->bus_type);
	printf("\n");
}

struct hid_device_ {
	HANDLE device_handle;
	BOOL blocking;
	USHORT output_report_length;
	unsigned char* write_buf;
	size_t input_report_length;
	USHORT feature_report_length;
	unsigned char* feature_buf;
	wchar_t* last_error_str;
	BOOL read_pending;
	char* read_buf;
	OVERLAPPED ol;
	OVERLAPPED write_ol;
	struct hid_device_info* device_info;
};

void print_dev(struct hid_device_* dev) {
	printf("Report Found");
	printf("\n");
	printf("  output_report_length:  %d\n", dev->output_report_length);
	printf("  input_report_length :  %d\n", dev->input_report_length);
	printf("  feature_report_length: %d\n", dev->feature_report_length);
	printf("\n");
}

void print_devices(struct hid_device_info* cur_dev) {
	while (cur_dev) {
		print_device(cur_dev);
		cur_dev = cur_dev->next;
	}
}

struct hid_device_info* find_device(struct hid_device_info* cur_dev, unsigned short usage_page, unsigned short usage) {
	while (cur_dev) {
		if (cur_dev->usage_page == usage_page && cur_dev->usage == usage) {
			return cur_dev;
		}
		cur_dev = cur_dev->next;
	}
	return NULL;
}

HID_API_EXPORT hid_device* HID_API_CALL hid_open_usage(unsigned short vendor_id, unsigned short product_id, unsigned short usage_page, unsigned short usage)
{
	/* TODO: Merge this functions with the Linux version. This function should be platform independent. */
	struct hid_device_info* devs, * cur_dev;
	const char* path_to_open = NULL;
	hid_device* handle = NULL;

	/* register_global_error: global error is reset by hid_enumerate/hid_init */
	devs = hid_enumerate(vendor_id, product_id);
	if (!devs) {
		/* register_global_error: global error is already set by hid_enumerate */
		return NULL;
	}

	cur_dev = devs;
	while (cur_dev) {
		if (cur_dev->vendor_id == vendor_id &&
			cur_dev->product_id == product_id) {
			if (cur_dev->usage_page == usage_page && cur_dev->usage == usage) {
				path_to_open = cur_dev->path;
				break;
			}
		}
		cur_dev = cur_dev->next;
	}

	if (path_to_open) {
		/* Open the device */
		handle = hid_open_path(path_to_open);
	}
	else {
		printf("Device with requested VID/PID/(SerialNumber) not found\n");
	}

	hid_free_enumeration(devs);

	return handle;
}

int test_test(void)
{
	int res;
	unsigned char buf[256];
#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	hid_device* handle;
	int i;

	// Set up the command buffer.
	memset(buf, 0x00, sizeof(buf));
	buf[0] = 0x01;
	buf[1] = 0x81;

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	////handle = hid_open(0x4d8, 0x3f, L"12345");
	handle = hid_open(0x4d8, 0x3f, NULL);
	if (!handle) {
		printf("unable to open device\n");
		hid_exit();
		return 1;
	}

	// Read the Manufacturer String
	wstr[0] = 0x0000;
	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read manufacturer string\n");
	printf("Manufacturer String: %ls\n", wstr);

	// Read the Product String
	wstr[0] = 0x0000;
	res = hid_get_product_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read product string\n");
	printf("Product String: %ls\n", wstr);

	// Read the Serial Number String
	wstr[0] = 0x0000;
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read serial number string\n");
	printf("Serial Number String: (%d) %ls", wstr[0], wstr);
	printf("\n");

	struct hid_device_info* info = hid_get_device_info(handle);
	if (info == NULL) {
		printf("Unable to get device info\n");
	}
	else {
		print_devices(info);
	}

	// Read Indexed String 1
	wstr[0] = 0x0000;
	res = hid_get_indexed_string(handle, 1, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read indexed string 1\n");
	printf("Indexed String 1: %ls\n", wstr);

	// Set the hid_read() function to be non-blocking.
	hid_set_nonblocking(handle, 1);

	// Try to read from the device. There should be no
	// data here, but execution should not block.
	res = hid_read(handle, buf, 17);

	// Send a Feature Report to the device
	buf[0] = 0x2;
	buf[1] = 0xa0;
	buf[2] = 0x0a;
	buf[3] = 0x00;
	buf[4] = 0x00;
	res = hid_send_feature_report(handle, buf, 17);
	if (res < 0) {
		printf("Unable to send a feature report.\n");
	}

	memset(buf, 0, sizeof(buf));

	// Read a Feature Report from the device
	buf[0] = 0x2;
	res = hid_get_feature_report(handle, buf, sizeof(buf));
	if (res < 0) {
		printf("Unable to get a feature report: %ls\n", hid_error(handle));
	}
	else {
		// Print out the returned buffer.
		printf("Feature Report\n   ");
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	memset(buf, 0, sizeof(buf));

	// Toggle LED (cmd 0x80). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x80;
	res = hid_write(handle, buf, 17);
	if (res < 0) {
		printf("Unable to write(): %ls\n", hid_error(handle));
	}


	// Request state (cmd 0x81). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x81;
	hid_write(handle, buf, 17);
	if (res < 0) {
		printf("Unable to write()/2: %ls\n", hid_error(handle));
	}

	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	res = 0;
	i = 0;
	while (res == 0) {
		res = hid_read(handle, buf, sizeof(buf));
		if (res == 0) {
			printf("waiting...\n");
		}
		if (res < 0) {
			printf("Unable to read(): %ls\n", hid_error(handle));
			break;
		}

		i++;
		if (i >= 10) { /* 10 tries by 500 ms - 5 seconds of waiting*/
			printf("read() timeout\n");
			break;
		}

#ifdef _WIN32
		Sleep(500);
#else
		usleep(500 * 1000);
#endif
	}

	if (res > 0) {
		printf("Data read:\n   ");
		// Print out the returned buffer.
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	hid_close(handle);

	return 0;
}

// vid: 0x256c
// pid: 0x006d
int test_huion(void)
{
	int res;
	unsigned char buf[256];
#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	hid_device* handle;
	int i;

	// Set up the command buffer.
	memset(buf, 0x00, sizeof(buf));
	buf[0] = 0x01;
	buf[1] = 0x81;

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	////handle = hid_open(0x4d8, 0x3f, L"12345");
	handle = hid_open(0x256c, 0x006d, NULL);
	if (!handle) {
		printf("unable to open device\n");
		hid_exit();
		return 1;
	}

	// Read the Manufacturer String
	wstr[0] = 0x0000;
	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read manufacturer string\n");
	printf("Manufacturer String: %ls\n", wstr);

	// Read the Product String
	wstr[0] = 0x0000;
	res = hid_get_product_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read product string\n");
	printf("Product String: %ls\n", wstr);

	// Read the Serial Number String
	wstr[0] = 0x0000;
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read serial number string\n");
	printf("Serial Number String: (%d) %ls", wstr[0], wstr);
	printf("\n");

	struct hid_device_info* info = hid_get_device_info(handle);
	if (info == NULL) {
		printf("Unable to get device info\n");
	}
	else {
		print_devices(info);
	}

	// Read Indexed String 1
	wstr[0] = 0x0000;
	res = hid_get_indexed_string(handle, 201, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read indexed string 1\n");
	printf("Indexed String 1: %ls\n", wstr);

	// Set the hid_read() function to be non-blocking.
	hid_set_nonblocking(handle, 1);

	hid_close(handle);

	return 0;

	// Try to read from the device. There should be no
	// data here, but execution should not block.
	res = hid_read(handle, buf, 17);

	// Send a Feature Report to the device
	buf[0] = 0x2;
	buf[1] = 0xa0;
	buf[2] = 0x0a;
	buf[3] = 0x00;
	buf[4] = 0x00;
	res = hid_send_feature_report(handle, buf, 17);
	if (res < 0) {
		printf("Unable to send a feature report.\n");
	}

	memset(buf, 0, sizeof(buf));

	// Read a Feature Report from the device
	buf[0] = 0x2;
	res = hid_get_feature_report(handle, buf, sizeof(buf));
	if (res < 0) {
		printf("Unable to get a feature report: %ls\n", hid_error(handle));
	}
	else {
		// Print out the returned buffer.
		printf("Feature Report\n   ");
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	memset(buf, 0, sizeof(buf));

	// Toggle LED (cmd 0x80). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x80;
	res = hid_write(handle, buf, 17);
	if (res < 0) {
		printf("Unable to write(): %ls\n", hid_error(handle));
	}

	// Request state (cmd 0x81). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x81;
	hid_write(handle, buf, 17);
	if (res < 0) {
		printf("Unable to write()/2: %ls\n", hid_error(handle));
	}

	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	res = 0;
	i = 0;
	while (res == 0) {
		res = hid_read(handle, buf, sizeof(buf));
		if (res == 0) {
			printf("waiting...\n");
		}
		if (res < 0) {
			printf("Unable to read(): %ls\n", hid_error(handle));
			break;
		}

		i++;
		if (i >= 10) { /* 10 tries by 500 ms - 5 seconds of waiting*/
			printf("read() timeout\n");
			break;
		}

#ifdef _WIN32
		Sleep(500);
#else
		usleep(500 * 1000);
#endif
	}

	if (res > 0) {
		printf("Data read:\n   ");
		// Print out the returned buffer.
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	hid_close(handle);

	return 0;
}

// vid: 0x0ed1
// pid: 0x04f8
int test_tescis(void)
{
	int res;
	unsigned char buf[256];
#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	hid_device* handle;
	int i;

	// Set up the command buffer.
	memset(buf, 0x00, sizeof(buf));
	buf[0] = 0x01;
	buf[1] = 0x81;

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	////handle = hid_open(0x4d8, 0x3f, L"12345");
	handle = hid_open(0x0ed1, 0x04f8, NULL);
	if (!handle) {
		printf("unable to open device\n");
		hid_exit();
		return 1;
	}

	// Read the Manufacturer String
	wstr[0] = 0x0000;
	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read manufacturer string\n");
	printf("Manufacturer String: %ls\n", wstr);

	// Read the Product String
	wstr[0] = 0x0000;
	res = hid_get_product_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read product string\n");
	printf("Product String: %ls\n", wstr);

	// Read the Serial Number String
	wstr[0] = 0x0000;
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read serial number string\n");
	printf("Serial Number String: (%d) %ls", wstr[0], wstr);
	printf("\n");

	struct hid_device_info* info = hid_get_device_info(handle);
	if (info == NULL) {
		printf("Unable to get device info\n");
	}
	else {
		print_devices(info);
	}

	//// Read Indexed String 1
	//wstr[0] = 0x0000;
	//res = hid_get_indexed_string(handle, 201, wstr, MAX_STR);
	//if (res < 0)
	//	printf("Unable to read indexed string 1\n");
	//printf("Indexed String 1: %ls\n", wstr);

	// Set the hid_read() function to be non-blocking.
	hid_set_nonblocking(handle, 1);

	hid_close(handle);

	return 0;

	// Try to read from the device. There should be no
	// data here, but execution should not block.
	res = hid_read(handle, buf, 17);

	// Send a Feature Report to the device
	buf[0] = 0x2;
	buf[1] = 0xa0;
	buf[2] = 0x0a;
	buf[3] = 0x00;
	buf[4] = 0x00;
	res = hid_send_feature_report(handle, buf, 17);
	if (res < 0) {
		printf("Unable to send a feature report.\n");
	}

	memset(buf, 0, sizeof(buf));

	// Read a Feature Report from the device
	buf[0] = 0x2;
	res = hid_get_feature_report(handle, buf, sizeof(buf));
	if (res < 0) {
		printf("Unable to get a feature report: %ls\n", hid_error(handle));
	}
	else {
		// Print out the returned buffer.
		printf("Feature Report\n   ");
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	memset(buf, 0, sizeof(buf));

	// Toggle LED (cmd 0x80). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x80;
	res = hid_write(handle, buf, 17);
	if (res < 0) {
		printf("Unable to write(): %ls\n", hid_error(handle));
	}

	// Request state (cmd 0x81). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x81;
	hid_write(handle, buf, 17);
	if (res < 0) {
		printf("Unable to write()/2: %ls\n", hid_error(handle));
	}

	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	res = 0;
	i = 0;
	while (res == 0) {
		res = hid_read(handle, buf, sizeof(buf));
		if (res == 0) {
			printf("waiting...\n");
		}
		if (res < 0) {
			printf("Unable to read(): %ls\n", hid_error(handle));
			break;
		}

		i++;
		if (i >= 10) { /* 10 tries by 500 ms - 5 seconds of waiting*/
			printf("read() timeout\n");
			break;
		}

#ifdef _WIN32
		Sleep(500);
#else
		usleep(500 * 1000);
#endif
	}

	if (res > 0) {
		printf("Data read:\n   ");
		// Print out the returned buffer.
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	hid_close(handle);

	return 0;
}

// 0x0EEF - VID
// 0xC121 - PID
int test_eeti(void)
{
	int res;
	unsigned char buf[256];
#define MAX_STR 255
	//wchar_t wstr[MAX_STR];
	hid_device* handle;
	int i;

	printf("trying to open eeti device\n");

	// Set up the command buffer.
	memset(buf, 0, sizeof(buf)); // clean up to 0
	buf[0] = 0x03; // report id 0x03

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	handle = hid_open_usage(0x0EEF, 0xC121, 0xFF00, 0x01); // Vendor-Defined 1
	if (!handle) {
		printf("unable to open eeti device\n");
		hid_exit();
		return 1;
	}

	struct hid_device_info* info = hid_get_device_info(handle);
	if (info == NULL) {
		printf("Unable to get device info\n");
	}
	else {
		print_devices(info);
	}

	print_dev(handle);

	// Set the hid_read() function to be non-blocking.
	hid_set_nonblocking(handle, 1); // by default it is blocking

	// Try to read from the device. There should be no
	// data here, but execution should not block.
	res = hid_read(handle, buf, 64); // as we set nonblocking above

	//memset(buf, 0, sizeof(buf)); // clean up to 0

	//// Send a Feature Report to the device
	//buf[0] = 0x03; // report id
	//buf[1] = 0x05; // length
	//buf[2] = 0x36; // CmdMj // FIXED
	//buf[3] = 0x91; // CmdMn // FIXED
	//buf[4] = 0x10; // DNSubCmd1 // FIXED
	//buf[5] = 0x01; // DNSubCmd2 // Enabling and Disabling of Touch Solution
	//buf[6] = 0x01; // 0x00, disable, 0x01, enable

	//res = hid_send_feature_report(handle, buf, 64);
	//if (res < 0) {
	//	printf("Unable to send a feature report.\n");
	//}

	//memset(buf, 0, sizeof(buf)); // clean up to 0

	//// Read a Feature Report from the device
	//buf[0] = 0x03; // report id
	//buf[1] = 0x04; // length
	//buf[2] = 0x36; // CmdMj // FIXED
	//buf[3] = 0x91; // CmdMn // FIXED
	//buf[4] = 0x10; // DNSubCmd1 // FIXED
	//buf[5] = 0x81; // DNSubCmd2 // Enabling and Disabling of Touch Solution

	//res = hid_get_feature_report(handle, buf, 64);
	//if (res < 0) {
	//	printf("Unable to get a feature report: %ls\n", hid_error(handle));
	//}
	//else {
	//	// Print out the returned buffer.
	//	printf("Feature Report\n   ");
	//	for (i = 0; i < res; i++)
	//		printf("%02x ", (unsigned int)buf[i]);
	//	printf("\n");
	//}

	memset(buf, 0, sizeof(buf)); // clean up to 0

	printf("Enabling and Disabling of Touch Solution\n");

	// Enabling and Disabling of Touch Solution. (DNSubCmd2 0x01). The first byte is the report number (0x03).
	buf[0] = 0x03; // report id
	buf[1] = 0x05; // length
	buf[2] = 0x36; // CmdMj // FIXED
	buf[3] = 0x91; // CmdMn // FIXED
	buf[4] = 0x10; // DNSubCmd1 // FIXED
	buf[5] = 0x01; // DNSubCmd2 // Enabling and Disabling of Touch Solution
	buf[6] = 0x01; // 0x00, disable, 0x01, enable

	printf("writing...\n");

	res = hid_write(handle, buf, 64);
	if (res < 0) {
		printf("Unable to write()/1: %ls\n", hid_error(handle));
	}

	if (res > 0) {
		printf("Data write:\n   ");
		// Print out the returned buffer.
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	memset(buf, 0, sizeof(buf)); // clean up to 0

	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	res = 0;
	i = 0;
	while (res == 0) {
		res = hid_read(handle, buf, sizeof(buf));
		if (res == 0) {
			printf("waiting...\n");
		}
		if (res < 0) {
			printf("Unable to read(): %ls\n", hid_error(handle));
			break;
		}

		i++;
		if (i >= 10) { /* 10 tries by 500 ms - 5 seconds of waiting*/
			printf("read() timeout\n");
			break;
		}

#ifdef _WIN32
		Sleep(500);
#else
		usleep(500 * 1000);
#endif
	}

	if (res > 0) {
		printf("Data read:\n   ");
		// Print out the returned buffer.
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	hid_close(handle);

	return 0;
}

// 0x222A - VID
// 0x546A - PID
int test_ilitek(void)
{
	int res;
	unsigned char buf[256];
#define MAX_STR 255
	//wchar_t wstr[MAX_STR];
	hid_device* handle;
	int i;

	printf("trying to open ilitek device\n");

	// Set up the command buffer.
	memset(buf, 0, sizeof(buf)); // clean up to 0
	buf[0] = 0x03; // report id 0x03

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	handle = hid_open_usage(0x222A, 0x546A, 0xFF00, 0x01); // Vendor-Defined 1
	if (!handle) {
		printf("unable to open ilitek device\n");
		hid_exit();
		return 1;
	}

	struct hid_device_info* info = hid_get_device_info(handle);
	if (info == NULL) {
		printf("Unable to get device info\n");
	}
	else {
		print_devices(info);
	}

	print_dev(handle);

	// Set the hid_read() function to be non-blocking.
	hid_set_nonblocking(handle, 1); // by default it is blocking

	// Try to read from the device. There should be no
	// data here, but execution should not block.
	res = hid_read(handle, buf, 64); // as we set nonblocking above

	memset(buf, 0, sizeof(buf)); // clean up to 0

	printf("Enabling and Disabling of Touch Report\n");

	// Enabling and Disabling of Touch Report. (Register 0x61). The first byte is the report number (0x03).
	buf[0] = 0x03; // report id
	buf[1] = 0xA3; // header
	buf[2] = 0x03; // write length // If data length is 2, means read current touch report status
	buf[3] = 0x02; // read length
	buf[4] = 0xFA; // command code
	buf[5] = 0x61; // register
	buf[6] = 0x00; // 0x00, enable, 0x01, disable

	printf("writing...\n");

	res = hid_write(handle, buf, 64);
	if (res < 0) {
		printf("Unable to write()/1: %ls\n", hid_error(handle));
	}

	if (res > 0) {
		printf("Data write:\n   ");
		// Print out the returned buffer.
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	memset(buf, 0, sizeof(buf)); // clean up to 0

	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	res = 0;
	i = 0;
	while (res == 0) {
		res = hid_read(handle, buf, sizeof(buf));
		if (res == 0) {
			printf("waiting...\n");
		}
		if (res < 0) {
			printf("Unable to read(): %ls\n", hid_error(handle));
			break;
		}

		i++;
		if (i >= 10) { /* 10 tries by 500 ms - 5 seconds of waiting*/
			printf("read() timeout\n");
			break;
		}

#ifdef _WIN32
		Sleep(500);
#else
		usleep(500 * 1000);
#endif
	}

	if (res > 0) {
		printf("Data read:\n   ");
		// Print out the returned buffer.
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int)buf[i]);
		printf("\n");
	}

	hid_close(handle);

	return 0;
}

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	printf("hidapi test/example tool. Compiled with hidapi version %s, runtime version %s.\n", HID_API_VERSION_STR, hid_version_str());
	if (HID_API_VERSION == HID_API_MAKE_VERSION(hid_version()->major, hid_version()->minor, hid_version()->patch)) {
		printf("Compile-time version matches runtime version of hidapi.\n\n");
	}
	else {
		printf("Compile-time version is different than runtime version of hidapi.\n]n");
	}

	if (hid_init())
		return -1;

#if defined(__APPLE__) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
	// To work properly needs to be called before hid_open/hid_open_path after hid_init.
	// Best/recommended option - call it right after hid_init.
	hid_darwin_set_open_exclusive(0);
#endif

	struct hid_device_info *devs;
	devs = hid_enumerate(0x0, 0x0);
	print_devices(devs);
	hid_free_enumeration(devs);

	if (test_tescis()) {
		return 1;
	}

	/* Free static HIDAPI objects. */
	hid_exit();

#ifdef _WIN32
	system("pause");
#endif

	return 0;
}
