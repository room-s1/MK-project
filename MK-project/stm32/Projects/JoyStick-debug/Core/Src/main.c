/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : KY-023 JOYSTICK - WORKING VERSION
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "KY023.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define JOYSTICK_SW_PIN GPIO_PIN_2
#define JOYSTICK_SW_PORT GPIOA
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* USER CODE BEGIN PV */
char msg[100];
uint16_t x_raw = 0;
uint16_t y_raw = 0;
KY023_HandleTypeDef ky023;

volatile uint8_t button_state = 1;
volatile uint32_t last_debounce = 0;
volatile uint32_t trigger_x_count = 0;
volatile uint32_t trigger_y_count = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);

/* USER CODE BEGIN PFP */
void usb_printf(const char* format, ...);
void read_joystick(void);
void check_button(void);

// Коллбеки
void on_x_positive(KY023_HandleTypeDef* ky);
void on_x_negative(KY023_HandleTypeDef* ky);
void on_y_positive(KY023_HandleTypeDef* ky);
void on_y_negative(KY023_HandleTypeDef* ky);
void on_x_trigger(KY023_HandleTypeDef* ky);
void on_y_trigger(KY023_HandleTypeDef* ky);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void usb_printf(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int len = vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);

  if (len > 0 && len < sizeof(msg))
  {
    CDC_Transmit_FS((uint8_t*)msg, len);
    HAL_Delay(20);
  }
}

void read_joystick(void)
{
  // Читаем X (PA0)
  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
  {
    x_raw = HAL_ADC_GetValue(&hadc1);
  }

  // Читаем Y (PA1) - меняем канал
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
  {
    y_raw = HAL_ADC_GetValue(&hadc1);
  }

  // Возвращаем канал на X для следующего цикла
  sConfig.Channel = ADC_CHANNEL_0;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

void check_button(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t reading = HAL_GPIO_ReadPin(JOYSTICK_SW_PORT, JOYSTICK_SW_PIN);

  if (reading != button_state)
  {
    if ((now - last_debounce) > 50)
    {
      button_state = reading;
      if (button_state == 0)
      {
        usb_printf("[BUTTON PRESSED]\r\n");
      }
      last_debounce = now;
    }
  }
}

void on_x_positive(KY023_HandleTypeDef* ky)
{
  usb_printf("[RIGHT] %d%%\r\n", KY023_GetX(ky));
}

void on_x_negative(KY023_HandleTypeDef* ky)
{
  usb_printf("[LEFT] %d%%\r\n", KY023_GetX(ky));
}

void on_y_positive(KY023_HandleTypeDef* ky)
{
  usb_printf("[UP] %d%%\r\n", KY023_GetY(ky));
}

void on_y_negative(KY023_HandleTypeDef* ky)
{
  usb_printf("[DOWN] %d%%\r\n", KY023_GetY(ky));
}

void on_x_trigger(KY023_HandleTypeDef* ky)
{
  trigger_x_count++;
  usb_printf("\r\n*** X TRIGGER #%lu! %d%% ***\r\n\r\n", trigger_x_count, KY023_GetX(ky));
}

void on_y_trigger(KY023_HandleTypeDef* ky)
{
  trigger_y_count++;
  usb_printf("\r\n*** Y TRIGGER #%lu! %d%% ***\r\n\r\n", trigger_y_count, KY023_GetY(ky));
}

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USB_DEVICE_Init();

  /* USER CODE BEGIN 2 */

  // Мигаем 3 раза
  for (int i = 0; i < 3; i++)
  {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_Delay(200);
  }

  HAL_Delay(2000);

  // Инициализация KY023
  KY023_Init(&ky023, 5);

  // Регистрация коллбеков
  KY023_OnXPositive = on_x_positive;
  KY023_OnXNegative = on_x_negative;
  KY023_OnYPositive = on_y_positive;
  KY023_OnYNegative = on_y_negative;
  KY023_OnXTrigger = on_x_trigger;
  KY023_OnYTrigger = on_y_trigger;

  // Настройка триггеров
  KY023_SetXTrigger(&ky023, 70, 15);
  KY023_SetYTrigger(&ky023, 70, 15);

  usb_printf("\r\n================================\r\n");
  usb_printf("KY-023 JOYSTICK READY!\r\n");
  usb_printf("Deadzone: 5%% | Trigger: 70%%\r\n");
  usb_printf("================================\r\n\r\n");

  HAL_Delay(500);

  /* USER CODE END 2 */

  uint32_t last_print = 0;

  while (1)
  {
    // Читаем ADC
    read_joystick();

    // Обновляем джойстик
    KY023_SetRawValues(&ky023, x_raw, y_raw);
    KY023_Update(&ky023);

    // Проверяем кнопку
    check_button();

    // Выводим сырые данные каждые 500ms
    if ((HAL_GetTick() - last_print) > 500)
    {
      usb_printf("[RAW] X:%4d Y:%4d | [NORM] X:%3d%% Y:%3d%%\r\n", x_raw, y_raw, KY023_GetX(&ky023),
                 KY023_GetY(&ky023));
      last_print = HAL_GetTick();
    }

    // Мигаем LED
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    HAL_Delay(100);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  HAL_ADC_Init(&hadc1);

  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

static void MX_DMA_Init(void)
{
  __HAL_RCC_DMA2_CLK_ENABLE();
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  // Кнопка джойстика
  GPIO_InitStruct.Pin = JOYSTICK_SW_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(JOYSTICK_SW_PORT, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(100);
  }
}