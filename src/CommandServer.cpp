#include "CommandServer.h"
#include "AbsoluteMouse.h"
#include "USBHIDKeyboard.h"
#include "USB.h"
#include "USBHID.h"

// 日志同时输出到 HWCDC(Serial) 与板载 UART0(Serial0/CH340)，
// 便于未连接 USB Serial/JTAG 口时通过 CH340 (COM13) 查看日志
#define LOG(...) do { Serial.printf(__VA_ARGS__); Serial0.printf(__VA_ARGS__); } while (0)

// 外部全局HID对象（在main.cpp中定义）
extern AbsoluteMouse  AbsMouse;
extern USBHIDKeyboard Keyboard;

// =====================================================
// 简易 JSON 解析器（避免引入大型库，节省Flash）
// =====================================================

// 从JSON字符串中提取指定key的字符串值
static String jsonGetString(const char* json, const char* key) {
    String search = "\"";
    search += key;
    search += "\"";
    const char* pos = strstr(json, search.c_str());
    if (!pos) return "";
    pos = strchr(pos + search.length(), ':');
    if (!pos) return "";
    pos = strchr(pos, '"');
    if (!pos) return "";
    const char* end = strchr(pos + 1, '"');
    if (!end) return "";
    String result;
    result.concat(pos + 1, end - pos - 1);
    return result;
}

// 从JSON字符串中提取指定key的整数值
static int jsonGetInt(const char* json, const char* key, int defaultVal = 0) {
    String search = "\"";
    search += key;
    search += "\"";
    const char* pos = strstr(json, search.c_str());
    if (!pos) return defaultVal;
    pos = strchr(pos + search.length(), ':');
    if (!pos) return defaultVal;
    pos++; // 跳过 ':'
    while (*pos == ' ' || *pos == '\t') pos++;
    return atoi(pos);
}

// =====================================================
// 按键名称 -> HID 码 映射
// =====================================================
uint8_t CommandServer::keyNameToCode(const char* name) {
    struct KeyMap { const char* name; uint8_t code; };
    static const KeyMap keymap[] = {
        {"enter",    0xB0}, {"return",   0xB0},
        {"esc",      0xB1}, {"escape",   0xB1},
        {"backspace",0xB2}, {"tab",      0xB3},
        {"space",    0x20}, {" ",        0x20},
        {"up",       0xDA}, {"down",     0xD9},
        {"left",     0xD8}, {"right",    0xD7},
        {"insert",   0xD1}, {"delete",   0xD4},
        {"home",     0xD2}, {"end",      0xD5},
        {"pageup",   0xD3}, {"pagedown", 0xD6},
        {"capslock", 0xC1},
        {"f1",0xC2},  {"f2",0xC3},  {"f3",0xC4},  {"f4",0xC5},
        {"f5",0xC6},  {"f6",0xC7},  {"f7",0xC8},  {"f8",0xC9},
        {"f9",0xCA},  {"f10",0xCB}, {"f11",0xCC}, {"f12",0xCD},
        {nullptr, 0}
    };
    for (int i = 0; keymap[i].name; i++) {
        if (strcasecmp(name, keymap[i].name) == 0) {
            return keymap[i].code;
        }
    }
    return 0;
}

uint8_t CommandServer::keyNameToModifier(const char* name) {
    if (strcasecmp(name, "ctrl") == 0)  return KEY_LEFT_CTRL;
    if (strcasecmp(name, "shift") == 0) return KEY_LEFT_SHIFT;
    if (strcasecmp(name, "alt") == 0)   return KEY_LEFT_ALT;
    if (strcasecmp(name, "gui") == 0)   return KEY_LEFT_GUI;
    if (strcasecmp(name, "win") == 0)   return KEY_LEFT_GUI;
    return 0;
}

// =====================================================
// JSON 命令解析
// =====================================================
bool CommandServer::parseCommand(const char* json, ParsedCommand& cmd) {
    memset(&cmd, 0, sizeof(cmd));

    String cmdStr = jsonGetString(json, "cmd");
    if (cmdStr.length() == 0) return false;

    if (cmdStr == "move") {
        cmd.type = CMD_MOVE;
        cmd.x = jsonGetInt(json, "x", 0);
        cmd.y = jsonGetInt(json, "y", 0);
    }
    else if (cmdStr == "click") {
        cmd.type = CMD_CLICK;
        String btn = jsonGetString(json, "btn");
        if (btn == "right")      cmd.btn = ABS_MOUSE_RIGHT;
        else if (btn == "middle") cmd.btn = ABS_MOUSE_MIDDLE;
        else                      cmd.btn = ABS_MOUSE_LEFT;
    }
    else if (cmdStr == "press") {
        cmd.type = CMD_PRESS;
        String btn = jsonGetString(json, "btn");
        if (btn == "right")      cmd.btn = ABS_MOUSE_RIGHT;
        else if (btn == "middle") cmd.btn = ABS_MOUSE_MIDDLE;
        else                      cmd.btn = ABS_MOUSE_LEFT;
    }
    else if (cmdStr == "release") {
        cmd.type = CMD_RELEASE;
        String btn = jsonGetString(json, "btn");
        if (btn == "right")      cmd.btn = ABS_MOUSE_RIGHT;
        else if (btn == "middle") cmd.btn = ABS_MOUSE_MIDDLE;
        else                      cmd.btn = ABS_MOUSE_LEFT;
    }
    else if (cmdStr == "scroll") {
        cmd.type = CMD_SCROLL;
        cmd.scroll_delta = (int8_t)jsonGetInt(json, "delta", 0);
    }
    else if (cmdStr == "type") {
        cmd.type = CMD_TYPE;
        String text = jsonGetString(json, "text");
        int len = text.length();
        if (len > 250) len = 250;
        memcpy(cmd.text, text.c_str(), len);
        cmd.text[len] = '\0';
    }
    else if (cmdStr == "key") {
        cmd.type = CMD_KEY;
        String key = jsonGetString(json, "key");
        cmd.key = keyNameToCode(key.c_str());
        if (cmd.key == 0 && key.length() == 1) {
            cmd.key = key[0];
        }
    }
    else if (cmdStr == "key_combo") {
        cmd.type = CMD_KEY_COMBO;
        String mod = jsonGetString(json, "mod");
        cmd.modifier = keyNameToModifier(mod.c_str());
        String key = jsonGetString(json, "key");
        cmd.key = keyNameToCode(key.c_str());
        if (cmd.key == 0 && key.length() == 1) {
            cmd.key = key[0];
        }
    }
    else {
        return false;
    }
    return true;
}

// =====================================================
// 执行解析后的命令（前置声明给loop使用）
// =====================================================
static void executeCommand(const ParsedCommand& cmd) {
    switch (cmd.type) {
    case CMD_MOVE: {
        uint16_t hx = (uint16_t)((uint32_t)cmd.x * 32767 / SCREEN_WIDTH);
        uint16_t hy = (uint16_t)((uint32_t)cmd.y * 32767 / SCREEN_HEIGHT);
        AbsMouse.move(hx, hy);
        Serial.printf("  MOVE screen(%d,%d) -> HID(%d,%d)\n", cmd.x, cmd.y, hx, hy);
        break;
    }
    case CMD_CLICK: {
        AbsMouse.click(cmd.btn);
        Serial.printf("  CLICK btn=%d\n", cmd.btn);
        break;
    }
    case CMD_PRESS: {
        AbsMouse.press(cmd.btn);
        Serial.printf("  PRESS btn=%d\n", cmd.btn);
        break;
    }
    case CMD_RELEASE: {
        AbsMouse.release(cmd.btn);
        Serial.printf("  RELEASE btn=%d\n", cmd.btn);
        break;
    }
    case CMD_SCROLL: {
        AbsMouse.scroll(cmd.scroll_delta);
        Serial.printf("  SCROLL delta=%d\n", cmd.scroll_delta);
        break;
    }
    case CMD_TYPE: {
        // 逐字符 press/release 并加延时：USB HID 键盘报告单缓冲，
        // 连续快速发送会使部分报告因端点忙碌而丢失（tud_hid_n_report
        // 返回 false 但框架不检查返回值），导致键卡住污染后续命令。
        const char* p = cmd.text;
        int n = 0;
        while (*p) {
            if (*p == '\\' && *(p + 1) == 'n') {
                // 字面 "\n" 转义为 Enter（0x0A -> _asciimap -> HID 0x28）
                Keyboard.press(0x0A);
                delay(20);
                Keyboard.release(0x0A);
                delay(20);
                p += 2;
                n++;
            } else {
                Keyboard.press((uint8_t)*p);   // 按下（自动处理 ASCII/Shift）
                delay(20);                      // 等按下报告发送完成
                Keyboard.release((uint8_t)*p);  // 释放
                delay(20);                      // 等释放报告发送完成
                p++;
                n++;
            }
        }
        Keyboard.releaseAll();              // 兜底：清空所有残留键
        delay(20);
        LOG("  TYPE \"%s\" (%d chars)\n", cmd.text, n);
        break;
    }
    case CMD_KEY: {
        if (cmd.key) {
            Keyboard.write(cmd.key);
            LOG("  KEY code=%02X\n", cmd.key);
        }
        break;
    }
    case CMD_KEY_COMBO: {
        if (cmd.modifier && cmd.key) {
            Keyboard.releaseAll();
            delay(20);
            Keyboard.press(cmd.modifier);
            delay(20);
            Keyboard.write(cmd.key);
            delay(20);
            Keyboard.release(cmd.modifier);
            delay(20);
            LOG("  KEY_COMBO mod=%02X key=%02X\n", cmd.modifier, cmd.key);
        }
        break;
    }
    default:
        break;
    }
}

// =====================================================
// 构造函数
// =====================================================
CommandServer::CommandServer()
    : _server(nullptr), _client(), _bufPos(0)
{
    memset(_jsonBuf, 0, sizeof(_jsonBuf));
}

// =====================================================
// 初始化 WiFi 和 TCP 服务器
// =====================================================
void CommandServer::begin() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi connection failed! Continuing anyway...");
    }

    _server = new WiFiServer(TCP_PORT);
    _server->begin();
    Serial.print("TCP server listening on port ");
    Serial.println(TCP_PORT);
}

// =====================================================
// 检查 WiFi 连接状态
// =====================================================
bool CommandServer::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// =====================================================
// 主循环：接收并处理TCP命令
// =====================================================
void CommandServer::loop() {
    // 处理新客户端连接
    if (!_client || !_client.connected()) {
        _client = _server->accept();
        if (_client) {
            Serial.println("Client connected");
            _bufPos = 0;
        }
    }

    // 接收数据
    if (_client && _client.connected()) {
        while (_client.available() > 0) {
            char c = _client.read();

            if (c == '\n' || c == '\r') {
                if (_bufPos > 0 && (c == '\n' || _client.peek() != '\n')) {
                    _jsonBuf[_bufPos] = '\0';

                    LOG("Received: %s\n", _jsonBuf);

                    ParsedCommand cmd;
                    if (parseCommand(_jsonBuf, cmd)) {
                        executeCommand(cmd);
                    } else {
                        LOG("  -> Unknown command\n");
                    }

                    _bufPos = 0;
                }
            } else if (_bufPos < JSON_BUF_SIZE - 1) {
                _jsonBuf[_bufPos++] = c;
            }
        }

        if (!_client.connected()) {
            Serial.println("Client disconnected");
            _client.stop();
            _bufPos = 0;
        }
    }
}
