# STM32G030 Adaptive Light Control

基于 STM32G030C8T6 的自适应灯光控制实验工程。项目使用光敏电阻采集环境光，通过 STEP 分级亮灯或 PWM 渐变调光控制板载 LED，并通过 LCD 与串口显示当前状态。

## 功能概览

- 光敏电阻 ADC 采样，环境越暗输出亮度越高
- STEP 模式：三颗 LED 按 0~3 级分级亮灭
- PWM 模式：使用 TIM14 定时中断实现软件 PWM，目前用于蓝色 LED 渐变调光
- 支持 AUTO / MANUAL 两种工作模式
- 支持 STEP / PWM 两种输出模式自由切换
- 支持串口命令调试和 LCD 状态显示
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

## 串口命令

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
| LED2 | PB2 |
| LED3 | PB1 |
| LED4 | PB0 |
| LCD SCLK | PB3 |
| LCD MOSI | PB5 |
| LCD DC/RS | PB4 |
| LCD CS | PA15 |
| LCD 控制脚 | PB6 |
| 软件 PWM 定时器 | TIM14 |

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

## 说明

本工程仍处于实验和调试阶段。当前 PWM 主要用于验证板载 LED 的渐变效果，若需要更平滑或更接近实际灯光应用的效果，后续可考虑使用硬件 PWM 或外接 LED 驱动模块。
