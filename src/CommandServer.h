#pragma once
#include <Arduino.h>
#include <WiFi.h>

// =====================================================
// 用户配置 — 请修改为你的实际WiFi信息
// =====================================================
#define WIFI_SSID       "Your_SSID"
#define WIFI_PASSWORD   "Your_Password"

// TCP服务器端口
#define TCP_PORT        8888

// 被控设备的屏幕分辨率（用于绝对坐标转换）
#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   600

// JSON接收缓冲区大小
#define JSON_BUF_SIZE   512

// =====================================================
// 命令类型枚举
// =====================================================
enum CmdType {
    CMD_NONE,
    CMD_MOVE,       // 移动鼠标到绝对坐标
    CMD_CLICK,      // 鼠标点击
    CMD_PRESS,      // 鼠标按下
    CMD_RELEASE,    // 鼠标释放
    CMD_SCROLL,     // 滚轮
    CMD_TYPE,       // 输入字符串
    CMD_KEY,        // 发送单个按键
    CMD_KEY_COMBO,  // 组合键
};

// 解析后的命令
struct ParsedCommand {
    CmdType type;
    uint16_t x, y;          // 用于MOVE
    uint8_t  btn;           // 用于CLICK/PRESS/RELEASE: "left"/"right"/"middle"
    int8_t   scroll_delta;  // 用于SCROLL
    char     text[256];     // 用于TYPE
    uint8_t  key;           // 用于KEY
    uint8_t  modifier;      // 用于KEY_COMBO
};

// =====================================================
// WiFi + TCP 命令服务器
// =====================================================
class CommandServer {
private:
    WiFiServer* _server;
    WiFiClient  _client;
    char        _jsonBuf[JSON_BUF_SIZE];
    int         _bufPos;

    // 解析JSON命令
    bool parseCommand(const char* json, ParsedCommand& cmd);
    // 解析按键名称为HID码
    uint8_t keyNameToCode(const char* name);
    // 解析按键名称为修饰键码
    uint8_t keyNameToModifier(const char* name);

public:
    CommandServer();
    void begin();
    // 主循环中调用，检查新连接和接收数据
    void loop();
    // WiFi是否已连接
    bool isWiFiConnected();
};
