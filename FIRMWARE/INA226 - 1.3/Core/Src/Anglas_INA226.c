/*
 * Anglas_INA226.c - Fixed for STM32 HAL / Keil
 */

#include "Anglas_INA226.h"

Anglas_IN226 ina;

static HAL_StatusTypeDef ina_last_status = HAL_OK;

/* ================= LOW LEVEL FUNCTIONS ================= */

static HAL_StatusTypeDef INA226_WriteRegister(uint8_t reg, uint16_t value)
{
    uint8_t data[2];

    data[0] = (uint8_t)(value >> 8);
    data[1] = (uint8_t)(value & 0xFF);

    ina_last_status = HAL_I2C_Mem_Write(&hi2c1,
                                        INA226_I2C_ADDRESS,
                                        reg,
                                        I2C_MEMADD_SIZE_8BIT,
                                        data,
                                        2,
                                        100);

    return ina_last_status;
}

static HAL_StatusTypeDef INA226_ReadRegister(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];

    ina_last_status = HAL_I2C_Mem_Read(&hi2c1,
                                       INA226_I2C_ADDRESS,
                                       reg,
                                       I2C_MEMADD_SIZE_8BIT,
                                       data,
                                       2,
                                       100);

    if (ina_last_status != HAL_OK)
    {
        *value = 0xFFFF;
        return ina_last_status;
    }

    *value = ((uint16_t)data[0] << 8) | data[1];

    return HAL_OK;
}

/* ================= PUBLIC FUNCTIONS ================= */

uint8_t INA226_IsReady(void)
{
    if (HAL_I2C_IsDeviceReady(&hi2c1, INA226_I2C_ADDRESS, 3, 100) == HAL_OK)
    {
        return 1;
    }

    return 0;
}

HAL_StatusTypeDef INA226_GetLastStatus(void)
{
    return ina_last_status;
}

uint16_t INA226_ReadRawRegister(uint8_t reg)
{
    uint16_t value;

    if (INA226_ReadRegister(reg, &value) != HAL_OK)
    {
        return 0xFFFF;
    }

    return value;
}

/*
   maxBusMiliampers: max current in mA
   resMiliohmios   : shunt resistor in milliohm

   Example for R010:
   R010 = 0.01 ohm = 10 milliohm

   INA226_Init(8000, 10, AVG_16, T_Vbus_1_1ms, T_Vshunt_1_1ms, MODE_SHUNT_BUS_CONTINUOUS);
*/
void INA226_Init(const uint16_t maxBusMiliampers,
                 const uint16_t resMiliohmios,
                 uint16_t AVG,
                 uint16_t Time_Vbus,
                 uint16_t Time_Vshunt,
                 uint16_t ModeOperation)
{
    uint16_t config;
    uint64_t cal_temp;
    uint64_t denominator;

    if (maxBusMiliampers == 0 || resMiliohmios == 0)
    {
        return;
    }

    ina.Rshunt = resMiliohmios;

    /* Optional reset */
    INA226_WriteRegister(INA_CONFIGURATION_REGISTER, RESET_INA226);
    HAL_Delay(2);

    config = NO_RESET_INA226 |
             (AVG << 9) |
             (Time_Vbus << 6) |
             (Time_Vshunt << 3) |
             ModeOperation;

    INA226_WriteRegister(INA_CONFIGURATION_REGISTER, config);

    /*
       current_LSB = maxCurrent / 32768

       Unit here:
       maxBusMiliampers = mA
       current_LSB = nA per bit
    */
    ina.current_LSB = ((uint64_t)maxBusMiliampers * 1000000ULL) / 32768ULL;

    if (ina.current_LSB == 0)
    {
        ina.current_LSB = 1;
    }

    /*
       Calibration = 0.00512 / (Current_LSB_A * Rshunt_Ohm)

       current_LSB is nA
       Rshunt is milliohm

       Calibration = 5120000000 / (current_LSB_nA * Rshunt_mOhm)
    */
    denominator = (uint64_t)ina.current_LSB * (uint64_t)resMiliohmios;

    if (denominator == 0)
    {
        return;
    }

    cal_temp = 5120000000ULL / denominator;

    if (cal_temp > 0xFFFF)
    {
        cal_temp = 0xFFFF;
    }

    ina.calibration = (uint16_t)cal_temp;

    /*
       Power LSB = 25 * Current LSB

       current_LSB is nA
       25 * current_LSB = nW
       /1000 => uW
    */
    ina.power_LSB = (uint32_t)((25ULL * (uint64_t)ina.current_LSB) / 1000ULL);

    if (ina.power_LSB == 0)
    {
        ina.power_LSB = 1;
    }

    INA226_WriteRegister(INA_CALIBRATION_REGISTER, ina.calibration);
}

/*
   Return shunt voltage in mV.
   Shunt register is signed.
*/
float INA226_Shunt(void)
{
    uint16_t raw_u;
    int16_t raw_s;

    if (INA226_ReadRegister(INA_SHUNT_VOLTAGE_REGISTER, &raw_u) != HAL_OK)
    {
        return -9999.0f;
    }

    raw_s = (int16_t)raw_u;

    return raw_s * INA_SHUNT_VOLTAGE_LSB_MV;
}

/*
   Return bus voltage in V.
   Bus voltage LSB = 1.25mV = 0.00125V.
*/
float INA226_Vbus(void)
{
    uint16_t raw;

    if (INA226_ReadRegister(INA_BUS_VOLTAGE_REGISTER, &raw) != HAL_OK)
    {
        return -9999.0f;
    }

    return raw * INA_BUS_VOLTAGE_LSB_V;
}

/*
   Return current in mA.
   Current register is signed.
*/
float INA226_Current(void)
{
    uint16_t raw_u;
    int16_t raw_s;
    int64_t current_nA;

    if (INA226_ReadRegister(INA_CURRENT_REGISTER, &raw_u) != HAL_OK)
    {
        return -9999.0f;
    }

    raw_s = (int16_t)raw_u;

    current_nA = (int64_t)raw_s * (int64_t)ina.current_LSB;

    return current_nA / 1000000.0f;
}

/*
   Return power in W.
*/
float INA226_Power(void)
{
    uint16_t raw;
    uint64_t power_uW;

    if (INA226_ReadRegister(INA_POWER_REGISTER, &raw) != HAL_OK)
    {
        return -9999.0f;
    }

    power_uW = (uint64_t)raw * (uint64_t)ina.power_LSB;

    return power_uW / 1000000.0f;
}

uint16_t INA226_Mode_pinAlert(uint16_t umbral)
{
    uint16_t dato;

    ina.mask_Enable = umbral;

    INA226_WriteRegister(INA_MASK_ENABLE_REGISTER, umbral);

    if (INA226_ReadRegister(INA_MASK_ENABLE_REGISTER, &dato) != HAL_OK)
    {
        return 0xFFFF;
    }

    return dato;
}

uint16_t INA226_Alert_Limit(uint16_t limite)
{
    uint32_t raw_limit;
    uint16_t dato;

    raw_limit = limite;

    /*
       If limite is current in mA:
       Vshunt_mV = I_mA * R_mOhm / 1000
       raw = Vshunt_mV / 0.0025
           = I_mA * R_mOhm * 0.4
    */
    switch (ina.mask_Enable)
    {
        case SHUNT_VOLTAGE_OVER:
        case SHUNT_VOLTAGE_UNDER:
            raw_limit = ((uint32_t)limite * (uint32_t)ina.Rshunt * 4U) / 10U;
            break;

        case BUS_VOLTAGE_OVER:
        case BUS_VOLTAGE_UNDER:
            /* limite is mV, bus LSB = 1.25mV, raw = mV / 1.25 = mV * 0.8 */
            raw_limit = ((uint32_t)limite * 8U) / 10U;
            break;

        case OVER_LIMIT_POWER:
            /* limite is mW, power_LSB is uW */
            if (ina.power_LSB != 0)
            {
                raw_limit = ((uint32_t)limite * 1000U) / ina.power_LSB;
            }
            break;

        default:
            break;
    }

    if (raw_limit > 0xFFFF)
    {
        raw_limit = 0xFFFF;
    }

    INA226_WriteRegister(INA_ALERT_LIMIT_REGISTER, (uint16_t)raw_limit);

    if (INA226_ReadRegister(INA_ALERT_LIMIT_REGISTER, &dato) != HAL_OK)
    {
        return 0xFFFF;
    }

    return dato;
}