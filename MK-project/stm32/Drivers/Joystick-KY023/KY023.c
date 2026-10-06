// ky023.c
#include "KY023.h"
#include <stdlib.h>  // for abs

// Инициализация глобальных коллбеков (NULL по умолчанию)
KY023_Callback KY023_OnXPositive = NULL;
KY023_Callback KY023_OnXNegative = NULL;
KY023_Callback KY023_OnYPositive = NULL;
KY023_Callback KY023_OnYNegative = NULL;
KY023_Callback KY023_OnCenter = NULL;
KY023_Callback KY023_OnAnyMove = NULL;
KY023_Callback KY023_OnXOver = NULL;
KY023_Callback KY023_OnYOver = NULL;
KY023_Callback KY023_OnXTrigger = NULL;
KY023_Callback KY023_OnYTrigger = NULL;

// Параметры триггеров
static uint8_t x_threshold = 70;
static uint8_t x_hysteresis = 10;
static uint8_t y_threshold = 70;
static uint8_t y_hysteresis = 10;

// Вспомогательная функция: нормализация
static int8_t normalize(uint16_t raw, uint16_t min_val, uint16_t max_val, uint8_t deadzone_percent)
{
  int32_t center = (min_val + max_val) / 2;
  int32_t range = max_val - min_val;

  if (range == 0) return 0;

  int32_t value = (int32_t)raw - center;
  int32_t half_range = range / 2;
  int32_t normalized = (value * 100) / half_range;

  if (normalized > 100) normalized = 100;
  if (normalized < -100) normalized = -100;

  if (normalized > -deadzone_percent && normalized < deadzone_percent)
  {
    normalized = 0;
  }

  return (int8_t)normalized;
}

// Проверка и вызов коллбека (без вывода)
static void call_callback(KY023_Callback cb, KY023_HandleTypeDef* ky)
{
  if (cb != NULL)
  {
    cb(ky);
  }
}

// Проверка триггеров
static void check_trigger(KY023_HandleTypeDef* ky, int8_t current, int8_t* prev, uint8_t* trigger_flag,
                          uint8_t threshold, uint8_t hysteresis, KY023_Callback over_cb, KY023_Callback trigger_cb)
{
  if (abs(current) >= threshold && abs(*prev) < threshold)
  {
    *trigger_flag = 1;
    call_callback(trigger_cb, ky);
    call_callback(over_cb, ky);
  }

  if (abs(current) < (threshold - hysteresis))
  {
    *trigger_flag = 0;
  }

  *prev = current;
}

void KY023_Init(KY023_HandleTypeDef* ky, uint8_t deadzone_percent)
{
  ky->x_raw = 0;
  ky->y_raw = 0;
  ky->x_norm = 0;
  ky->y_norm = 0;
  ky->x_prev = 0;
  ky->y_prev = 0;
  ky->x_trigger = 0;
  ky->y_trigger = 0;
  ky->deadzone = deadzone_percent;

  ky->x_min = 0;
  ky->x_max = 4095;
  ky->y_min = 0;
  ky->y_max = 4095;
}

void KY023_SetRawValues(KY023_HandleTypeDef* ky, uint16_t x, uint16_t y)
{
  ky->x_raw = x;
  ky->y_raw = y;
}

void KY023_Update(KY023_HandleTypeDef* ky)
{
  int8_t old_x = ky->x_norm;
  int8_t old_y = ky->y_norm;

  ky->x_norm = normalize(ky->x_raw, ky->x_min, ky->x_max, ky->deadzone);
  ky->y_norm = normalize(ky->y_raw, ky->y_min, ky->y_max, ky->deadzone);

  // Движение
  if (ky->x_norm != old_x || ky->y_norm != old_y)
  {
    call_callback(KY023_OnAnyMove, ky);
  }

  // Триггеры
  check_trigger(ky, ky->x_norm, &ky->x_prev, &ky->x_trigger, x_threshold, x_hysteresis, KY023_OnXOver,
                KY023_OnXTrigger);

  check_trigger(ky, ky->y_norm, &ky->y_prev, &ky->y_trigger, y_threshold, y_hysteresis, KY023_OnYOver,
                KY023_OnYTrigger);

  // Направления
  if (ky->x_norm > 0) call_callback(KY023_OnXPositive, ky);
  if (ky->x_norm < 0) call_callback(KY023_OnXNegative, ky);
  if (ky->y_norm > 0) call_callback(KY023_OnYPositive, ky);
  if (ky->y_norm < 0) call_callback(KY023_OnYNegative, ky);

  // Центр
  if (ky->x_norm == 0 && ky->y_norm == 0)
  {
    call_callback(KY023_OnCenter, ky);
  }
}

void KY023_RegisterCallbacks(KY023_Callback on_x_pos, KY023_Callback on_x_neg, KY023_Callback on_y_pos,
                             KY023_Callback on_y_neg, KY023_Callback on_center, KY023_Callback on_any_move)
{
  KY023_OnXPositive = on_x_pos;
  KY023_OnXNegative = on_x_neg;
  KY023_OnYPositive = on_y_pos;
  KY023_OnYNegative = on_y_neg;
  KY023_OnCenter = on_center;
  KY023_OnAnyMove = on_any_move;
}

void KY023_SetXTrigger(KY023_HandleTypeDef* ky, uint8_t threshold, uint8_t hysteresis)
{
  x_threshold = (threshold > 100) ? 100 : threshold;
  x_hysteresis = (hysteresis > 50) ? 50 : hysteresis;
  ky->x_trigger = 0;
}

void KY023_SetYTrigger(KY023_HandleTypeDef* ky, uint8_t threshold, uint8_t hysteresis)
{
  y_threshold = (threshold > 100) ? 100 : threshold;
  y_hysteresis = (hysteresis > 50) ? 50 : hysteresis;
  ky->y_trigger = 0;
}

void KY023_Calibrate(KY023_HandleTypeDef* ky, uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max)
{
  ky->x_min = x_min;
  ky->x_max = x_max;
  ky->y_min = y_min;
  ky->y_max = y_max;
}

void KY023_AutoCalibrate(KY023_HandleTypeDef* ky)
{
  int32_t center_x = ky->x_raw;
  int32_t center_y = ky->y_raw;

  ky->x_min = (center_x > 2000) ? (center_x - 2000) : 0;
  ky->x_max = (center_x < 2095) ? (center_x + 2000) : 4095;
  ky->y_min = (center_y > 2000) ? (center_y - 2000) : 0;
  ky->y_max = (center_y < 2095) ? (center_y + 2000) : 4095;
}

int8_t KY023_GetX(KY023_HandleTypeDef* ky)
{
  return ky->x_norm;
}
int8_t KY023_GetY(KY023_HandleTypeDef* ky)
{
  return ky->y_norm;
}
uint16_t KY023_GetRawX(KY023_HandleTypeDef* ky)
{
  return ky->x_raw;
}
uint16_t KY023_GetRawY(KY023_HandleTypeDef* ky)
{
  return ky->y_raw;
}
uint8_t KY023_GetXTrigger(KY023_HandleTypeDef* ky)
{
  return ky->x_trigger;
}
uint8_t KY023_GetYTrigger(KY023_HandleTypeDef* ky)
{
  return ky->y_trigger;
}
void KY023_ResetXTrigger(KY023_HandleTypeDef* ky)
{
  ky->x_trigger = 0;
}
void KY023_ResetYTrigger(KY023_HandleTypeDef* ky)
{
  ky->y_trigger = 0;
}