/*
Реализация графических примитивов
*/

#include "ST7735.h"
#include <stdlib.h>
#include <math.h>

/*
1. Линии (Брезенхем)
Рисование линии
handle: указатель на драйвер
x0,y0: начальная точка
x1,y1: конечная точка
color: цвет линии
*/

void ST7735_DrawLine(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
  int16_t dx = abs(x1 - x0);
  int16_t dy = -abs(y1 - y0);
  int16_t sx = x0 < x1 ? 1 : -1;
  int16_t sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy;
  int16_t err2;

  while (1)
  {
    ST7735_DrawPixel(handle, x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    err2 = 2 * err;
    if (err2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (err2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

/*
2. Прямоугольники
Рисование контура прямоугольника
*/

void ST7735_DrawRect(ST7735_Handle* handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  ST7735_DrawLine(handle, x, y, x + width - 1, y, color);                            // Верхняя
  ST7735_DrawLine(handle, x, y + height - 1, x + width - 1, y + height - 1, color);  // Нижняя
  ST7735_DrawLine(handle, x, y, x, y + height - 1, color);                           // Левая
  ST7735_DrawLine(handle, x + width - 1, y, x + width - 1, y + height - 1, color);   // Правая
}

/*
Заливка прямоугольника
*/

void ST7735_FillRect(ST7735_Handle* handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  // устанавливаем окно и заливаем блоками
  ST7735_SetWindow(handle, x, y, x + width - 1, y + height - 1);

  // Отправляем команду записи в память (используем handle->send_cmd напрямую)
  handle->send_cmd(ST7735_RAMWR);

  // Подготавливаем буфер для цвета
  uint8_t color_bytes[2];
  color_bytes[0] = (color >> 8) & 0xFF;
  color_bytes[1] = color & 0xFF;

  uint32_t total_pixels = width * height;
  uint32_t sent = 0;

#define FILL_BLOCK_SIZE 2048

  uint8_t buffer[FILL_BLOCK_SIZE * 2];
  for (uint32_t i = 0; i < FILL_BLOCK_SIZE; i++)
  {
    buffer[i * 2] = color_bytes[0];
    buffer[i * 2 + 1] = color_bytes[1];
  }

  while (sent < total_pixels)
  {
    uint32_t to_send = total_pixels - sent;
    if (to_send > FILL_BLOCK_SIZE) to_send = FILL_BLOCK_SIZE;
    handle->send_data(buffer, to_send * 2);
    sent += to_send;
  }
}

/*
3. Круг (Брезенхем)
Вспомогательная функция для рисования симметричных точек круга
*/

static void draw_circle_points(ST7735_Handle* handle, int16_t x0, int16_t y0, int16_t x, int16_t y, uint16_t color)
{
  ST7735_DrawPixel(handle, x0 + x, y0 + y, color);
  ST7735_DrawPixel(handle, x0 - x, y0 + y, color);
  ST7735_DrawPixel(handle, x0 + x, y0 - y, color);
  ST7735_DrawPixel(handle, x0 - x, y0 - y, color);
  ST7735_DrawPixel(handle, x0 + y, y0 + x, color);
  ST7735_DrawPixel(handle, x0 - y, y0 + x, color);
  ST7735_DrawPixel(handle, x0 + y, y0 - x, color);
  ST7735_DrawPixel(handle, x0 - y, y0 - x, color);
}

/*
Рисование контура круга
*/

void ST7735_DrawCircle(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color)
{
  int16_t x = 0;
  int16_t y = radius;
  int16_t d = 3 - 2 * radius;

  while (x <= y)
  {
    draw_circle_points(handle, x0, y0, x, y, color);
    if (d < 0)
    {
      d = d + 4 * x + 6;
    }
    else
    {
      d = d + 4 * (x - y) + 10;
      y--;
    }
    x++;
  }
}

/*
Заливка круга
*/

void ST7735_FillCircle(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color)
{
  // Рисуем горизонтальные линии для каждой строки круга
  for (int16_t y = -radius; y <= radius; y++)
  {
    int16_t dx = (int16_t)sqrt(radius * radius - y * y);
    ST7735_DrawLine(handle, x0 - dx, y0 + y, x0 + dx, y0 + y, color);
  }
}

/*
4. Треугольник
Рисование контура треугольника
*/

void ST7735_DrawTriangle(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2,
                         uint16_t y2, uint16_t color)
{
  ST7735_DrawLine(handle, x0, y0, x1, y1, color);
  ST7735_DrawLine(handle, x1, y1, x2, y2, color);
  ST7735_DrawLine(handle, x2, y2, x0, y0, color);
}

/*
Функция для заливки треугольника
*/

static void fill_flat_bottom_triangle(ST7735_Handle* handle, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                                      int16_t y2, uint16_t color)
{
  float inv_slope1 = (float)(x1 - x0) / (y1 - y0);
  float inv_slope2 = (float)(x2 - x0) / (y2 - y0);

  for (int16_t y = y0; y <= y1; y++)
  {
    int16_t x_start = x0 + (int16_t)(inv_slope1 * (y - y0));
    int16_t x_end = x0 + (int16_t)(inv_slope2 * (y - y0));
    ST7735_DrawLine(handle, x_start, y, x_end, y, color);
  }
}

static void fill_flat_top_triangle(ST7735_Handle* handle, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                                   int16_t y2, uint16_t color)
{
  float inv_slope1 = (float)(x2 - x0) / (y2 - y0);
  float inv_slope2 = (float)(x2 - x1) / (y2 - y1);

  for (int16_t y = y0; y <= y2; y++)
  {
    int16_t x_start = x0 + (int16_t)(inv_slope1 * (y - y0));
    int16_t x_end = x1 + (int16_t)(inv_slope2 * (y - y1));
    ST7735_DrawLine(handle, x_start, y, x_end, y, color);
  }
}

/*
Заливка треугольника (разбиением на плоские треугольники)
*/

void ST7735_FillTriangle(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2,
                         uint16_t y2, uint16_t color)
{
  // Сортируем точки по Y
  int16_t points[3][2] = {{x0, y0}, {x1, y1}, {x2, y2}};

  // Пузырьковая сортировка по Y
  for (int i = 0; i < 2; i++)
  {
    for (int j = i + 1; j < 3; j++)
    {
      if (points[i][1] > points[j][1])
      {
        int16_t temp_x = points[i][0];
        int16_t temp_y = points[i][1];
        points[i][0] = points[j][0];
        points[i][1] = points[j][1];
        points[j][0] = temp_x;
        points[j][1] = temp_y;
      }
    }
  }

  x0 = points[0][0];
  y0 = points[0][1];
  x1 = points[1][0];
  y1 = points[1][1];
  x2 = points[2][0];
  y2 = points[2][1];

  // Разбиваем на два треугольника: плоский верх и плоский низ
  if (y1 == y2)
  {
    fill_flat_bottom_triangle(handle, x0, y0, x1, y1, x2, y2, color);
  }
  else if (y0 == y1)
  {
    fill_flat_top_triangle(handle, x0, y0, x1, y1, x2, y2, color);
  }
  else
  {
    // Находим точку на ребре x0-x2 с Y=y1
    int16_t x3 = x0 + (int16_t)((float)(x2 - x0) * (y1 - y0) / (y2 - y0));
    fill_flat_bottom_triangle(handle, x0, y0, x1, y1, x3, y1, color);
    fill_flat_top_triangle(handle, x1, y1, x3, y1, x2, y2, color);
  }
}