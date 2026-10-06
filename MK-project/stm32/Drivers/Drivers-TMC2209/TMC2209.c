#include "TMC2209.h"

/*==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================*/

static uint8_t TMC2209_crc_calculate_write(uint64_t datagram_data)
{
  uint8_t datagram_size = 8;
  uint8_t crc = 0;
  uint8_t byte;
  for (uint8_t i = 0; i < (datagram_size - 1); ++i)
  {
    byte = (datagram_data >> (i * 8)) & 0xFF;
    for (uint8_t j = 0; j < 8; ++j)
    {
      if ((crc >> 7) ^ (byte & 0x01))
      {
        crc = (crc << 1) ^ 0x07;
      }
      else
      {
        crc = crc << 1;
      }
      byte = byte >> 1;
    }
  }
  return crc;
}

static uint8_t TMC2209_crc_calculate_read(uint32_t datagram_data)
{
  uint8_t datagram_size = 4;
  uint8_t crc = 0;
  uint8_t byte;
  for (uint8_t i = 0; i < (datagram_size - 1); ++i)
  {
    byte = (datagram_data >> (i * 8)) & 0xFF;
    for (uint8_t j = 0; j < 8; ++j)
    {
      if ((crc >> 7) ^ (byte & 0x01))
      {
        crc = (crc << 1) ^ 0x07;
      }
      else
      {
        crc = crc << 1;
      }
      byte = byte >> 1;
    }
  }
  return crc;
}

/*==================== БАЗОВЫЕ ОПЕРАЦИИ UART ====================*/

static void TMC2209_WriteDatagram(TMC2209_Handle* handle, uint8_t driver_index, uint8_t register_address, uint32_t data)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];

  drv->writereadreplaydatagram.byte = 0;
  drv->writereadreplaydatagram.sync = SYNC;
  drv->writereadreplaydatagram.serial_address = drv->address;
  drv->writereadreplaydatagram.register_address = register_address;
  drv->writereadreplaydatagram.rw = 1;
  drv->writereadreplaydatagram.data = data;
  drv->writereadreplaydatagram.crc = TMC2209_crc_calculate_write(drv->writereadreplaydatagram.byte);

  for (int i = 0; i < 8; i++)
  {
    drv->tmc_buffer.tx_buffer[i] = (drv->writereadreplaydatagram.byte >> (i * 8)) & 0xFF;
  }

  HAL_UART_Transmit(handle->huart, drv->tmc_buffer.tx_buffer, 8, 100);
}

static void TMC2209_ReadDatagram(TMC2209_Handle* handle, uint8_t driver_index, uint8_t register_address)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];

  // Очищаем структуру
  drv->readrequestdatagram.byte = 0;

  // Заполняем поля (правильный порядок!)
  drv->readrequestdatagram.sync = SYNC;
  drv->readrequestdatagram.serial_address = drv->address;
  drv->readrequestdatagram.register_address = register_address;
  drv->readrequestdatagram.rw = 0;
  drv->readrequestdatagram.crc = TMC2209_crc_calculate_read(drv->readrequestdatagram.byte);

  // Очищаем буферы
  memset(drv->tmc_buffer.tx_buffer, 0, 8);
  memset(drv->tmc_buffer.rx_buffer, 0, 8);

  // ОТПРАВЛЯЕМ 4 БАЙТА в правильном порядке (little-endian)
  drv->tmc_buffer.tx_buffer[0] = (drv->readrequestdatagram.byte >> 0) & 0xFF;
  drv->tmc_buffer.tx_buffer[1] = (drv->readrequestdatagram.byte >> 8) & 0xFF;
  drv->tmc_buffer.tx_buffer[2] = (drv->readrequestdatagram.byte >> 16) & 0xFF;
  drv->tmc_buffer.tx_buffer[3] = (drv->readrequestdatagram.byte >> 24) & 0xFF;

  HAL_UART_Transmit(handle->huart, drv->tmc_buffer.tx_buffer, 4, 100);
  HAL_Delay(1);

  // ПРИНИМАЕМ 8 БАЙТ (ответ драйвера)
  HAL_UART_Receive(handle->huart, drv->tmc_buffer.rx_buffer, 8, 100);
}

/*==================== ИНИЦИАЛИЗАЦИЯ ====================*/

void TMC2209_Init(TMC2209_Handle* handle, UART_HandleTypeDef* huart)
{
  memset(handle, 0, sizeof(TMC2209_Handle));
  handle->huart = huart;
  handle->active_drivers = 0;

  for (int i = 0; i < 4; i++)
  {
    handle->drivers[i].address = i;
  }
}

void TMC2209_SetupDriver(TMC2209_Handle* handle, uint8_t driver_index, uint8_t address, GPIO_TypeDef* ms1_port,
                         uint16_t ms1_pin, GPIO_TypeDef* ms2_port, uint16_t ms2_pin, GPIO_TypeDef* enable_port,
                         uint16_t enable_pin, GPIO_TypeDef* step_port, uint16_t step_pin, GPIO_TypeDef* dir_port,
                         uint16_t dir_pin)
{
  if (driver_index >= 4) return;

  TMC2209_Driver* drv = &handle->drivers[driver_index];

  drv->address = address;
  drv->MS1_port = ms1_port;
  drv->MS1_pin = ms1_pin;
  drv->MS2_port = ms2_port;
  drv->MS2_pin = ms2_pin;
  drv->ENABLE_port = enable_port;
  drv->ENABLE_pin = enable_pin;
  drv->STEP_port = step_port;
  drv->STEP_pin = step_pin;
  drv->DIR_port = dir_port;
  drv->DIR_pin = dir_pin;

  handle->active_drivers++;
}

/*==================== УПРАВЛЕНИЕ ПИТАНИЕМ ====================*/

void TMC2209_SetAddressDriver(TMC2209_Handle* handle, uint8_t driver_index)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];

  HAL_GPIO_WritePin(drv->MS1_port, drv->MS1_pin, (drv->address & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(drv->MS2_port, drv->MS2_pin, (drv->address & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void TMC2209_DriverEnable(TMC2209_Handle* handle, uint8_t driver_index)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  HAL_GPIO_WritePin(drv->ENABLE_port, drv->ENABLE_pin, GPIO_PIN_RESET);
}

void TMC2209_DriverDisable(TMC2209_Handle* handle, uint8_t driver_index)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  HAL_GPIO_WritePin(drv->ENABLE_port, drv->ENABLE_pin, GPIO_PIN_SET);
}

/*==================== GLOBAL CONFIG ====================*/

void TMC2209_WriteGlobalConfig(TMC2209_Handle* handle, uint8_t driver_index, bool iScaleAnalog, bool internalRsense,
                               bool enableSpreadCycle, bool Shaft, bool indexOtpw)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];

  drv->globalconfig.byte = 0;
  drv->globalconfig.i_scale_analog = iScaleAnalog;
  drv->globalconfig.internal_rsense = internalRsense;
  drv->globalconfig.enable_spread_cycle = enableSpreadCycle;
  drv->globalconfig.shaft = Shaft;
  drv->globalconfig.index_otpw = indexOtpw;
  drv->globalconfig.index_step = !indexOtpw;
  drv->globalconfig.pdn_disable = true;
  drv->globalconfig.mstep_reg_select = true;
  drv->globalconfig.multistep_filt = true;
  drv->globalconfig.test_mode = false;

  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_GCONF, drv->globalconfig.byte);
}

void TMC2209_GLOBALC_AnalogCurrentScaling(TMC2209_Handle* handle, uint8_t driver_index, bool check)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->globalconfig.i_scale_analog = check;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_GCONF, drv->globalconfig.byte);
}

void TMC2209_GLOBALC_ExternalSenseResistors(TMC2209_Handle* handle, uint8_t driver_index, bool check)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->globalconfig.internal_rsense = check;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_GCONF, drv->globalconfig.byte);
}

void TMC2209_GLOBALC_StealthChop(TMC2209_Handle* handle, uint8_t driver_index, bool check)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->globalconfig.enable_spread_cycle = check;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_GCONF, drv->globalconfig.byte);
}

void TMC2209_GLOBALC_InverseMotor(TMC2209_Handle* handle, uint8_t driver_index, bool check)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->globalconfig.shaft = check;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_GCONF, drv->globalconfig.byte);
}

void TMC2209_GLOBALC_index_step_or_otpw(TMC2209_Handle* handle, uint8_t driver_index, bool step)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->globalconfig.index_step = step ? true : false;
  drv->globalconfig.index_otpw = step ? false : true;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_GCONF, drv->globalconfig.byte);
}

/*==================== CURRENT CONFIG ====================*/

void TMC2209_WriteDriverCurrent(TMC2209_Handle* handle, uint8_t driver_index, uint8_t perIRun, uint8_t perIHold,
                                uint8_t perIHoldDelay)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];

  drv->drivercurrent.byte = 0;
  drv->drivercurrent.irun = (perIRun * CURRENT_IHOLD_IRUN_MAX) / 100;
  drv->drivercurrent.ihold = (perIHold * CURRENT_IHOLD_IRUN_MAX) / 100;
  drv->drivercurrent.iholddelay = (perIHoldDelay * IHOLDDELAY_MAX) / 100;

  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_IHOLD_IRUN, drv->drivercurrent.byte);
}

void TMC2209_CURRENC_setRunCurrent(TMC2209_Handle* handle, uint8_t driver_index, uint8_t percent)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->drivercurrent.irun = (percent * CURRENT_IHOLD_IRUN_MAX) / 100;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_IHOLD_IRUN, drv->drivercurrent.byte);
}

void TMC2209_CURRENC_setHoldCurrent(TMC2209_Handle* handle, uint8_t driver_index, uint8_t percent)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->drivercurrent.ihold = (percent * CURRENT_IHOLD_IRUN_MAX) / 100;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_IHOLD_IRUN, drv->drivercurrent.byte);
}

void TMC2209_CURRENC_setHoldDelay(TMC2209_Handle* handle, uint8_t driver_index, uint8_t percent)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->drivercurrent.iholddelay = (percent * IHOLDDELAY_MAX) / 100;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_IHOLD_IRUN, drv->drivercurrent.byte);
}

/*==================== COOL CONFIG ====================*/

void TMC2209_WriteCoolConfig(TMC2209_Handle* handle, uint8_t driver_index, uint8_t seMin, uint8_t seUP, uint8_t seMax,
                             uint8_t seDn, uint8_t seIMin)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];

  drv->coolconfig.byte = 0;
  drv->coolconfig.semin = seMin;
  drv->coolconfig.seup = seUP;
  drv->coolconfig.semax = seMax;
  drv->coolconfig.sedn = seDn;
  drv->coolconfig.seimin = seIMin;

  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_COOLCONF, drv->coolconfig.byte);
}

void TMC2209_COOLC_setSemin(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->coolconfig.semin = data;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_COOLCONF, drv->coolconfig.byte);
}

void TMC2209_COOLC_setSeup(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->coolconfig.seup = data;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_COOLCONF, drv->coolconfig.byte);
}

void TMC2209_COOLC_setSemax(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->coolconfig.semax = data;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_COOLCONF, drv->coolconfig.byte);
}

void TMC2209_COOLC_setSedn(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->coolconfig.sedn = data;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_COOLCONF, drv->coolconfig.byte);
}

void TMC2209_COOLC_setSeIMin(TMC2209_Handle* handle, uint8_t driver_index, uint8_t data)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->coolconfig.seimin = data;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_COOLCONF, drv->coolconfig.byte);
}

/*==================== CHOPPER CONFIG ====================*/

void TMC2209_WriteChopperConfig(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Toff, uint8_t Hstart,
                                uint8_t Hend, uint8_t Tbl, uint8_t Vsense, uint8_t Mres, uint8_t Interpolation,
                                uint8_t Double_Edge)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];

  drv->chopperconfig.byte = 0;
  drv->chopperconfig.toff = Toff;
  drv->chopperconfig.hstart = Hstart;
  drv->chopperconfig.hend = Hend;
  drv->chopperconfig.tbl = Tbl;
  drv->chopperconfig.vsense = Vsense;
  drv->chopperconfig.mres = Mres;
  drv->chopperconfig.interpolation = Interpolation;
  drv->chopperconfig.double_edge = Double_Edge;

  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_CHOPCONF, drv->chopperconfig.byte);
}

void TMC2209_CHOPPERC_setToff(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Toff)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->chopperconfig.toff = Toff;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_CHOPCONF, drv->chopperconfig.byte);
}

void TMC2209_CHOPPERC_setHstart(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Hstart)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->chopperconfig.hstart = Hstart;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_CHOPCONF, drv->chopperconfig.byte);
}

void TMC2209_CHOPPERC_setHend(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Hend)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->chopperconfig.hend = Hend;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_CHOPCONF, drv->chopperconfig.byte);
}

void TMC2209_CHOPPERC_setTbl(TMC2209_Handle* handle, uint8_t driver_index, uint8_t Tbl)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->chopperconfig.tbl = Tbl;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_CHOPCONF, drv->chopperconfig.byte);
}

void TMC2209_CHOPPERC_VSense(TMC2209_Handle* handle, uint8_t driver_index, bool Vsense)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->chopperconfig.vsense = Vsense;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_CHOPCONF, drv->chopperconfig.byte);
}

void TMC2209_CHOPPERC_setMres(TMC2209_Handle* handle, uint8_t driver_index, uint16_t steps)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  switch (steps)
  {
    case 0:
      drv->chopperconfig.mres = MRES_001;
      break;
    case 1:
      drv->chopperconfig.mres = MRES_002;
      break;
    case 2:
      drv->chopperconfig.mres = MRES_004;
      break;
    case 3:
      drv->chopperconfig.mres = MRES_008;
      break;
    case 4:
      drv->chopperconfig.mres = MRES_016;
      break;
    case 5:
      drv->chopperconfig.mres = MRES_032;
      break;
    case 6:
      drv->chopperconfig.mres = MRES_064;
      break;
    case 7:
      drv->chopperconfig.mres = MRES_128;
      break;
    case 8:
      drv->chopperconfig.mres = MRES_256;
      break;
    default:
      drv->chopperconfig.mres = MRES_016;
      break;
  }
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_CHOPCONF, drv->chopperconfig.byte);
}

void TMC2209_CHOPPERC_Interplation(TMC2209_Handle* handle, uint8_t driver_index, bool Interpolation)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->chopperconfig.interpolation = Interpolation;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_CHOPCONF, drv->chopperconfig.byte);
}

void TMC2209_CHOPPERC_Double_Edge(TMC2209_Handle* handle, uint8_t driver_index, bool ddedge)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->chopperconfig.double_edge = ddedge;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_CHOPCONF, drv->chopperconfig.byte);
}

/*==================== REPLYDELAY CONFIG ====================*/

void TMC2209_WriteReplyDelayConfig(TMC2209_Handle* handle, uint8_t driver_index, uint8_t reply_delay)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->replydelay.replydelay = reply_delay;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_REPLYDELAY, drv->replydelay.byte);
}

/*==================== PWM CONFIG ====================*/

void TMC2209_WritePWMConfig(TMC2209_Handle* handle, uint8_t driver_index, uint8_t pwm_Offset, uint8_t pwm_Grad,
                            uint8_t pwm_Freq, bool pwm_Autoscale, bool pwm_Autograd, uint8_t Freewheel, uint8_t pwm_Reg,
                            uint8_t pwm_Lim)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];

  drv->pwmconfig.byte = 0;
  drv->pwmconfig.pwm_offset = pwm_Offset;
  drv->pwmconfig.pwm_grad = pwm_Grad;
  drv->pwmconfig.pwm_freq = pwm_Freq;
  drv->pwmconfig.pwm_autoscale = pwm_Autoscale;
  drv->pwmconfig.pwm_autograd = pwm_Autograd;
  drv->pwmconfig.freewheel = Freewheel;
  drv->pwmconfig.pwm_reg = pwm_Reg;
  drv->pwmconfig.pwm_lim = pwm_Lim;

  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWMCONF, drv->pwmconfig.byte);
}

void TMC2209_PWMC_PwmOffset(TMC2209_Handle* handle, uint8_t driver_index, uint8_t pwm_amplitude)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->pwmconfig.pwm_offset = pwm_amplitude;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWMCONF, drv->pwmconfig.byte);
}

void TMC2209_PWMC_PwmGradient(TMC2209_Handle* handle, uint8_t driver_index, uint8_t pwm_amplitude)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->pwmconfig.pwm_grad = pwm_amplitude;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWMCONF, drv->pwmconfig.byte);
}

void TMC2209_PWMC_PWM_Freq(TMC2209_Handle* handle, uint8_t driver_index, uint8_t freq)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->pwmconfig.pwm_freq = freq;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWMCONF, drv->pwmconfig.byte);
}

void TMC2209_PWMC_AutomaticCurrentScalling(TMC2209_Handle* handle, uint8_t driver_index, bool state)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->pwmconfig.pwm_autoscale = state;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWMCONF, drv->pwmconfig.byte);
}

void TMC2209_PWMC_AutomaticGradient(TMC2209_Handle* handle, uint8_t driver_index, bool state)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->pwmconfig.pwm_autograd = state;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWMCONF, drv->pwmconfig.byte);
}

void TMC2209_PWMC_Freewheel(TMC2209_Handle* handle, uint8_t driver_index, uint8_t freewhell)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->pwmconfig.freewheel = freewhell;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWMCONF, drv->pwmconfig.byte);
}

void TMC2209_PWMC_Pwm_reg(TMC2209_Handle* handle, uint8_t driver_index, uint8_t reg)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->pwmconfig.pwm_reg = reg;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWMCONF, drv->pwmconfig.byte);
}

void TMC2209_PWMC_Pwm_lim(TMC2209_Handle* handle, uint8_t driver_index, uint8_t lim)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->pwmconfig.pwm_lim = lim;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWMCONF, drv->pwmconfig.byte);
}

/*==================== PWMAUTO CONFIG ====================*/

void TMC2209_WritePWMAuto(TMC2209_Handle* handle, uint8_t driver_index, bool offset, bool gradient)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  drv->pwmauto.pwm_offset_auto = offset;
  drv->pwmauto.pwm_gradient_auto = gradient;
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_PWM_AUTO, drv->pwmauto.byte);
}

/*==================== READ CONFIG STATE ====================*/

void TMC2209_ReadDriverStatus(TMC2209_Handle* handle, uint8_t driver_index)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  TMC2209_ReadDatagram(handle, driver_index, ADDRESS_DRV_STATUS);
  // ПРАВИЛЬНЫЙ ПОРЯДОК: байты 0-3 это данные ответа
  drv->driverstatus.byte = (drv->tmc_buffer.rx_buffer[0] << 0) | (drv->tmc_buffer.rx_buffer[1] << 8) |
                           (drv->tmc_buffer.rx_buffer[2] << 16) | (drv->tmc_buffer.rx_buffer[3] << 24);
}

void TMC2209_ReadGlobalStatus(TMC2209_Handle* handle, uint8_t driver_index)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  TMC2209_ReadDatagram(handle, driver_index, ADDRESS_GSTAT);
  drv->globalstatus.byte = (drv->tmc_buffer.rx_buffer[0] << 0) | (drv->tmc_buffer.rx_buffer[1] << 8) |
                           (drv->tmc_buffer.rx_buffer[2] << 16) | (drv->tmc_buffer.rx_buffer[3] << 24);
}

void TMC2209_ReadInputStatus(TMC2209_Handle* handle, uint8_t driver_index)
{
  TMC2209_Driver* drv = &handle->drivers[driver_index];
  TMC2209_ReadDatagram(handle, driver_index, ADDRESS_IOIN);
  drv->input.byte = (drv->tmc_buffer.rx_buffer[0] << 0) | (drv->tmc_buffer.rx_buffer[1] << 8) |
                    (drv->tmc_buffer.rx_buffer[2] << 16) | (drv->tmc_buffer.rx_buffer[3] << 24);
}

/*==================== ГОТОВЫЕ КОНФИГУРАЦИИ ====================*/

void TMC2209_OptimalConfigLite(TMC2209_Handle* handle, uint8_t driver_index)
{
  TMC2209_SetAddressDriver(handle, driver_index);
  HAL_Delay(1);
  TMC2209_DriverEnable(handle, driver_index);
  HAL_Delay(1);
  TMC2209_WriteGlobalConfig(handle, driver_index, 1, true, false, false, false);
  HAL_Delay(1);
  TMC2209_WriteDriverCurrent(handle, driver_index, 80, 40, 80);
  HAL_Delay(1);
  TMC2209_WriteCoolConfig(handle, driver_index, 4, 1, 10, 0, 0);
  HAL_Delay(1);
  TMC2209_WriteChopperConfig(handle, driver_index, 4, 5, 1, 3, 0, 0, 1, 0);
  HAL_Delay(1);
  TMC2209_WritePWMConfig(handle, driver_index, 40, 3, 1, 1, 0, 0, 0, 4);
  HAL_Delay(1);
}

/*==================== VACTUAL ====================*/

void TMC2209_VACTUAL(TMC2209_Handle* handle, uint8_t driver_index, int32_t speed)
{
  TMC2209_WriteDatagram(handle, driver_index, ADDRESS_VACTUAL, (uint32_t)speed);
}