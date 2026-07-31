/**
 * ESP_HID — ESP32-S3 USB HID 远程控制设备
 *
 * 功能：
 *  - ESP32-S3 通过 USB 连接被控电脑，模拟为 HID 鼠标 + 键盘
 *  - 连接 WiFi 后开启 TCP 服务器，接收 JSON 格式控制指令
 *  - 支持：鼠标绝对坐标移动、点击、滚轮、键盘字符串输入
 *
 * 指令格式（每行一个 JSON，以 \n 结尾）：
 *   {"cmd":"move",  "x":960,  "y":540}          // 移动到屏幕坐标
 *   {"cmd":"click", "btn":"left"}                // 点击 (left/right/middle)
 *   {"cmd":"press", "btn":"left"}                // 按下
 *   {"cmd":"release","btn":"left"}               // 释放
 *   {"cmd":"scroll","delta":-1}                  // 滚轮 (+上/-下)
 *   {"cmd":"type",  "text":"Hello World"}        // 输入字符串
 *   {"cmd":"key",   "key":"enter"}               // 特殊按键
 *   {"cmd":"key_combo","mod":"ctrl","key":"c"}   // 组合键
 *
 * 连接方式：telnet <ESP_IP> 8888 或使用 TCP socket
 *
 * 硬件接线:
 *  - USB Serial/JTAG 口 → 电脑 (用于查看调试串口输出)
 *  - USB OTG 口        → 被控电脑 (用于 HID 输入)
 */

#include <Arduino.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "AbsoluteMouse.h"
#include "CommandServer.h"

// 全局 HID 设备实例
USBHIDKeyboard Keyboard;
AbsoluteMouse  AbsMouse;
CommandServer CmdServer;

void setup() {
    // =====================================================
    // 第一步：初始化调试串口（HWCDC，USB Serial/JTAG 口）
    // 注意: 当 ARDUINO_USB_MODE=1 时，Serial 是 HWCDC，
    //       不依赖 TinyUSB，应该最先初始化以便输出诊断信息
    // =====================================================
    Serial.begin(115200);
    delay(1000);  // 等待 HWCDC 枚举

    Serial.println("\n\n=====================================");
    Serial.println("  ESP_HID - USB HID Remote Control");
    Serial.println("=====================================");
    Serial.printf("Platform: %s\n", ARDUINO_BOARD);
    Serial.printf("ARDUINO_USB_MODE=%d  ARDUINO_USB_CDC_ON_BOOT=%d\n",
                  ARDUINO_USB_MODE, ARDUINO_USB_CDC_ON_BOOT);
#if CONFIG_TINYUSB_HID_ENABLED
    Serial.println("CONFIG_TINYUSB_HID_ENABLED=1 (OK)");
#else
    Serial.println("CONFIG_TINYUSB_HID_ENABLED=0 (ERROR!)");
#endif
#if CONFIG_TINYUSB_ENABLED
    Serial.println("CONFIG_TINYUSB_ENABLED=1 (OK)");
#else
    Serial.println("CONFIG_TINYUSB_ENABLED=0 (ERROR!)");
#endif

    // =====================================================
    // 第二步：初始化 TinyUSB 协议栈
    // USB.begin() 内部调用 tinyusb_init()，加载所有已注册接口
    // HID 接口在全局对象的构造函数中已通过 USBHID 注册
    // =====================================================
    Serial.println("\n--- Initializing TinyUSB... ---");
    bool usbOk = USB.begin();
    Serial.printf("USB.begin() returned: %s\n", usbOk ? "true (OK)" : "false (FAILED!)");

    if (!usbOk) {
        Serial.println("FATAL: USB initialization failed! HID will not work.");
        Serial.println("Check USB OTG port connection and hardware.");
    }

    // 等待 USB 枚举完成
    Serial.println("Waiting for USB enumeration (3s)...");
    delay(3000);

    // 检查 USB 是否挂载（连接到主机）
    Serial.printf("USB device mounted: %s\n", (bool)USB ? "YES" : "NO (not connected?)");

    // =====================================================
    // 第三步：初始化 HID 设备信号量
    // =====================================================
    Serial.println("\n--- Initializing HID devices... ---");
    Keyboard.begin();
    AbsMouse.begin();
    Serial.println("Keyboard.begin() and AbsMouse.begin() done.");

    // 快速测试：发送一个空的HID报告看是否ready
    Serial.printf("USB HID ready (tud_hid_n_ready): %s\n",
                  tud_hid_n_ready(0) ? "YES" : "NO");

    // =====================================================
    // 第四步：连接 WiFi
    // =====================================================
    Serial.println("\n--- Starting WiFi & TCP server... ---");
    CmdServer.begin();

    Serial.println("\n=====================================");
    Serial.println("  System ready.");
    Serial.println("=====================================");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi IP: ");
        Serial.println(WiFi.localIP());
        Serial.printf("TCP port: %d\n", TCP_PORT);
    } else {
        Serial.println("WiFi NOT connected - check SSID/PASSWORD in CommandServer.h");
    }

    Serial.println("\nIf HID devices are NOT showing on the controlled PC:");
    Serial.println("  1. Make sure you are using the USB OTG port (NOT Serial/JTAG)");
    Serial.println("  2. Check the diagnostic messages above");
    Serial.println("  3. Try a different USB cable");
}

void loop() {
    CmdServer.loop();
    delay(1);
}
