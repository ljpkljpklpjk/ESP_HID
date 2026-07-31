#pragma once
#include "USBHID.h"

#if CONFIG_TINYUSB_HID_ENABLED

// 绝对坐标鼠标按键定义（与USBHIDMouse兼容）
#define ABS_MOUSE_LEFT      0x01
#define ABS_MOUSE_RIGHT     0x02
#define ABS_MOUSE_MIDDLE    0x04
#define ABS_MOUSE_BACKWARD  0x08
#define ABS_MOUSE_FORWARD   0x10
#define ABS_MOUSE_ALL       0x1F

// 绝对坐标鼠标HID报告结构（不含report ID）
typedef struct __attribute__((packed)) {
    uint8_t  buttons;   // 按键位图
    uint16_t x;         // 绝对X坐标 (0 ~ 32767)
    uint16_t y;         // 绝对Y坐标 (0 ~ 32767)
    int8_t   wheel;     // 滚轮 (-127 ~ 127)
} hid_abs_mouse_report_t;

class AbsoluteMouse : public USBHIDDevice {
private:
    USBHID  hid;
    uint8_t _buttons;

public:
    AbsoluteMouse(void);
    void begin(void);
    void end(void);

    // 鼠标移动：x/y为绝对坐标(0~32767)，映射到整个屏幕
    void move(uint16_t x, uint16_t y);

    // 滚轮：delta正数向上滚，负数向下滚
    void scroll(int8_t delta);

    // 按键操作
    void click(uint8_t b = ABS_MOUSE_LEFT);
    void press(uint8_t b = ABS_MOUSE_LEFT);
    void release(uint8_t b = ABS_MOUSE_LEFT);
    bool isPressed(uint8_t b = ABS_MOUSE_LEFT);
    void releaseAll(void);

    // 内部使用：返回HID报告描述符
    uint16_t _onGetDescriptor(uint8_t* buffer);
};

#endif // CONFIG_TINYUSB_HID_ENABLED
