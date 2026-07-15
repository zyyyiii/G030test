# STM32G030 Adaptive Light Control

基于 STM32G030C8T6 的自适应灯光控制实验工程。项目使用光敏电阻采集环境光，通过 STEP 分级亮灯或 PWM 渐变调光控制板载 LED，并通过 LCD、串口和 ESP8266 WiFi 模块显示或远程控制当前状态。

## 功能概览

- 光敏电阻 ADC 采样，环境越暗输出亮度越高
- STEP 模式：三颗 LED 按 0~3 级分级亮灭
- PWM 模式：使用 TIM14 定时中断实现软件 PWM，目前用于蓝色 LED 渐变调光
- 支持 AUTO / MANUAL 两种工作模式
- 支持 STEP / PWM 两种输出模式自由切换
- 支持串口命令调试和 LCD 状态显示
- 支持 ESP8266 AT 指令转发、TCP Server 远程控制
- 支持阈值写入片内 Flash，掉电后仍可加载
- PWM 亮度使用 gamma 表校正，使手动调光观感更均匀

## 当前模式

系统由两个独立模式组合：

| 工作模式 | 输出模式 | 行为 |
| --- | --- | --- |
| AUTO | STEP | 根据光照阈值自动控制 LED 数量 |
| MANUAL | STEP | 通过串口手动调节 LED 等级 0~3 |
| AUTO | PWM | 根据光照自动映射 PWM 亮度 |
| MANUAL | PWM | 通过串口手动调节 PWM 档位 0~20 |

## 本地串口命令

| 命令 | 功能 |
| --- | --- |
| `m` | 切换 AUTO / MANUAL |
| `g` | 切换 STEP / PWM |
| `a` | MANUAL 模式下降低亮度 |
| `d` | MANUAL 模式下提高亮度 |
| `w` | 提高自动模式阈值 |
| `s` | 降低自动模式阈值 |
| `p` | 保存当前阈值到 Flash |
| `r` | 恢复默认阈值到 RAM，若需永久保存需再发送 `p` |

本地串口使用 USART1，连接电脑串口助手。除了单字符控制命令外，串口助手还可以通过 `@` 前缀向 ESP8266 转发 AT 指令：

```text
@AT
@AT+GMR
@AT+CWMODE=1
@AT+CWJAP="WiFi名","WiFi密码"
@AT+CIFSR
```

串口输出会显示：

```text
mode=AUTO, output=PWM, light=..., key_adc=..., uart_key=..., level=..., pwm=..., pwm_level=..., th=.../.../...
```

其中：

- `light`：光敏电阻 ADC 值
- `level`：STEP 模式亮度等级，范围 0~3
- `pwm`：实际 PWM 占空比值，范围 0~200
- `pwm_level`：手动/自动映射后的亮度档位，范围 0~20
- `th`：自动模式三个光照阈值

## 主要硬件连接

| 功能 | 引脚/外设 |
| --- | --- |
| 光敏电阻 | ADC1 IN4 / PA4 |
| 五向按键 ADC | ADC1 IN1 / PA1 |
| USART1 TX | PA9 |
| USART1 RX | PA10 |
| ESP8266 TX/RX | USART2 / PA2 PA3 |
| LED2 | PB2 |
| LED3 | PB1 |
| LED4 | PB0 |
| LCD SCLK | PB3 |
| LCD MOSI | PB5 |
| LCD DC/RS | PB4 |
| LCD CS | PA15 |
| LCD 控制脚 | PB6 |
| 软件 PWM 定时器 | TIM14 |

USART1 用于电脑串口助手调试，USART2 用于 STM32 与 ESP8266 模块通信。USART1 和 USART2 都使用中断接收缓冲区，避免长 AT 指令或 ESP8266 回复丢字。

LED 为低电平点亮，代码中使用：

```c
#define LED_ON  GPIO_PIN_RESET
#define LED_OFF GPIO_PIN_SET
```

## PWM 调光

PWM 模式使用 TIM14 基础定时器中断实现软件 PWM：

- TIM14 Prescaler：`15`
- TIM14 Period：`49`
- 定时中断周期约 `50us`
- 单周期 PWM 步数：`200`
- PWM 周期约 `10ms`，频率约 `100Hz`

手动 PWM 并不直接线性调节 0~200，而是调节 0~20 的亮度档位，再通过 gamma 表转换到实际 PWM 值：

```c
static const uint16_t g_pwm_gamma_table[21] =
{
  0, 1, 2, 4, 7,
  11, 16, 23, 31, 41,
  53, 66, 81, 98, 116,
  135, 154, 172, 186, 195,
  200
};
```

这张表不是 LED 的精确物理参数，而是根据人眼亮度感知规律给出的初始校正曲线，可根据实测观感继续微调。

## LCD 显示

LCD 当前显示：

```text
Mode:AUTO PWM
Light:xxxx
Level:x PWM:y
T1:xxxx T2:xxxx
T3:xxxx
Config SAVED / Save FAILED / Output PWM ...
```

## 阈值保存

阈值保存使用 STM32 片内 Flash 最后一页。

保存流程：

1. 发送 `w/s` 调整阈值
2. 发送 `p` 写入 Flash
3. 写入后立即读回并校验
4. LCD 显示 `Config SAVED` 或 `Save FAILED`

注意：Flash 有擦写寿命，不建议连续频繁发送 `p`。

## WiFi 远程控制

板载 WiFi 模块为 ESP-12F / ESP8266，STM32 通过 USART2 使用 AT 指令控制它。电脑串口助手仍连接 USART1，通过 `@` 前缀把 AT 指令转发给 ESP8266。

### 1. 连接 WiFi

建议先使用 2.4GHz、WPA2-PSK、英文/数字名称和密码的热点。

```text
@AT+CWMODE=1
@AT+CWJAP="Xiaomi6","11111111"
@AT+CIFSR
```

连接成功后会看到：

```text
WIFI CONNECTED
WIFI GOT IP
OK
```

`AT+CIFSR` 会返回 ESP8266 的 IP，例如：

```text
+CIFSR:STAIP,"192.168.123.240"
```

如果返回 `STAIP,"0.0.0.0"` 或 `No AP`，表示尚未连接 WiFi。若能 `CWLAP` 扫描到热点但 `CWJAP` 返回 `FAIL`，优先检查热点是否为 2.4GHz、WPA2、是否允许新设备接入，以及 ESP8266 供电是否稳定。

### 2. 开启 TCP Server

连接 WiFi 后，在串口助手发送：

```text
@AT+CIPMUX=1
@AT+CIPSERVER=1,8080
```

`no change` 也可以接受，表示配置已经处于目标状态。

### 3. TCP 客户端连接

电脑或手机需要和 ESP8266 处于同一 WiFi/热点下。以串口助手的 TCP Client 功能为例：

```text
模式：TCP-C
远端 IP：AT+CIFSR 查询到的 STAIP，例如 192.168.123.240
远端端口：8080
本地 IP：电脑 WLAN 的 IPv4 地址，例如 192.168.123.187
本地端口：0
```

连接成功后，在 TCP 客户端发送单字符即可远程控制：

| TCP 发送 | 功能 |
| --- | --- |
| `m` | 切换 AUTO / MANUAL |
| `g` | 切换 STEP / PWM |
| `a` | 降低手动亮度 |
| `d` | 提高手动亮度 |
| `w` | 提高阈值 |
| `s` | 降低阈值 |
| `p` | 保存阈值到 Flash |
| `r` | 恢复默认阈值到 RAM |

ESP8266 收到 TCP 数据后会通过 USART2 输出类似：

```text
+IPD,0,1:m
```

STM32 解析 `+IPD` 中冒号后的命令字符，并复用本地 `Process_Key()` 控制逻辑。COM 串口窗口会显示：

```text
wifi remote: CENTER
```

表示远程命令已经被 STM32 识别。

## 构建环境

- STM32CubeMX
- Keil MDK-ARM
- ARM Compiler 5
- STM32G0 HAL 库

工程入口：

```text
G030test/G030test.ioc
G030test/MDK-ARM/G030test.uvprojx
```

构建时请使用 Keil 打开 `G030test.uvprojx`，选择目标 `G030test` 编译并烧录。

## 调试建议

1. 先确认串口正常输出状态信息
2. 在 AUTO + STEP 下验证光照阈值与 LED 数量
3. 在 MANUAL + STEP 下验证 `a/d` 控制等级
4. 在 AUTO + PWM 下观察光照变化是否影响 PWM
5. 在 MANUAL + PWM 下观察 `pwm_level` 和蓝色 LED 渐变
6. 调好阈值后发送 `p` 保存
7. 使用 `@AT` 验证 STM32 与 ESP8266 通信，期望回复 `OK`
8. 使用 `@AT+CIFSR` 确认 ESP8266 获取到有效 IP
9. 开启 TCP Server 后，用 TCP Client 发送 `m/g/a/d` 验证远程控制

## 说明

本工程仍处于实验和调试阶段。当前 PWM 主要用于验证板载 LED 的渐变效果，若需要更平滑或更接近实际灯光应用的效果，后续可考虑使用硬件 PWM 或外接 LED 驱动模块。当前 WiFi 远程控制基于 ESP8266 AT 固件和 TCP Server，适合局域网调试演示；如需正式联网应用，可进一步封装协议、加入连接状态显示和自动重连逻辑。
