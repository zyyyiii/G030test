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

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
  #define LIGHT_BRIGHT_THRESHOLD 500
  #define LIGHT_MID_THRESHOLD 1800
  #define LIGHT_DARK_THRESHOLD 2800
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

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

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t light_adc = Read_Light_ADC();
    uint32_t key_adc = Read_Key_ADC();
    uint8_t led_level = Get_Light_Level(light_adc);
    Key_t uart_key = Get_Key_By_UART();
    char msg[120];

    Set_Led_Level(led_level);

    sprintf(msg,
            "light=%lu, key_adc=%lu, uart_key=%s, level=%d\r\n",
            light_adc,
            key_adc,
            Key_To_String(uart_key),
            led_level);
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);

    HAL_Delay(300);
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
  if (adc_value < LIGHT_BRIGHT_THRESHOLD)
  {
    return 0;
  }
  else if (adc_value < LIGHT_MID_THRESHOLD)
  {
    return 1;
  }
  else if (adc_value < LIGHT_DARK_THRESHOLD)
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
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, level >= 1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, level >= 2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, level >= 3 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static Key_t Get_Key_By_UART(void)
{
  uint8_t rx_data = 0;

  if (HAL_UART_Receive(&huart1, &rx_data, 1, 0) == HAL_OK)
  {
    if (rx_data == 'w' || rx_data == 'W')
    {
      return KEY_UP;
    }
    else if (rx_data == 's' || rx_data == 'S')
    {
      return KEY_DOWN;
    }
    else if (rx_data == 'a' || rx_data == 'A')
    {
      return KEY_LEFT;
    }
    else if (rx_data == 'd' || rx_data == 'D')
    {
      return KEY_RIGHT;
    }
    else if (rx_data == 'm' || rx_data == 'M')
    {
      return KEY_CENTER;
    }
  }

  return KEY_NONE;
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
