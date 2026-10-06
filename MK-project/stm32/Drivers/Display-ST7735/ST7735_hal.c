/*
ST7735_hal.c
Привязка драйвера ST7735 к STM32 HAL
*/

#include "ST7735.h"
#include "main.h"

/*
1. Настройка пинов (согласно MX_GPIO_Init в main.c)
Пины уже настроены в main.c:
PA3 - DC (Data/Command)
PA4 - CS (Chip Select)
PA6 - RESET
*/

#define LCD_DC_PIN GPIO_PIN_3
#define LCD_DC_PORT GPIOA
#define LCD_CS_PIN GPIO_PIN_4
#define LCD_CS_PORT GPIOA
#define LCD_RESET_PIN GPIO_PIN_6
#define LCD_RESET_PORT GPIOA

// Внешняя ссылка на SPI (объявлен в main.c)
extern SPI_HandleTypeDef hspi1;

/*
2. Callback-функции для драйвера
Отправка команды (DC = 0)
cmd: байт команды
*/

static void ST7735_HAL_SendCmd(uint8_t cmd)
{
  // DC = 0 (команда)
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);

  // CS = 0 (выбираем SPI устройство)
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);

  // Отправка команды через SPI
  HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);

  // CS = 1 (отключаем)
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/*
Отправка данных (DC = 1)
data: указатель на данные
len: количество байт
*/

static void ST7735_HAL_SendData(uint8_t* data, uint16_t len)
{
  // DC = 1 (данные)
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);

  // CS = 0
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);

  // Отправка данных через SPI
  HAL_SPI_Transmit(&hspi1, data, len, HAL_MAX_DELAY);

  // CS = 1
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/*
Задержка в миллисекундах
ms: время задержки
*/

static void ST7735_HAL_Delay(uint32_t ms)
{
  HAL_Delay(ms);
}

/*
Управление пином RESET
state: 0 - сброс, 1 - отпустить сброс
*/
static void ST7735_HAL_Reset(uint8_t state)
{
  HAL_GPIO_WritePin(LCD_RESET_PORT, LCD_RESET_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/*
3. Публичная функция инициализации дисплея
Инициализация дисплея с привязкой к HAL
handle: указатель на структуру драйвера
Вызовите эту функцию после MX_GPIO_Init() и MX_SPI1_Init()
*/

void ST7735_HAL_Init(ST7735_Handle* handle)
{
  // Регистрируем callback-функции
  handle->send_cmd = ST7735_HAL_SendCmd;
  handle->send_data = ST7735_HAL_SendData;
  handle->delay_ms = ST7735_HAL_Delay;
  handle->reset = ST7735_HAL_Reset;

  // Инициализируем драйвер
  ST7735_Init(handle);
}