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
  KEY_CENTER
} Key_t;

typedef enum
{
  MODE_AUTO = 0,
  MODE_MANUAL
} WorkMode_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
  #define LIGHT_BRIGHT_THRESHOLD_DEFAULT 1000
  #define LIGHT_MID_THRESHOLD_DEFAULT 2300
  #define LIGHT_DARK_THRESHOLD_DEFAULT 3200
  #define LIGHT_THRESHOLD_STEP 100
  #define ADC_SAMPLE_INTERVAL_MS 100
  #define LCD_REFRESH_INTERVAL_MS 500
  #define UART_REPORT_INTERVAL_MS 1000
  #define LCD_CHARS_PER_LINE 16
  #define LED_ON GPIO_PIN_RESET
  #define LED_OFF GPIO_PIN_SET
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static WorkMode_t g_work_mode = MODE_AUTO;
static uint8_t g_manual_level = 0;
static uint32_t g_light_bright_threshold = LIGHT_BRIGHT_THRESHOLD_DEFAULT;
static uint32_t g_light_mid_threshold = LIGHT_MID_THRESHOLD_DEFAULT;
static uint32_t g_light_dark_threshold = LIGHT_DARK_THRESHOLD_DEFAULT;
static uint32_t g_light_adc = 0;
static uint32_t g_key_adc = 0;
static uint8_t g_led_level = 0;

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
static const char *Key_To_String(Key_t key);
static const char *Mode_To_String(WorkMode_t mode);
static void Process_Key(Key_t key);
static void LCD_Show_Status(void);
static void LCD_Draw_TextLine(uint16_t y, const char *text);
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
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(CSS_GPIO_Port, CSS_Pin, GPIO_PIN_SET);

  Lcd_Init();
  Lcd_Clear(BLACK);
  LCD_Show_Status();
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

    Process_Key(uart_key);

    if (now - last_adc_sample_tick >= ADC_SAMPLE_INTERVAL_MS)
    {
      last_adc_sample_tick = now;
      g_light_adc = Read_Light_ADC();
      g_key_adc = Read_Key_ADC();
    }

    if (g_work_mode == MODE_AUTO)
    {
      g_led_level = Get_Light_Level(g_light_adc);
    }
    else
    {
      g_led_level = g_manual_level;
    }

    Set_Led_Level(g_led_level);

    if (now - last_lcd_refresh_tick >= LCD_REFRESH_INTERVAL_MS)
    {
      last_lcd_refresh_tick = now;
      LCD_Show_Status();
    }

    if (now - last_uart_report_tick >= UART_REPORT_INTERVAL_MS || uart_key != KEY_NONE)
    {
      char msg[180];

      last_uart_report_tick = now;
      sprintf(msg,
              "mode=%s, light=%lu, key_adc=%lu, uart_key=%s, level=%d, th=%lu/%lu/%lu\r\n",
              Mode_To_String(g_work_mode),
              g_light_adc,
              g_key_adc,
              Key_To_String(uart_key),
              g_led_level,
              g_light_bright_threshold,
              g_light_mid_threshold,
              g_light_dark_threshold);
      HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);
    }
  /* USER CODE END 3 */
  }
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

static Key_t Get_Key_By_UART(void)
{
  uint8_t rx_data = 0;
  uint8_t read_count = 0;
  Key_t key = KEY_NONE;

  while (read_count < 16 && HAL_UART_Receive(&huart1, &rx_data, 1, 0) == HAL_OK)
  {
    read_count++;

    if (rx_data == 'w' || rx_data == 'W')
    {
      key = KEY_UP;
    }
    else if (rx_data == 's' || rx_data == 'S')
    {
      key = KEY_DOWN;
    }
    else if (rx_data == 'a' || rx_data == 'A')
    {
      key = KEY_LEFT;
    }
    else if (rx_data == 'd' || rx_data == 'D')
    {
      key = KEY_RIGHT;
    }
    else if (rx_data == 'm' || rx_data == 'M')
    {
      key = KEY_CENTER;
    }
  }

  return key;
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

static void Process_Key(Key_t key)
{
  if (key == KEY_CENTER)
  {
    if (g_work_mode == MODE_AUTO)
    {
      g_work_mode = MODE_MANUAL;
      g_manual_level = 0;
    }
    else
    {
      g_work_mode = MODE_AUTO;
    }
  }
  else if (key == KEY_LEFT)
  {
    if (g_work_mode == MODE_MANUAL && g_manual_level > 0)
    {
      g_manual_level--;
    }
  }
  else if (key == KEY_RIGHT)
  {
    if (g_work_mode == MODE_MANUAL && g_manual_level < 3)
    {
      g_manual_level++;
    }
  }
  else if (key == KEY_UP)
  {
    g_light_bright_threshold += LIGHT_THRESHOLD_STEP;
    g_light_mid_threshold += LIGHT_THRESHOLD_STEP;
    g_light_dark_threshold += LIGHT_THRESHOLD_STEP;
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
}

static void LCD_Show_Status(void)
{
  char line[32];

  sprintf(line, "Mode:%s", Mode_To_String(g_work_mode));
  LCD_Draw_TextLine(0, line);

  sprintf(line, "Light:%lu", g_light_adc);
  LCD_Draw_TextLine(16, line);

  sprintf(line, "Level:%d", g_led_level);
  LCD_Draw_TextLine(32, line);

  sprintf(line, "T1:%lu T2:%lu",
          g_light_bright_threshold,
          g_light_mid_threshold);
  LCD_Draw_TextLine(48, line);

  sprintf(line, "T3:%lu",
          g_light_dark_threshold);
  LCD_Draw_TextLine(64, line);
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
