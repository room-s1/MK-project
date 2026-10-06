/* Библиотека для TMC2209 с поддержкой нескольких UART
   Оригинальная логика и структуры полностью сохранены
*/

#ifndef TMC2209_H
#define TMC2209_H

#include "stm32f4xx.h"
#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"
#include <string.h>

/*==================== МАКРОСЫ ====================*/
#define MRES_256 0b0000
#define MRES_128 0b0001
#define MRES_064 0b0010
#define MRES_032 0b0011
#define MRES_016 0b0100
#define MRES_008 0b0101
#define MRES_004 0b0110
#define MRES_002 0b0111
#define MRES_001 0b1000

#define SYNC 0b101
#define VACTUAL_STEP_DIR_INTERFACE 0
#define CURRENT_IHOLD_IRUN_MAX 31
#define IHOLDDELAY_MAX 15
#define IHOLD_DEFAULT 16
#define IRUN_DEFAULT 31

/*==================== РЕГИСТРЫ ====================*/
#define ADDRESS_GCONF 0x00
#define ADDRESS_GSTAT 0x01
#define ADDRESS_IFCNT 0x02
#define ADDRESS_REPLYDELAY 0x03
#define ADDRESS_IOIN 0x06
#define ADDRESS_IHOLD_IRUN 0x10
#define ADDRESS_TPOWERDOWN 0x11
#define ADDRESS_TSTEP 0x12
#define ADDRESS_TPWMTHRS 0x13
#define ADDRESS_TCOOLTHRS 0x14
#define ADDRESS_VACTUAL 0x22
#define ADDRESS_SGTHRS 0x40
#define ADDRESS_SG_RESULT 0x41
#define ADDRESS_COOLCONF 0x42
#define ADDRESS_MSCNT 0x6A
#define ADDRESS_MSCURACT 0x6B
#define ADDRESS_CHOPCONF 0x6C
#define ADDRESS_DRV_STATUS 0x6F
#define ADDRESS_PWMCONF 0x70
#define ADDRESS_PWM_SCALE 0x71
#define ADDRESS_PWM_AUTO 0x72

/*==================== СТРУКТУРЫ ====================*/

typedef struct
{
  bool connection_status;
  bool inicialisation;
  bool software_enabled;
  bool automatic_current_scaling_enabled;
  bool automatic_gradient_adaptation_enabled;
  bool cool_step_enabled;
  bool stealth_chop_mode_enabled;
  bool vreff_pin_mode;
  bool external_resistance_mode;
  bool inversion_mode_enable;
  uint16_t microsteps_mode;
  uint8_t standstill_mode;
  uint8_t irun_percent;
  uint8_t irun_register_value;
  uint8_t ihold_percent;
  uint8_t ihold_register_value;
  uint8_t iholddelay_percent;
  uint8_t iholddelay_register_value;
  uint8_t pwm_offset;
  uint8_t pwm_gradient;
} Settings;

typedef union
{
  struct
  {
    uint32_t temperature_warning : 1;
    uint32_t temperature_shutdown : 1;
    uint32_t short_to_ground_a : 1;
    uint32_t short_to_ground_b : 1;
    uint32_t low_side_short_a : 1;
    uint32_t low_side_short_b : 1;
    uint32_t open_load_a : 1;
    uint32_t open_load_b : 1;
    uint32_t temperature_120c : 1;
    uint32_t temperature_143c : 1;
    uint32_t temperature_150c : 1;
    uint32_t temperature_157c : 1;
    uint32_t reserved0 : 4;
    uint32_t current_scaling : 5;
    uint32_t reserved1 : 9;
    uint32_t stealth_chop_mode : 1;
    uint32_t standstill : 1;
  };
  uint32_t byte;
} DriverStatus;

typedef union
{
  struct
  {
    uint32_t reset : 1;
    uint32_t drv_err : 1;
    uint32_t uv_cp : 1;
    uint32_t reserved : 29;
  };
  uint32_t byte;
} GlobalStatus;

typedef union
{
  struct
  {
    uint64_t sync : 4;
    uint64_t reserved : 4;
    uint64_t serial_address : 8;
    uint64_t register_address : 7;
    uint64_t rw : 1;
    uint64_t data : 32;
    uint64_t crc : 8;
  };
  uint64_t byte;
} WriteReadReplyDatagram;

typedef union
{
  struct
  {
    uint32_t sync : 4;
    uint32_t reserved : 4;
    uint32_t serial_address : 8;
    uint32_t register_address : 7;
    uint32_t rw : 1;
    uint32_t crc : 8;
  };
  uint32_t byte;
} ReadRequestDatagram;

typedef union
{
  struct
  {
    uint32_t i_scale_analog : 1;
    uint32_t internal_rsense : 1;
    uint32_t enable_spread_cycle : 1;
    uint32_t shaft : 1;
    uint32_t index_otpw : 1;
    uint32_t index_step : 1;
    uint32_t pdn_disable : 1;
    uint32_t mstep_reg_select : 1;
    uint32_t multistep_filt : 1;
    uint32_t test_mode : 1;
    uint32_t reserved : 22;
  };
  uint32_t byte;
} GlobalConfig;

typedef union
{
  struct
  {
    uint32_t reserved0 : 8;
    uint32_t replydelay : 4;
    uint32_t reserved1 : 20;
  };
  uint32_t byte;
} ReplyDelay;

typedef union
{
  struct
  {
    uint32_t enn : 1;
    uint32_t reserved0 : 1;
    uint32_t ms1 : 1;
    uint32_t ms2 : 1;
    uint32_t diag : 1;
    uint32_t reserved1 : 1;
    uint32_t pdn_serial : 1;
    uint32_t step : 1;
    uint32_t spread_en : 1;
    uint32_t dir : 1;
    uint32_t reserved2 : 14;
    uint32_t version : 8;
  };
  uint32_t byte;
} Input;

typedef union
{
  struct
  {
    uint32_t ihold : 5;
    uint32_t reserved0 : 3;
    uint32_t irun : 5;
    uint32_t reserved1 : 3;
    uint32_t iholddelay : 4;
    uint32_t reserved2 : 12;
  };
  uint32_t byte;
} DriverCurrent;

typedef union
{
  struct
  {
    uint32_t semin : 4;
    uint32_t reserved0 : 1;
    uint32_t seup : 2;
    uint32_t reserved1 : 3;
    uint32_t semax : 4;
    uint32_t reserved2 : 1;
    uint32_t sedn : 2;
    uint32_t seimin : 1;
    uint32_t reserved3 : 16;
  };
  uint32_t byte;
} CoolConfig;

typedef union
{
  struct
  {
    uint32_t toff : 4;
    uint32_t hstart : 3;
    uint32_t hend : 4;
    uint32_t reserved0 : 4;
    uint32_t tbl : 2;
    uint32_t vsense : 1;
    uint32_t reserved1 : 6;
    uint32_t mres : 4;
    uint32_t interpolation : 1;
    uint32_t double_edge : 1;
    uint32_t diss2g : 1;
    uint32_t diss2vs : 1;
  };
  uint32_t byte;
} ChopperConfig;

typedef union
{
  struct
  {
    uint32_t pwm_offset : 8;
    uint32_t pwm_grad : 8;
    uint32_t pwm_freq : 2;
    uint32_t pwm_autoscale : 1;
    uint32_t pwm_autograd : 1;
    uint32_t freewheel : 2;
    uint32_t reserved0 : 2;
    uint32_t pwm_reg : 4;
    uint32_t pwm_lim : 4;
  };
  uint32_t byte;
} PwmConfig;

typedef union
{
  struct
  {
    uint32_t pwm_scale_sum : 8;
    uint32_t reserved0 : 8;
    uint32_t pwm_scale_auto : 9;
    uint32_t reserved1 : 7;
  };
  uint32_t byte;
} PwmScale;

typedef union
{
  struct
  {
    uint32_t pwm_offset_auto : 8;
    uint32_t reserved_0 : 8;
    uint32_t pwm_gradient_auto : 8;
    uint32_t reserved_1 : 8;
  };
  uint32_t byte;
} PwmAuto;

typedef struct
{
  uint8_t tx_buffer[8];
  uint8_t rx_buffer[8];
  uint32_t bufferbyte;
} TMC2209_Buffer;

/* Основная структура драйвера (без привязки к конкретному UART) */
typedef struct
{
  uint8_t address;
  GPIO_TypeDef* MS1_port;
  uint16_t MS1_pin;
  GPIO_TypeDef* MS2_port;
  uint16_t MS2_pin;
  GPIO_TypeDef* ENABLE_port;
  uint16_t ENABLE_pin;
  GPIO_TypeDef* STEP_port;
  uint16_t STEP_pin;
  GPIO_TypeDef* DIR_port;
  uint16_t DIR_pin;

  Settings settings;
  DriverStatus driverstatus;
  GlobalStatus globalstatus;

  WriteReadReplyDatagram writereadreplaydatagram;
  ReadRequestDatagram readrequestdatagram;
  GlobalConfig globalconfig;
  ReplyDelay replydelay;
  Input input;
  DriverCurrent drivercurrent;
  CoolConfig coolconfig;
  ChopperConfig chopperconfig;
  PwmConfig pwmconfig;
  PwmScale pwmscale;
  PwmAuto pwmauto;
  TMC2209_Buffer tmc_buffer;
} TMC2209_Driver;

/* Новая структура-контейнер для одного UART порта (4 драйвера максимум) */
typedef struct
{
  UART_HandleTypeDef* huart;
  TMC2209_Driver drivers[4];
  uint8_t active_drivers;
} TMC2209_Handle;

/*==================== ФУНКЦИИ ====================*/

/* Инициализация объекта TMC2209_Handle */
void TMC2209_Init(TMC2209_Handle* handle, UART_HandleTypeDef* huart);

/* Настройка драйвера (пины и адрес) */
void TMC2209_SetupDriver(TMC2209_Handle* handle, uint8_t driver_index, uint8_t address, GPIO_TypeDef* ms1_port,
                         uint16_t ms1_pin, GPIO_TypeDef* ms2_port, uint16_t ms2_pin, GPIO_TypeDef* enable_port,
                         uint16_t enable_pin, GPIO_TypeDef* step_port, uint16_t step_pin, GPIO_TypeDef* dir_port,
                         uint16_t dir_pin);

/* Управление питанием */
void TMC2209_DriverEnable(TMC2209_Handle* handle, uint8_t driver_index);
void TMC2209_DriverDisable(TMC2209_Handle* handle, uint8_t driver_index);

/* Установка адреса аппаратно */
void TMC2209_SetAddressDriver(TMC2209_Handle* handle, uint8_t driver_index);

/* Global Config */
void TMC2209_WriteGlobalConfig(TMC2209_Handle* handle, uint8_t driver_index, bool iScaleAnalog, bool internalRsense,
                               bool enableSpreadCycle, bool Shaft, bool indexOtpw);
void TMC2209_GLOBALC_AnalogCurrentScaling(TMC2209_Handle* handle, uint8_t driver_index, bool check);
void TMC2209_GLOBALC_ExternalSenseResistors(TMC2209_Handle* handle, uint8_t driver_index, bool check);
void TMC2209_GLOBALC_StealthChop(TMC2209_Handle* handle, uint8_t driver_index, bool check);
void TMC2209_GLOBALC_InverseMotor(TMC2209_Handle* handle, uint8_t driver_index, bool check);
void TMC2209_GLOBALC_index_step_or_otpw(TMC2209_Handle* handle, uint8_t driver_index, bool step);

/* Настройка тока */
void TMC2209_WriteDriverCurrent(TMC2209_Handle* handle, uint8_t driver_index, uint8_t perIRun, uint8_t perIHold,
                                uint8_t perIHoldDelay);
void TMC2209_CURRENC_setRunCurrent(TMC2209_Handle* handle, uint8_t driver_index, uint8_t percent);
void TMC2209_CURRENC_setHoldCurrent(TMC2209_Handle* handle, uint8_t driver_index, uint8_t percent);
void TMC2209_CURRENC_setHoldDelay(TMC2209_Handle* handle, uint8_t driver_index, uint8_t percent);

/* Cool Config */
void TMC2209_WriteCoolConfig(TMC2209_Handle* handle, uint8_t driver_index, uint8_t seMin, uint8_t seUP, uint8_t seMax,
                             uint8_t seDn, uint8_t seIMin);
void TMC2209_COOLC_setSemin(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data);
void TMC2209_COOLC_setSeup(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data);
void TMC2209_COOLC_setSemax(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data);
void TMC2209_COOLC_setSedn(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data);
void TMC2209_COOLC_setSeIMin(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data);

/* Chopper Config */
void TMC2209_WriteChopperConfig(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Toff, uint8_t Hstart,
                                uint8_t Hend, uint8_t Tbl, uint8_t Vsense, uint8_t Mres, uint8_t Interpolation,
                                uint8_t Double_Edge);
void TMC2209_CHOPPERC_setToff(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Toff);
void TMC2209_CHOPPERC_setHstart(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Hstart);
void TMC2209_CHOPPERC_setHend(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Hend);
void TMC2209_CHOPPERC_setTbl(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Tbl);
void TMC2209_CHOPPERC_VSense(TMC2209_Handle* handle, uint8_t driver_index, bool Vsense);
void TMC2209_CHOPPERC_setMres(TMC2209_Handle* handle, uint8_t driver_index, uint16_t steps);
void TMC2209_CHOPPERC_Interplation(TMC2209_Handle* handle, uint8_t driver_index, bool Interpolation);
void TMC2209_CHOPPERC_Double_Edge(TMC2209_Handle* handle, uint8_t driver_index, bool ddedge);

/* PWM Config */
void TMC2209_WritePWMConfig(TMC2209_Handle* handle, uint8_t driver_index, uint8_t pwm_Offset, uint8_t pwm_Grad,
                            uint8_t pwm_Freq, bool pwm_Autoscale, bool pwm_Autograd, uint8_t Freewheel, uint8_t pwm_Reg,
                            uint8_t pwm_Lim);
void TMC2209_PWMC_PwmOffset(TMC2209_Handle* handle, uint8_t driver_index, uint8_t pwm_amplitude);
void TMC2209_PWMC_PwmGradient(TMC2209_Handle* handle, uint8_t driver_index, uint8_t pwm_amplitude);
void TMC2209_PWMC_PWM_Freq(TMC2209_Handle* handle, uint8_t driver_index, uint8_t freq);
void TMC2209_PWMC_AutomaticCurrentScalling(TMC2209_Handle* handle, uint8_t driver_index, bool state);
void TMC2209_PWMC_AutomaticGradient(TMC2209_Handle* handle, uint8_t driver_index, bool state);
void TMC2209_PWMC_Freewheel(TMC2209_Handle* handle, uint8_t driver_index, uint8_t freewhell);
void TMC2209_PWMC_Pwm_reg(TMC2209_Handle* handle, uint8_t driver_index, uint8_t reg);
void TMC2209_PWMC_Pwm_lim(TMC2209_Handle* handle, uint8_t driver_index, uint8_t lim);

/* Дополнительные регистры */
void TMC2209_WriteReplyDelayConfig(TMC2209_Handle* handle, uint8_t driver_index, uint8_t reply_delay);
void TMC2209_WritePWMAuto(TMC2209_Handle* handle, uint8_t driver_index, bool offset, bool gradient);

/* Чтение статусов */
void TMC2209_ReadDriverStatus(TMC2209_Handle* handle, uint8_t driver_index);
void TMC2209_ReadGlobalStatus(TMC2209_Handle* handle, uint8_t driver_index);
void TMC2209_ReadInputStatus(TMC2209_Handle* handle, uint8_t driver_index);

/* Готовые конфигурации */
void TMC2209_OptimalConfigLite(TMC2209_Handle* handle, uint8_t driver_index);

/* Режим VACTUAL */
void TMC2209_VACTUAL(TMC2209_Handle* handle, uint8_t driver_index, int32_t speed);

#endif /* TMC2209_H */