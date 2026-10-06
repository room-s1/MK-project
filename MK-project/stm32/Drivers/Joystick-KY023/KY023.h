// ky023.h
#ifndef KY023_H
#define KY023_H

#include <stdint.h>

// Структура джойстика KY-023
typedef struct
{
  uint16_t x_raw;    // сырое значение X (0-4095)
  uint16_t y_raw;    // сырое значение Y (0-4095)
  int8_t x_norm;     // нормализованное X (-100..+100)
  int8_t y_norm;     // нормализованное Y (-100..+100)
  uint8_t deadzone;  // мертвая зона в процентах (0-100)

  // Калибровочные значения
  uint16_t x_min;
  uint16_t x_max;
  uint16_t y_min;
  uint16_t y_max;

  // Состояния для триггеров
  int8_t x_prev;      // предыдущее значение X
  int8_t y_prev;      // предыдущее значение Y
  uint8_t x_trigger;  // флаг триггера по X
  uint8_t y_trigger;  // флаг триггера по Y
} KY023_HandleTypeDef;

// Типы коллбек-функций (пустые, для заполнения пользователем)
typedef void (*KY023_Callback)(KY023_HandleTypeDef* ky);

// Глобальные коллбеки (по умолчанию NULL)
extern KY023_Callback KY023_OnXPositive;  // X > 0 (вправо)
extern KY023_Callback KY023_OnXNegative;  // X < 0 (влево)
extern KY023_Callback KY023_OnYPositive;  // Y > 0 (вверх)
extern KY023_Callback KY023_OnYNegative;  // Y < 0 (вниз)
extern KY023_Callback KY023_OnCenter;     // X=0 и Y=0 (центр)
extern KY023_Callback KY023_OnXOver;      // X перевалил за порог (например >50)
extern KY023_Callback KY023_OnYOver;      // Y перевалил за порог
extern KY023_Callback KY023_OnAnyMove;    // любое движение
extern KY023_Callback KY023_OnXTrigger;   // X перешел порог триггера
extern KY023_Callback KY023_OnYTrigger;   // Y перешел порог триггера

// Основные функции
void KY023_Init(KY023_HandleTypeDef* ky, uint8_t deadzone_percent);
void KY023_SetRawValues(KY023_HandleTypeDef* ky, uint16_t x, uint16_t y);
void KY023_Update(KY023_HandleTypeDef* ky);

// Функции для установки коллбеков
void KY023_RegisterCallbacks(KY023_Callback on_x_pos, KY023_Callback on_x_neg, KY023_Callback on_y_pos,
                             KY023_Callback on_y_neg, KY023_Callback on_center, KY023_Callback on_any_move);

// Настройка триггеров
void KY023_SetXTrigger(KY023_HandleTypeDef* ky, uint8_t threshold, uint8_t hysteresis);
void KY023_SetYTrigger(KY023_HandleTypeDef* ky, uint8_t threshold, uint8_t hysteresis);

// Калибровка
void KY023_Calibrate(KY023_HandleTypeDef* ky, uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max);
void KY023_AutoCalibrate(KY023_HandleTypeDef* ky);

// Геттеры
int8_t KY023_GetX(KY023_HandleTypeDef* ky);
int8_t KY023_GetY(KY023_HandleTypeDef* ky);
uint16_t KY023_GetRawX(KY023_HandleTypeDef* ky);
uint16_t KY023_GetRawY(KY023_HandleTypeDef* ky);
uint8_t KY023_GetXTrigger(KY023_HandleTypeDef* ky);
uint8_t KY023_GetYTrigger(KY023_HandleTypeDef* ky);

// Сброс флагов триггеров
void KY023_ResetXTrigger(KY023_HandleTypeDef* ky);
void KY023_ResetYTrigger(KY023_HandleTypeDef* ky);

#endif