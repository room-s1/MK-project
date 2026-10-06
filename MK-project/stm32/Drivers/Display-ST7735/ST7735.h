#ifndef ST7735_H
#define ST7735_H

#include <stdint.h>

/*
1. Размеры дисплея
*/

#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160

/*
2. Цвета (RGB565)
*/

#define ST7735_BLACK 0x0000
#define ST7735_WHITE 0xFFFF
#define ST7735_RED 0xF800
#define ST7735_GREEN 0x07E0
#define ST7735_BLUE 0x001F
#define ST7735_YELLOW 0xFFE0
#define ST7735_CYAN 0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_DARKGRAY 0x7BEF
#define ST7735_LIGHTGRAY 0xC618
#define ST7735_ORANGE 0xFD20
#define ST7735_PURPLE 0x780F
#define ST7735_BROWN 0xBC40
#define ST7735_PINK 0xF81F

/*
3. Регистры команд
*/
#define ST7735_SWRESET 0x01  // Software reset
#define ST7735_SLPOUT 0x11   // Sleep out
#define ST7735_DISPON 0x29   // Display on
#define ST7735_DISPOFF 0x28  // Display off
#define ST7735_CASET 0x2A    // Column address set
#define ST7735_RASET 0x2B    // Row address set
#define ST7735_RAMWR 0x2C    // Memory write
#define ST7735_INVON 0x21    // Inversion on
#define ST7735_INVOFF 0x20   // Inversion off
#define ST7735_MADCTL 0x36   // Memory access control
#define ST7735_COLMOD 0x3A   // Color mode

/*
4. Ориентация экрана
*/

typedef enum
{
  ST7735_PORTRAIT = 0x00,
  ST7735_LANDSCAPE = 0x60,     // MV=1, MX=1
  ST7735_PORTRAIT_INV = 0xC0,  // MY=1, MX=1
  ST7735_LANDSCAPE_INV = 0xA0  // MY=1, MV=1
} ST7735_Orientation;

/*
5. Структура для регистра MADCTL (объединение)
*/

typedef union
{
  uint8_t byte;
  struct
  {
    uint8_t MY : 1;   // Бит 7
    uint8_t MX : 1;   // Бит 6
    uint8_t MV : 1;   // Бит 5
    uint8_t ML : 1;   // Бит 4
    uint8_t BGR : 1;  // Бит 3
    uint8_t MH : 1;   // Бит 2
    uint8_t res : 2;  // Зарезервировано
  } bits;
} ST7735_MADCTL_Reg;

/*
6. Типы callback-функций
*/

typedef void (*ST7735_Callback_SendCmd)(uint8_t cmd);
typedef void (*ST7735_Callback_SendData)(uint8_t* data, uint16_t len);
typedef void (*ST7735_Callback_Delay)(uint32_t ms);
typedef void (*ST7735_Callback_Reset)(uint8_t state);

/*
7. Основная структура драйвера
*/

typedef struct
{
  // Callback-функции
  void (*send_cmd)(uint8_t cmd);                   // Отправить команду
  void (*send_data)(uint8_t* data, uint16_t len);  // Отправить данные
  void (*delay_ms)(uint32_t ms);                   // Задержка
  void (*reset)(uint8_t state);                    // Управление RESET

  // Состояние
  ST7735_Orientation orient;
} ST7735_Handle;

/*
8. Структура для шрифта
*/

typedef struct
{
  const uint8_t* data;  // Данные шрифта (битовая маска)
  uint8_t width;        // Ширина символа в пикселях
  uint8_t height;       // Высота символа в пикселях
  uint16_t first_char;  // Первый символ в таблице (обычно 32 пробел)
  uint16_t last_char;   // Последний символ в таблице
} ST7735_Font;

/*
9. Прототипы функций драйвера
*/
// Основные функции
void ST7735_Init(ST7735_Handle* handle);
void ST7735_SetOrientation(ST7735_Handle* handle, ST7735_Orientation orient);
void ST7735_SetWindow(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ST7735_FillScreen(ST7735_Handle* handle, uint16_t color);
void ST7735_DrawPixel(ST7735_Handle* handle, uint16_t x, uint16_t y, uint16_t color);

// Графические примитивы
void ST7735_DrawLine(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void ST7735_DrawRect(ST7735_Handle* handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void ST7735_FillRect(ST7735_Handle* handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void ST7735_DrawCircle(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color);
void ST7735_FillCircle(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color);
void ST7735_DrawTriangle(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2,
                         uint16_t y2, uint16_t color);
void ST7735_FillTriangle(ST7735_Handle* handle, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2,
                         uint16_t y2, uint16_t color);

// Функции для работы со шрифтами
void ST7735_DrawChar(ST7735_Handle* handle, uint16_t x, uint16_t y, char ch, const ST7735_Font* font, uint16_t color,
                     uint16_t bg_color);
void ST7735_DrawString(ST7735_Handle* handle, uint16_t x, uint16_t y, const char* str, const ST7735_Font* font,
                       uint16_t color, uint16_t bg_color);
void ST7735_DrawStringCenter(ST7735_Handle* handle, uint16_t y, const char* str, const ST7735_Font* font,
                             uint16_t color, uint16_t bg_color);

/*
10. Встроенные шрифты
*/
extern const ST7735_Font Font_5x7;

#endif /* ST7735_H */