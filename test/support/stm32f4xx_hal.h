/**
 * @file    stm32f4xx_hal.h  (STUB solo para tests en host)
 * @brief   Reemplaza al header real de ST para que app.c, aht20.h y
 *          serial_display.h compilen en la PC. Solo define los tipos que
 *          esos headers necesitan; no hay ninguna funcion de HAL.
 */
#ifndef STM32F4XX_HAL_H
#define STM32F4XX_HAL_H

#include <stdint.h>

typedef struct { uint32_t dummy; } I2C_HandleTypeDef;
typedef struct { uint32_t dummy; } UART_HandleTypeDef;

typedef enum {
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

#endif /* STM32F4XX_HAL_H */
