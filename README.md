# ESP_HID — ESP32-S3 网络 HID 远程控制器

基于 ESP32-S3 的 USB HID 远程控制设备。通过 WiFi 接收指令，经 USB 模拟键盘+鼠标控制目标电脑。

## 功能

| 功能 | 说明 |
|------|------|
| **绝对坐标移动** | 屏幕坐标自动转换为 HID 绝对坐标 (0~32767) |
| **鼠标点击** | 支持左/中/右键的 click / press / release |
| **滚轮滚动** | 正数向上、负数向下 |
| **键盘输入** | ASCII 字符串直接输入 |
| **特殊按键** | Enter / Esc / Tab / F1~F12 / 方向键等 |
| **组合键** | Ctrl / Shift / Alt / Win + 任意键 |

## 硬件连接

ESP32-S3 有两个 USB-C 口：

| 接口 | 连接到 | 作用 |
|------|--------|------|
| **USB Serial/JTAG** | 开发电脑 | 烧录固件 + 串口调试 |
| **USB OTG** | 被控电脑 | HID 键盘 + HID 绝对坐标鼠标 |

## 网络协议

TCP 端口 `8888`，每行一条 JSON 命令，以 `\n` 结尾。

### 命令格式

```json
{"cmd":"move",       "x":960,  "y":540}
{"cmd":"click",      "btn":"left"}
{"cmd":"click",      "btn":"right"}
{"cmd":"click",      "btn":"middle"}
{"cmd":"press",      "btn":"left"}
{"cmd":"release",    "btn":"left"}
{"cmd":"scroll",     "delta":-1}
{"cmd":"type",       "text":"Hello World"}
{"cmd":"key",        "key":"enter"}
{"cmd":"key_combo",  "mod":"ctrl", "key":"c"}
```

### 特殊按键名称

`enter` `esc` `tab` `backspace` `space` `up` `down` `left` `right` `home` `end` `pageup` `pagedown` `insert` `delete` `capslock` `f1`~`f12`

### 修饰键名称

`ctrl` `shift` `alt` `gui` (Windows 键)

## 快速开始

### 1. 配置 WiFi 和屏幕分辨率

编辑 `src/CommandServer.h`：

```cpp
#define WIFI_SSID       "你的WiFi名称"
#define WIFI_PASSWORD   "你的WiFi密码"
#define SCREEN_WIDTH    1920    // 被控电脑屏幕宽
#define SCREEN_HEIGHT   1080    // 被控电脑屏幕高
```

### 2. 编译上传

```bash
pio run -t upload
```

### 3. 连接

将 USB OTG 口连接被控电脑，USB Serial/JTAG 口连接开发电脑查看串口（波特率 115200）。

### 4. 发送指令

```bash
telnet <ESP32的IP> 8888
```

粘贴 JSON 命令即可控制。或用 Python：

```python
import socket, json, time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(("192.168.1.100", 8888))

def send(cmd):
    sock.sendall((json.dumps(cmd) + "\n").encode())
    time.sleep(0.05)

send({"cmd": "move",  "x": 960, "y": 540})
send({"cmd": "click", "btn": "left"})
send({"cmd": "type",  "text": "Hello from ESP32!"})
send({"cmd": "key",   "key": "enter"})

sock.close()
```

## 项目结构

```
ESP_HID/
├── platformio.ini              # PlatformIO 配置
├── src/
│   ├── main.cpp                # 主入口，初始化 USB / WiFi / TCP
│   ├── AbsoluteMouse.h/.cpp    # 自定义绝对坐标 HID 鼠标
│   └── CommandServer.h/.cpp    # WiFi 连接 + TCP 服务器 + JSON 命令解析
└── README.md
```

## 构建要求

- [PlatformIO](https://platformio.org/) (推荐) 或 Arduino IDE
- 开发板：ESP32-S3 (如 ESP32-S3 Box)
- 框架：Arduino (espressif32)
- USB 库：内置 TinyUSB

## 注意事项

- ESP32-S3 上 `ARDUINO_USB_MODE=1` 时需**手动调用 `USB.begin()`**，框架不会自动初始化 TinyUSB
- USB OTG 口与 USB Serial/JTAG 口是独立的物理接口，HID 必须走 OTG 口
- 绝对坐标范围 0~32767，由屏幕分辨率自动换算
- 同一时间只支持一个 TCP 客户端连接

## License

MIT
