/*
Реализация драйвера ST7735
*/

#include "ST7735.h"
#include <string.h>

/*
1. Вспомогательные функции
 Отправка только команды
 handle: указатель на структуру драйвера
 cmd: команда
 */

static void ST7735_SendCommand(ST7735_Handle* handle, uint8_t cmd)
{
  handle->send_cmd(cmd);
}

/*
Отправка команды с данными
handle: указатель на структуру драйвера
cmd: команда
data: указатель на данные
len: длина данных
*/

static void ST7735_SendCommandWithData(ST7735_Handle* handle, uint8_t cmd, uint8_t* data, uint16_t len)
{
  handle->send_cmd(cmd);
  if (len > 0 && data != NULL)
  {
    handle->send_data(data, len);
  }
}

/*
2. Инициализация дисплея
Инициализация дисплея ST7735
handle: указатель на структуру драйвера
*/

void ST7735_Init(ST7735_Handle* handle)
{
  // Аппаратный сброс
  handle->reset(0);       // RESET низкий
  handle->delay_ms(10);   // Задержка
  handle->reset(1);       // RESET высокий
  handle->delay_ms(120);  // Ждём выхода из сброса

  // Физический сброс
  ST7735_SendCommand(handle, ST7735_SWRESET);
  handle->delay_ms(150);

  // Выход из сна
  ST7735_SendCommand(handle, ST7735_SLPOUT);
  handle->delay_ms(120);

  // Настройка цвета: 16 бит на пиксель (RGB565)
  uint8_t colmod_data = 0x05;
  ST7735_SendCommandWithData(handle, ST7735_COLMOD, &colmod_data, 1);
  handle->delay_ms(10);

  // Настройка ориентации по умолчанию (портрет, BGR порядок)
  ST7735_MADCTL_Reg madctl;
  madctl.byte = 0x00;
  madctl.bits.BGR = 1;  // BGR порядок для ST7735
  ST7735_SendCommandWithData(handle, ST7735_MADCTL, &madctl.byte, 1);

  // Включаем дисплей
  ST7735_SendCommand(handle, ST7735_DISPON);
  handle->delay_ms(100);

  // Устанавливаем ориентацию по умолчанию
  ST7735_SetOrientation(handle, ST7735_PORTRAIT);
}

/*
3. Управление ориентацией
Установка ориентации экрана
handle: указатель на структуру драйвера
orient: новая ориентация
*/

void ST7735_SetOrientation(ST7735_Handle* handle, ST7735_Orientation orient)
{
  ST7735_MADCTL_Reg madctl;
  madctl.byte = (uint8_t)orient;
  madctl.bits.BGR = 1;  // Всегда BGR для ST7735

  ST7735_SendCommandWithData(handle, ST7735_MADCTL, &madctl.byte, 1);
  handle->orient = orient;
}

/*
4. Настройка окна рисования
Установка области для рисования (окно)
handle: указатель на структуру драйвера
x0: начальная X координата
y0: начальная Y координата
x1: конечная X координата
y1: конечная Y координата
*/

void ST7735_SetWindow(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  // Преобразуем координаты в формат big-endian для отправки
  uint8_t data[4];

  // CASET
  data[0] = (x0 >> 8) & 0xFF;  // Старший байт X0
  data[1] = x0 & 0xFF;         // Младший байт X0
  data[2] = (x1 >> 8) & 0xFF;  // Старший байт X1
  data[3] = x1 & 0xFF;         // Младший байт X1
  ST7735_SendCommandWithData(handle, ST7735_CASET, data, 4);

  // RASET
  data[0] = (y0 >> 8) & 0xFF;  // Старший байт Y0
  data[1] = y0 & 0xFF;         // Младший байт Y0
  data[2] = (y1 >> 8) & 0xFF;  // Старший байт Y1
  data[3] = y1 & 0xFF;         // Младший байт Y1
  ST7735_SendCommandWithData(handle, ST7735_RASET, data, 4);
}

/*
5. Рисование
Заполнение всего экрана одним цветом
handle: указатель на структуру драйвера
color: цвет в формате RGB565
*/

void ST7735_FillScreen(ST7735_Handle* handle, uint16_t color)
{
  // Устанавливаем окно на весь экран
  ST7735_SetWindow(handle, 0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);

  // Отправляем команду записи в память
  ST7735_SendCommand(handle, ST7735_RAMWR);

  // Создаём буфер для цвета в формате big-endian (старший байт первый)
  uint8_t color_bytes[2];
  color_bytes[0] = (color >> 8) & 0xFF;  // Старший байт
  color_bytes[1] = color & 0xFF;         // Младший байт

  // Заполняем экран пикселями
  uint32_t total_pixels = ST7735_WIDTH * ST7735_HEIGHT;

// Отправляем блоками по 1024 пикселя для эффективности
#define BLOCK_SIZE 1024
  uint8_t buffer[BLOCK_SIZE * 2];  // 2 байта на пиксель

  for (uint32_t i = 0; i < BLOCK_SIZE; i++)
  {
    buffer[i * 2] = color_bytes[0];
    buffer[i * 2 + 1] = color_bytes[1];
  }

  uint32_t sent = 0;
  while (sent < total_pixels)
  {
    uint32_t to_send = total_pixels - sent;
    if (to_send > BLOCK_SIZE) to_send = BLOCK_SIZE;
    handle->send_data(buffer, to_send * 2);
    sent += to_send;
  }
}

/*
Рисование одного пикселя
handle: указатель на структуру драйвера
x: X координата
y: Y координата
color: цвет в формате RGB565
*/

void ST7735_DrawPixel(ST7735_Handle* handle, uint16_t x, uint16_t y, uint16_t color)
{
  // Проверка границ
  if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return;

  // Устанавливаем окно на один пиксель
  ST7735_SetWindow(handle, x, y, x, y);

  // Отправляем команду записи
  ST7735_SendCommand(handle, ST7735_RAMWR);

  // Отправляем цвет (big-endian)
  uint8_t color_bytes[2];
  color_bytes[0] = (color >> 8) & 0xFF;
  color_bytes[1] = color & 0xFF;
  handle->send_data(color_bytes, 2);
}