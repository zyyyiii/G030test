/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  KEY_NONE = 0,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_CENTER,
  KEY_SAVE,
  KEY_RESET_DEFAULT,
  KEY_OUTPUT_MODE
} Key_t;

typedef enum
{
  MODE_AUTO = 0,
  MODE_MANUAL
} WorkMode_t;

typedef enum
{
  OUTPUT_STEP = 0,
  OUTPUT_PWM
} LightOutputMode_t;

typedef struct
{
  uint32_t magic;
  uint32_t version;
  uint32_t bright_threshold;
  uint32_t mid_threshold;
  uint32_t dark_threshold;
  uint32_t checksum;
} AppConfig_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
  #define LIGHT_BRIGHT_THRESHOLD_DEFAULT 1000
  #define LIGHT_MID_THRESHOLD_DEFAULT 2300
  #define LIGHT_DARK_THRESHOLD_DEFAULT 3200
  #define LIGHT_THRESHOLD_STEP 100
  #define LIGHT_THRESHOLD_MAX 4095
  #define ADC_SAMPLE_INTERVAL_MS 100
  #define LCD_REFRESH_INTERVAL_MS 500
  #define UART_REPORT_INTERVAL_MS 1000
  #define LCD_MESSAGE_HOLD_MS 2000
  #define SOFT_PWM_LED_STEPS 200
  #define SOFT_PWM_TOTAL_STEPS SOFT_PWM_LED_STEPS
  #define PWM_LEVEL_MAX 20
  #define UART1_RX_BUFFER_SIZE 128
  #define UART_RX_DRAIN_LIMIT 128
  #define WIFI_AT_CMD_MAX_LEN 96
  #define WIFI_AT_INPUT_TIMEOUT_MS 1000
  #define UART2_RX_BUFFER_SIZE 256
  #define WIFI_AT_REPLY_TIMEOUT_MS 12000
  #define WIFI_AT_REPLY_IDLE_MS 120
  #define WIFI_JOIN_REPLY_TIMEOUT_MS 30000
  #define WIFI_JOIN_REPLY_IDLE_MS WIFI_JOIN_REPLY_TIMEOUT_MS
  #define LCD_CHARS_PER_LINE 16
  #define LED_ON GPIO_PIN_RESET
  #define LED_OFF GPIO_PIN_SET
  #define CONFIG_MAGIC 0x20260713U
  #define CONFIG_VERSION 1U
  #define CONFIG_FLASH_ADDRESS (FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static WorkMode_t g_work_mode = MODE_AUTO;
static uint8_t g_manual_level = 0;
static uint8_t g_manual_pwm_level = 0;
static uint32_t g_light_bright_threshold = LIGHT_BRIGHT_THRESHOLD_DEFAULT;
static uint32_t g_light_mid_threshold = LIGHT_MID_THRESHOLD_DEFAULT;
static uint32_t g_light_dark_threshold = LIGHT_DARK_THRESHOLD_DEFAULT;
static uint32_t g_light_adc = 0;
static uint32_t g_key_adc = 0;
static uint8_t g_led_level = 0;
static volatile LightOutputMode_t g_output_mode = OUTPUT_STEP;
static volatile uint16_t g_pwm_brightness = 0;
static uint8_t g_uart1_rx_byte = 0;
static volatile uint8_t g_uart1_rx_head = 0;
static volatile uint8_t g_uart1_rx_tail = 0;
static volatile uint8_t g_uart1_rx_buffer[UART1_RX_BUFFER_SIZE];
static uint8_t g_uart2_rx_byte = 0;
static volatile uint16_t g_uart2_rx_head = 0;
static volatile uint16_t g_uart2_rx_tail = 0;
static volatile uint8_t g_uart2_rx_buffer[UART2_RX_BUFFER_SIZE];
static const char *g_lcd_message = "";
static uint32_t g_lcd_message_until = 0;
static const uint16_t g_pwm_gamma_table[PWM_LEVEL_MAX + 1] =
{
  0, 1, 2, 4, 7,
  11, 16, 23, 31, 41,
  53, 66, 81, 98, 116,
  135, 154, 172, 186, 195,
  200
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static uint32_t Read_ADC_Channel(uint32_t channel);
static uint32_t Read_Light_ADC(void);
static uint32_t Read_Key_ADC(void);
static uint8_t Get_Light_Level(uint32_t adc_value);
static void Set_Led_Level(uint8_t level);
static Key_t Get_Key_By_UART(void);
static Key_t Get_Key_By_WIFI(void);
static Key_t Command_Char_To_Key(uint8_t data);
static const char *Key_To_String(Key_t key);
static const char *Mode_To_String(WorkMode_t mode);
static const char *Output_Mode_To_String(LightOutputMode_t mode);
static void Process_Key(Key_t key);
static void LCD_Show_Status(void);
static void LCD_Draw_TextLine(uint16_t y, const char *text);
static void LCD_Set_Message(const char *message, uint32_t hold_ms);
static uint16_t PWM_Level_To_Duty(uint8_t level);
static uint8_t PWM_Duty_To_Level(uint16_t duty);
static uint16_t Get_PWM_Brightness(uint32_t adc_value);
static void Soft_PWM_Update(void);
static uint32_t Config_Calc_Checksum(const AppConfig_t *config);
static uint8_t Config_Is_Valid(const AppConfig_t *config);
static void Config_Load(void);
static uint8_t Config_Save(void);
static void Config_Reset_Defaults(void);
static void UART1_Start_Receive_IT(void);
static void UART1_Buffer_Push(uint8_t data);
static uint8_t UART1_Read_Byte(uint8_t *data);
static void UART2_Start_Receive_IT(void);
static void UART2_Buffer_Push(uint8_t data);
static uint8_t UART2_Read_Byte(uint8_t *data);
static void UART1_Send_String(const char *text);
static void WIFI_Read_And_Send_AT_Command(void);
static void WIFI_Send_AT_Command(const char *at_command);
static void WIFI_Forward_Response(uint32_t timeout_ms, uint32_t idle_ms);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM14_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  Config_Load();

  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
  UART1_Start_Receive_IT();
  UART2_Start_Receive_IT();

  if (HAL_TIM_Base_Start_IT(&htim14) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_GPIO_WritePin(CSS_GPIO_Port, CSS_Pin, GPIO_PIN_SET);

  Lcd_Init();
  Lcd_Clear(BLACK);
  LCD_Show_Status();
  UART1_Send_String("\r\nLocal: m/g/a/d/w/s/p/r, WiFi AT line parser v2: @AT+GMR\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();
    static uint32_t last_adc_sample_tick = 0;
    static uint32_t last_lcd_refresh_tick = 0;
    static uint32_t last_uart_report_tick = 0;
    Key_t uart_key = Get_Key_By_UART();
    Key_t wifi_key = Get_Key_By_WIFI();
    Key_t active_key = uart_key != KEY_NONE ? uart_key : wifi_key;

    Process_Key(active_key);

    if (now - last_adc_sample_tick >= ADC_SAMPLE_INTERVAL_MS)
    {
      uint32_t raw_light_adc;

      last_adc_sample_tick = now;
      raw_light_adc = Read_Light_ADC();
      if (g_light_adc == 0)
      {
        g_light_adc = raw_light_adc;
      }
      else
      {
        g_light_adc = (g_light_adc * 3U + raw_light_adc) / 4U;
      }
      g_key_adc = Read_Key_ADC();
    }

    if (g_work_mode == MODE_AUTO)
    {
      g_led_level = Get_Light_Level(g_light_adc);
      g_pwm_brightness = Get_PWM_Brightness(g_light_adc);
    }
    else
    {
      g_led_level = g_manual_level;
      g_pwm_brightness = PWM_Level_To_Duty(g_manual_pwm_level);
    }

    if (g_output_mode == OUTPUT_STEP)
    {
      Set_Led_Level(g_led_level);
    }

    if (now - last_lcd_refresh_tick >= LCD_REFRESH_INTERVAL_MS)
    {
      last_lcd_refresh_tick = now;
      LCD_Show_Status();
    }

    if (now - last_uart_report_tick >= UART_REPORT_INTERVAL_MS || active_key != KEY_NONE)
    {
      char msg[220];

      last_uart_report_tick = now;
      sprintf(msg,
              "mode=%s, output=%s, light=%lu, key_adc=%lu, uart_key=%s, level=%d, pwm=%d, pwm_level=%d, th=%lu/%lu/%lu\r\n",
              Mode_To_String(g_work_mode),
              Output_Mode_To_String(g_output_mode),
              g_light_adc,
              g_key_adc,
              Key_To_String(active_key),
              g_led_level,
              g_pwm_brightness,
              PWM_Duty_To_Level(g_pwm_brightness),
              g_light_bright_threshold,
              g_light_mid_threshold,
              g_light_dark_threshold);
      HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);
    }
  }
 /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static uint32_t Read_ADC_Channel(uint32_t channel)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  uint32_t adc_value = 0;

  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_ADC_Start(&hadc1);

  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
  {
    adc_value = HAL_ADC_GetValue(&hadc1);
  }

  HAL_ADC_Stop(&hadc1);

  return adc_value;
}

static uint32_t Read_Light_ADC(void)
{
  return Read_ADC_Channel(ADC_CHANNEL_4);
}

static uint32_t Read_Key_ADC(void)
{
  return Read_ADC_Channel(ADC_CHANNEL_1);
}

static uint8_t Get_Light_Level(uint32_t adc_value)
{
  if (adc_value < g_light_bright_threshold)
  {
    return 0;
  }
  else if (adc_value < g_light_mid_threshold)
  {
    return 1;
  }
  else if (adc_value < g_light_dark_threshold)
  {
    return 2;
  }
  else
  {
    return 3;
  }
}

static void Set_Led_Level(uint8_t level)
{
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, level >= 1 ? LED_ON : LED_OFF);
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, level >= 2 ? LED_ON : LED_OFF);
  HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, level >= 3 ? LED_ON : LED_OFF);
}

static uint16_t PWM_Level_To_Duty(uint8_t level)
{
  if (level > PWM_LEVEL_MAX)
  {
    level = PWM_LEVEL_MAX;
  }

  return g_pwm_gamma_table[level];
}

static uint8_t PWM_Duty_To_Level(uint16_t duty)
{
  uint8_t level;

  for (level = 0; level < PWM_LEVEL_MAX; level++)
  {
    if (duty <= g_pwm_gamma_table[level])
    {
      return level;
    }
  }

  return PWM_LEVEL_MAX;
}

static uint16_t Get_PWM_Brightness(uint32_t adc_value)
{
  uint32_t span;
  uint32_t level;

  if (adc_value <= g_light_bright_threshold)
  {
    return PWM_Level_To_Duty(0);
  }

  if (adc_value >= g_light_dark_threshold)
  {
    return PWM_Level_To_Duty(PWM_LEVEL_MAX);
  }

  span = g_light_dark_threshold - g_light_bright_threshold;
  level = ((adc_value - g_light_bright_threshold) * PWM_LEVEL_MAX + span / 2U) / span;

  return PWM_Level_To_Duty((uint8_t)level);
}

static void Soft_PWM_Update(void)
{
  static uint16_t pwm_step = 0;
  uint16_t brightness;

  if (g_output_mode != OUTPUT_PWM)
  {
    return;
  }

  pwm_step++;
  if (pwm_step >= SOFT_PWM_LED_STEPS)
  {
    pwm_step = 0;
  }

  brightness = g_pwm_brightness;
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, LED_OFF);
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, pwm_step < brightness ? LED_ON : LED_OFF);
  HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, LED_OFF);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM14)
  {
    Soft_PWM_Update();
  }
}

static Key_t Get_Key_By_UART(void)
{
  uint8_t rx_data = 0;
  uint8_t read_count = 0;
  Key_t key = KEY_NONE;

  while (read_count < UART_RX_DRAIN_LIMIT && UART1_Read_Byte(&rx_data))
  {
    read_count++;

    if (rx_data == '@')
    {
      WIFI_Read_And_Send_AT_Command();
    }
    else
    {
      Key_t rx_key = Command_Char_To_Key(rx_data);

      if (rx_key != KEY_NONE)
      {
        key = rx_key;
      }
    }
  }

  return key;
}

static Key_t Get_Key_By_WIFI(void)
{
  static uint8_t prefix_index = 0;
  static uint8_t reading_header = 0;
  static uint16_t ipd_length = 0;
  static uint16_t ipd_data_left = 0;
  const char prefix[] = "+IPD,";
  uint8_t rx_data = 0;
  Key_t key = KEY_NONE;

  while (UART2_Read_Byte(&rx_data))
  {
    if (ipd_data_left > 0)
    {
      key = Command_Char_To_Key(rx_data);
      ipd_data_left--;

      if (key != KEY_NONE)
      {
        char msg[32];

        sprintf(msg, "\r\nwifi remote: %s\r\n", Key_To_String(key));
        UART1_Send_String(msg);
        return key;
      }

      continue;
    }

    if (reading_header)
    {
      if (rx_data >= '0' && rx_data <= '9')
      {
        ipd_length = (uint16_t)(ipd_length * 10U + rx_data - '0');
      }
      else if (rx_data == ',')
      {
        ipd_length = 0;
      }
      else if (rx_data == ':')
      {
        ipd_data_left = ipd_length > 0 ? ipd_length : 1;
        reading_header = 0;
        ipd_length = 0;
      }
      else
      {
        reading_header = 0;
        ipd_length = 0;
      }

      continue;
    }

    if (rx_data == (uint8_t)prefix[prefix_index])
    {
      prefix_index++;
      if (prefix[prefix_index] == '\0')
      {
        prefix_index = 0;
        reading_header = 1;
        ipd_length = 0;
      }
    }
    else
    {
      prefix_index = rx_data == '+' ? 1 : 0;
    }
  }

  return KEY_NONE;
}

static Key_t Command_Char_To_Key(uint8_t data)
{
  if (data == 'w' || data == 'W')
  {
    return KEY_UP;
  }
  else if (data == 's' || data == 'S')
  {
    return KEY_DOWN;
  }
  else if (data == 'a' || data == 'A')
  {
    return KEY_LEFT;
  }
  else if (data == 'd' || data == 'D')
  {
    return KEY_RIGHT;
  }
  else if (data == 'm' || data == 'M')
  {
    return KEY_CENTER;
  }
  else if (data == 'p' || data == 'P')
  {
    return KEY_SAVE;
  }
  else if (data == 'r' || data == 'R')
  {
    return KEY_RESET_DEFAULT;
  }
  else if (data == 'g' || data == 'G')
  {
    return KEY_OUTPUT_MODE;
  }

  return KEY_NONE;
}

static void UART1_Send_String(const char *text)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)text, strlen(text), 100);
}

static void UART1_Start_Receive_IT(void)
{
  if (HAL_UART_Receive_IT(&huart1, &g_uart1_rx_byte, 1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void UART1_Buffer_Push(uint8_t data)
{
  uint8_t next_head = (uint8_t)((g_uart1_rx_head + 1U) % UART1_RX_BUFFER_SIZE);

  if (next_head != g_uart1_rx_tail)
  {
    g_uart1_rx_buffer[g_uart1_rx_head] = data;
    g_uart1_rx_head = next_head;
  }
}

static uint8_t UART1_Read_Byte(uint8_t *data)
{
  if (g_uart1_rx_tail == g_uart1_rx_head)
  {
    return 0;
  }

  *data = g_uart1_rx_buffer[g_uart1_rx_tail];
  g_uart1_rx_tail = (uint8_t)((g_uart1_rx_tail + 1U) % UART1_RX_BUFFER_SIZE);

  return 1;
}

static void UART2_Start_Receive_IT(void)
{
  if (HAL_UART_Receive_IT(&huart2, &g_uart2_rx_byte, 1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void UART2_Buffer_Push(uint8_t data)
{
  uint16_t next_head = (uint16_t)((g_uart2_rx_head + 1U) % UART2_RX_BUFFER_SIZE);

  if (next_head != g_uart2_rx_tail)
  {
    g_uart2_rx_buffer[g_uart2_rx_head] = data;
    g_uart2_rx_head = next_head;
  }
}

static uint8_t UART2_Read_Byte(uint8_t *data)
{
  if (g_uart2_rx_tail == g_uart2_rx_head)
  {
    return 0;
  }

  *data = g_uart2_rx_buffer[g_uart2_rx_tail];
  g_uart2_rx_tail = (uint16_t)((g_uart2_rx_tail + 1U) % UART2_RX_BUFFER_SIZE);

  return 1;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    UART1_Buffer_Push(g_uart1_rx_byte);
    UART1_Start_Receive_IT();
  }
  else if (huart->Instance == USART2)
  {
    UART2_Buffer_Push(g_uart2_rx_byte);
    UART2_Start_Receive_IT();
  }
}

static void WIFI_Read_And_Send_AT_Command(void)
{
  char command[WIFI_AT_CMD_MAX_LEN + 1];
  uint8_t rx_data = 0;
  uint8_t len = 0;
  uint32_t start_tick = HAL_GetTick();

  while (len < WIFI_AT_CMD_MAX_LEN
         && HAL_GetTick() - start_tick < WIFI_AT_INPUT_TIMEOUT_MS)
  {
    if (UART1_Read_Byte(&rx_data))
    {
      if (rx_data == '\r' || rx_data == '\n')
      {
        break;
      }

      command[len++] = (char)rx_data;
    }
    else
    {
      HAL_Delay(1);
    }
  }

  command[len] = '\0';
  UART1_Send_String("\r\nwifi cmd ready\r\n");
  WIFI_Send_AT_Command(len == 0 ? "AT" : command);
}

static void WIFI_Send_AT_Command(const char *at_command)
{
  char command[WIFI_AT_CMD_MAX_LEN + 3];
  uint8_t len = strlen(at_command);
  uint8_t discard = 0;
  uint32_t reply_timeout = WIFI_AT_REPLY_TIMEOUT_MS;
  uint32_t reply_idle = WIFI_AT_REPLY_IDLE_MS;

  if (len > WIFI_AT_CMD_MAX_LEN)
  {
    len = WIFI_AT_CMD_MAX_LEN;
  }

  memcpy(command, at_command, len);
  command[len] = '\0';

  UART1_Send_String("\r\nwifi tx: ");
  UART1_Send_String(command);
  UART1_Send_String("\r\nwifi rx:\r\n");

  while (UART2_Read_Byte(&discard))
  {
  }

  command[len++] = '\r';
  command[len++] = '\n';
  HAL_UART_Transmit(&huart2, (uint8_t *)command, len, 1000);

  if (strncmp(at_command, "AT+CWJAP", 8) == 0 || strncmp(at_command, "AT+CWLAP", 8) == 0)
  {
    reply_timeout = WIFI_JOIN_REPLY_TIMEOUT_MS;
    reply_idle = WIFI_JOIN_REPLY_IDLE_MS;
  }

  WIFI_Forward_Response(reply_timeout, reply_idle);
}

static void WIFI_Forward_Response(uint32_t timeout_ms, uint32_t idle_ms)
{
  uint8_t rx_data = 0;
  uint8_t got_response = 0;
  uint32_t start_tick = HAL_GetTick();
  uint32_t last_rx_tick = start_tick;

  while (HAL_GetTick() - start_tick < timeout_ms)
  {
    if (UART2_Read_Byte(&rx_data))
    {
      got_response = 1;
      last_rx_tick = HAL_GetTick();
      HAL_UART_Transmit(&huart1, &rx_data, 1, 100);
    }
    else if (got_response && HAL_GetTick() - last_rx_tick >= idle_ms)
    {
      break;
    }
    else
    {
      HAL_Delay(1);
    }
  }

  if (!got_response)
  {
    UART1_Send_String("(timeout/no response)\r\n");
  }

  UART1_Send_String("\r\n(wifi rx end)\r\n");
}

static const char *Key_To_String(Key_t key)
{
  switch (key)
  {
    case KEY_UP:
      return "UP";
    case KEY_DOWN:
      return "DOWN";
    case KEY_LEFT:
      return "LEFT";
    case KEY_RIGHT:
      return "RIGHT";
    case KEY_CENTER:
      return "CENTER";
    case KEY_SAVE:
      return "SAVE";
    case KEY_RESET_DEFAULT:
      return "RESET";
    case KEY_OUTPUT_MODE:
      return "OUTPUT";
    default:
      return "NONE";
  }
}

static const char *Mode_To_String(WorkMode_t mode)
{
  if (mode == MODE_MANUAL)
  {
    return "MANUAL";
  }

  return "AUTO";
}

static const char *Output_Mode_To_String(LightOutputMode_t mode)
{
  if (mode == OUTPUT_PWM)
  {
    return "PWM";
  }

  return "STEP";
}

static void Process_Key(Key_t key)
{
  if (key == KEY_CENTER)
  {
    if (g_work_mode == MODE_AUTO)
    {
      g_work_mode = MODE_MANUAL;
      g_manual_level = g_led_level;
      g_manual_pwm_level = PWM_Duty_To_Level(g_pwm_brightness);
    }
    else
    {
      g_work_mode = MODE_AUTO;
    }
  }
  else if (key == KEY_LEFT)
  {
    if (g_work_mode == MODE_MANUAL && g_output_mode == OUTPUT_PWM)
    {
      if (g_manual_pwm_level > 0)
      {
        g_manual_pwm_level--;
      }
    }
    else if (g_work_mode == MODE_MANUAL && g_manual_level > 0)
    {
      g_manual_level--;
    }
  }
  else if (key == KEY_RIGHT)
  {
    if (g_work_mode == MODE_MANUAL && g_output_mode == OUTPUT_PWM)
    {
      if (g_manual_pwm_level < PWM_LEVEL_MAX)
      {
        g_manual_pwm_level++;
      }
    }
    else if (g_work_mode == MODE_MANUAL && g_manual_level < 3)
    {
      g_manual_level++;
    }
  }
  else if (key == KEY_UP)
  {
    if (g_light_dark_threshold + LIGHT_THRESHOLD_STEP <= LIGHT_THRESHOLD_MAX)
    {
      g_light_bright_threshold += LIGHT_THRESHOLD_STEP;
      g_light_mid_threshold += LIGHT_THRESHOLD_STEP;
      g_light_dark_threshold += LIGHT_THRESHOLD_STEP;
    }
  }
  else if (key == KEY_DOWN)
  {
    if (g_light_bright_threshold > LIGHT_THRESHOLD_STEP)
    {
      g_light_bright_threshold -= LIGHT_THRESHOLD_STEP;
    }
    if (g_light_mid_threshold > LIGHT_THRESHOLD_STEP)
    {
      g_light_mid_threshold -= LIGHT_THRESHOLD_STEP;
    }
    if (g_light_dark_threshold > LIGHT_THRESHOLD_STEP)
    {
      g_light_dark_threshold -= LIGHT_THRESHOLD_STEP;
    }
  }
  else if (key == KEY_SAVE)
  {
    if (Config_Save())
    {
      LCD_Set_Message("Config SAVED", LCD_MESSAGE_HOLD_MS);
    }
    else
    {
      LCD_Set_Message("Save FAILED", LCD_MESSAGE_HOLD_MS);
    }
    LCD_Show_Status();
  }
  else if (key == KEY_RESET_DEFAULT)
  {
    Config_Reset_Defaults();
    LCD_Set_Message("Default RAM", LCD_MESSAGE_HOLD_MS);
    LCD_Show_Status();
  }
  else if (key == KEY_OUTPUT_MODE)
  {
    if (g_output_mode == OUTPUT_STEP)
    {
      g_output_mode = OUTPUT_PWM;
      LCD_Set_Message("Output PWM", LCD_MESSAGE_HOLD_MS);
    }
    else
    {
      g_output_mode = OUTPUT_STEP;
      LCD_Set_Message("Output STEP", LCD_MESSAGE_HOLD_MS);
    }
    LCD_Show_Status();
  }
}

static uint32_t Config_Calc_Checksum(const AppConfig_t *config)
{
  return config->magic
         ^ config->version
         ^ config->bright_threshold
         ^ config->mid_threshold
         ^ config->dark_threshold
         ^ 0xA5A5A5A5U;
}

static uint8_t Config_Is_Valid(const AppConfig_t *config)
{
  if (config->magic != CONFIG_MAGIC || config->version != CONFIG_VERSION)
  {
    return 0;
  }

  if (config->checksum != Config_Calc_Checksum(config))
  {
    return 0;
  }

  if (config->bright_threshold >= config->mid_threshold
      || config->mid_threshold >= config->dark_threshold
      || config->dark_threshold > LIGHT_THRESHOLD_MAX)
  {
    return 0;
  }

  return 1;
}

static void Config_Load(void)
{
  const AppConfig_t *config = (const AppConfig_t *)CONFIG_FLASH_ADDRESS;

  if (Config_Is_Valid(config))
  {
    g_light_bright_threshold = config->bright_threshold;
    g_light_mid_threshold = config->mid_threshold;
    g_light_dark_threshold = config->dark_threshold;
  }
  else
  {
    Config_Reset_Defaults();
  }
}

static uint8_t Config_Save(void)
{
  AppConfig_t config;
  FLASH_EraseInitTypeDef erase_init = {0};
  uint32_t page_error = 0;
  uint32_t page = (CONFIG_FLASH_ADDRESS - FLASH_BASE) / FLASH_PAGE_SIZE;
  uint32_t words[sizeof(AppConfig_t) / sizeof(uint32_t)];
  uint32_t i;
  uint8_t save_ok = 0;
  const AppConfig_t *saved_config = (const AppConfig_t *)CONFIG_FLASH_ADDRESS;

  config.magic = CONFIG_MAGIC;
  config.version = CONFIG_VERSION;
  config.bright_threshold = g_light_bright_threshold;
  config.mid_threshold = g_light_mid_threshold;
  config.dark_threshold = g_light_dark_threshold;
  config.checksum = Config_Calc_Checksum(&config);

  memcpy(words, &config, sizeof(config));

  HAL_FLASH_Unlock();

  erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
  erase_init.Banks = FLASH_BANK_1;
  erase_init.Page = page;
  erase_init.NbPages = 1;

  if (HAL_FLASHEx_Erase(&erase_init, &page_error) == HAL_OK)
  {
    save_ok = 1;

    for (i = 0; i < sizeof(AppConfig_t) / 8U; i++)
    {
      uint64_t data = ((uint64_t)words[i * 2U + 1U] << 32) | words[i * 2U];

      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                            CONFIG_FLASH_ADDRESS + i * 8U,
                            data) != HAL_OK)
      {
        save_ok = 0;
        break;
      }
    }
  }

  HAL_FLASH_Lock();

  if (save_ok
      && Config_Is_Valid(saved_config)
      && saved_config->bright_threshold == g_light_bright_threshold
      && saved_config->mid_threshold == g_light_mid_threshold
      && saved_config->dark_threshold == g_light_dark_threshold)
  {
    return 1;
  }

  return 0;
}

static void Config_Reset_Defaults(void)
{
  g_light_bright_threshold = LIGHT_BRIGHT_THRESHOLD_DEFAULT;
  g_light_mid_threshold = LIGHT_MID_THRESHOLD_DEFAULT;
  g_light_dark_threshold = LIGHT_DARK_THRESHOLD_DEFAULT;
}

static void LCD_Show_Status(void)
{
  char line[32];
  const char *message = "";

  sprintf(line, "Mode:%s %s",
          Mode_To_String(g_work_mode),
          Output_Mode_To_String(g_output_mode));
  LCD_Draw_TextLine(0, line);

  sprintf(line, "Light:%lu", g_light_adc);
  LCD_Draw_TextLine(16, line);

  sprintf(line, "Level:%d PWM:%d",
          g_led_level,
          g_pwm_brightness);
  LCD_Draw_TextLine(32, line);

  sprintf(line, "T1:%lu T2:%lu",
          g_light_bright_threshold,
          g_light_mid_threshold);
  LCD_Draw_TextLine(48, line);

  sprintf(line, "T3:%lu",
          g_light_dark_threshold);
  LCD_Draw_TextLine(64, line);

  if (HAL_GetTick() < g_lcd_message_until)
  {
    message = g_lcd_message;
  }
  LCD_Draw_TextLine(80, message);
}

static void LCD_Draw_TextLine(uint16_t y, const char *text)
{
  char display[LCD_CHARS_PER_LINE + 1];
  uint8_t i;

  memset(display, ' ', LCD_CHARS_PER_LINE);
  display[LCD_CHARS_PER_LINE] = '\0';

  for (i = 0; i < LCD_CHARS_PER_LINE && text[i] != '\0'; i++)
  {
    display[i] = text[i];
  }

  Gui_DrawFont_GBK16(0, y, WHITE, BLACK, (uint8_t *)display);
}

static void LCD_Set_Message(const char *message, uint32_t hold_ms)
{
  g_lcd_message = message;
  g_lcd_message_until = HAL_GetTick() + hold_ms;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
