#include <Arduino.h>
#include "AbsoluteMouse.h"

#if CONFIG_TINYUSB_HID_ENABLED

// ============================================================
// 绝对坐标鼠标 HID 报告描述符
//
// Report ID: HID_REPORT_ID_VENDOR (7)
// 报告结构 (6 bytes):
//   Byte 0: 按键位图 (5 buttons + 3 padding bits)
//   Byte 1-2: 绝对X坐标 (0~32767, little-endian)
//   Byte 3-4: 绝对Y坐标 (0~32767, little-endian)
//   Byte 5: 滚轮 (-127~127)
// ============================================================
static const uint8_t abs_mouse_report_descriptor[] = {
    // Usage Page (Generic Desktop)
    0x05, 0x01,
    // Usage (Mouse)
    0x09, 0x02,
    // Collection (Application)
    0xA1, 0x01,
        // Report ID (HID_REPORT_ID_VENDOR = 7)
        0x85, HID_REPORT_ID_VENDOR,
        // Usage (Pointer)
        0x09, 0x01,
        // Collection (Physical)
        0xA1, 0x00,
            // --- 按键 (5 buttons) ---
            // Usage Page (Button)
            0x05, 0x09,
            // Usage Minimum (1)
            0x19, 0x01,
            // Usage Maximum (5)
            0x29, 0x05,
            // Logical Minimum (0)
            0x15, 0x00,
            // Logical Maximum (1)
            0x25, 0x01,
            // Report Count (5)
            0x95, 0x05,
            // Report Size (1)
            0x75, 0x01,
            // Input (Data, Variable, Absolute)
            0x81, 0x02,
            // --- 填充 3 bits ---
            // Report Count (1)
            0x95, 0x01,
            // Report Size (3)
            0x75, 0x03,
            // Input (Constant, Variable, Absolute)
            0x81, 0x01,

            // --- 绝对坐标 X, Y ---
            // Usage Page (Generic Desktop)
            0x05, 0x01,
            // Usage (X)
            0x09, 0x30,
            // Usage (Y)
            0x09, 0x31,
            // Logical Minimum (0)
            0x15, 0x00,
            // Logical Maximum (32767)
            0x26, 0xFF, 0x7F,
            // Physical Minimum (0)
            0x35, 0x00,
            // Physical Maximum (32767)
            0x46, 0xFF, 0x7F,
            // Report Size (16)
            0x75, 0x10,
            // Report Count (2)
            0x95, 0x02,
            // Input (Data, Variable, Absolute)
            0x81, 0x02,

            // --- 滚轮 ---
            // Usage (Wheel)
            0x09, 0x38,
            // Logical Minimum (-127)
            0x15, 0x81,
            // Logical Maximum (127)
            0x25, 0x7F,
            // Report Size (8)
            0x75, 0x08,
            // Report Count (1)
            0x95, 0x01,
            // Input (Data, Variable, Relative)
            0x81, 0x06,
        // End Collection (Physical)
        0xC0,
    // End Collection (Application)
    0xC0,
};

AbsoluteMouse::AbsoluteMouse() : hid(), _buttons(0) {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        hid.addDevice(this, sizeof(abs_mouse_report_descriptor));
    }
}

uint16_t AbsoluteMouse::_onGetDescriptor(uint8_t* dst) {
    memcpy(dst, abs_mouse_report_descriptor, sizeof(abs_mouse_report_descriptor));
    return sizeof(abs_mouse_report_descriptor);
}

void AbsoluteMouse::begin() {
    hid.begin();
}

void AbsoluteMouse::end() {
}

void AbsoluteMouse::move(uint16_t x, uint16_t y) {
    hid_abs_mouse_report_t report;
    report.buttons = _buttons;
    report.x       = x;
    report.y       = y;
    report.wheel   = 0;
    hid.SendReport(HID_REPORT_ID_VENDOR, &report, sizeof(report));
}

void AbsoluteMouse::scroll(int8_t delta) {
    hid_abs_mouse_report_t report;
    report.buttons = _buttons;
    report.x       = 0;
    report.y       = 0;
    report.wheel   = delta;
    hid.SendReport(HID_REPORT_ID_VENDOR, &report, sizeof(report));
}

void AbsoluteMouse::click(uint8_t b) {
    _buttons = b;
    move(0, 0);
    delay(10);
    _buttons = 0;
    move(0, 0);
}

void AbsoluteMouse::press(uint8_t b) {
    uint8_t old = _buttons;
    _buttons = _buttons | b;
    if (_buttons != old) {
        move(0, 0);
    }
}

void AbsoluteMouse::release(uint8_t b) {
    uint8_t old = _buttons;
    _buttons = _buttons & ~b;
    if (_buttons != old) {
        move(0, 0);
    }
}

bool AbsoluteMouse::isPressed(uint8_t b) {
    return (_buttons & b) > 0;
}

void AbsoluteMouse::releaseAll(void) {
    _buttons = 0;
    move(0, 0);
}

#endif // CONFIG_TINYUSB_HID_ENABLED
