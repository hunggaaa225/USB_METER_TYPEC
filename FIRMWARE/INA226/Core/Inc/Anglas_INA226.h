/*
 * Anglas_INA226.h - Fixed for STM32 HAL / Keil
 */

#ifndef INC_ANGLAS_INA226_H_
#define INC_ANGLAS_INA226_H_

#include "main.h"

extern I2C_HandleTypeDef hi2c1;

/* ================= I2C ADDRESS =================
   INA226 address 7-bit = 0x40
   STM32 HAL needs address shifted left 1 bit.
*/
#define INA226_I2C_ADDRESS_7BIT   0x40
#define INA226_I2C_ADDRESS        (INA226_I2C_ADDRESS_7BIT << 1)

#define INA226_I2C_ADDRESS_WRITE  INA226_I2C_ADDRESS
#define INA226_I2C_ADDRESS_READ   INA226_I2C_ADDRESS

/* ================= REGISTERS ================= */
#define INA_CONFIGURATION_REGISTER      0x00
#define INA_SHUNT_VOLTAGE_REGISTER      0x01
#define INA_BUS_VOLTAGE_REGISTER        0x02
#define INA_POWER_REGISTER              0x03
#define INA_CURRENT_REGISTER            0x04
#define INA_CALIBRATION_REGISTER        0x05
#define INA_MASK_ENABLE_REGISTER        0x06
#define INA_ALERT_LIMIT_REGISTER        0x07

/* ================= LSB =================
   INA226 datasheet:
   Bus voltage LSB   = 1.25mV
   Shunt voltage LSB = 2.5uV = 0.0025mV
*/
#define INA_BUS_VOLTAGE_LSB_V           0.00125f
#define INA_SHUNT_VOLTAGE_LSB_MV        0.0025f

/* ================= CONFIG ================= */
#define NO_RESET_INA226                 0x4000
#define RESET_INA226                    0xC000

#define AVG_1                           0
#define AVG_4                           1
#define AVG_16                          2
#define AVG_64                          3
#define AVG_128                         4
#define AVG_256                         5
#define AVG_512                         6
#define AVG_1024                        7

#define T_Vbus_140us                    0
#define T_Vbus_204us                    1
#define T_Vbus_332us                    2
#define T_Vbus_588us                    3
#define T_Vbus_1_1ms                    4
#define T_Vbus_2_116ms                  5
#define T_Vbus_4_156ms                  6
#define T_Vbus_8_244ms                  7

#define T_Vshunt_140us                  0
#define T_Vshunt_204us                  1
#define T_Vshunt_332us                  2
#define T_Vshunt_588us                  3
#define T_Vshunt_1_1ms                  4
#define T_Vshunt_2_116ms                5
#define T_Vshunt_4_156ms                6
#define T_Vshunt_8_244ms                7

#define MODE_POWER_DOWN                 0
#define MODE_SHUNT_VOLTAGE_TRIGGERED    1
#define MODE_BUS_VOLTAGE_TRIGGERED      2
#define MODE_SHUNT_BUS_TRIGGERED        3
#define MODE_SHUNT_VOLTAGE_CONTINUOUS   5
#define MODE_BUS_VOLTAGE_CONTINUOUS     6
#define MODE_SHUNT_BUS_CONTINUOUS       7

/* Alert modes */
#define SHUNT_VOLTAGE_OVER              0x8002
#define SHUNT_VOLTAGE_UNDER             0x4002
#define BUS_VOLTAGE_OVER                0x2002
#define BUS_VOLTAGE_UNDER               0x1002
#define OVER_LIMIT_POWER                0x0802

typedef struct
{
    uint16_t calibration;
    uint32_t current_LSB;     // nA per bit
    uint32_t power_LSB;       // uW per bit
    uint16_t Rshunt;          // milliohm
    uint16_t mask_Enable;
} Anglas_IN226;

/* ================= FUNCTIONS ================= */

uint8_t INA226_IsReady(void);
HAL_StatusTypeDef INA226_GetLastStatus(void);

void INA226_Init(const uint16_t maxBusMiliampers,
                 const uint16_t resMiliohmios,
                 uint16_t AVG,
                 uint16_t Time_Vbus,
                 uint16_t Time_Vshunt,
                 uint16_t ModeOperation);

float INA226_Shunt(void);     // return mV
float INA226_Vbus(void);      // return V
float INA226_Current(void);   // return mA
float INA226_Power(void);     // return W

uint16_t INA226_ReadRawRegister(uint8_t reg);
uint16_t INA226_Mode_pinAlert(uint16_t umbral);
uint16_t INA226_Alert_Limit(uint16_t limite);

#endif