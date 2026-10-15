/**
 ******************************************************************************
 * @file    serial_display.h
 * @brief   "Clase" SerialDisplay: envía texto por el UART2 (puerto virtual
 *          del ST-LINK) para mostrar la temperatura en una terminal PuTTY,
 *          a 115200 8N1.
 *
 ******************************************************************************
 */

#ifndef SERIAL_DISPLAY_H
#define SERIAL_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

typedef struct {
    UART_HandleTypeDef *huart_;
} SerialDisplay;

/** @brief Asocia el objeto al periférico UART ya configurado por CubeMX. */
void SerialDisplay_Init(SerialDisplay *self, UART_HandleTypeDef *huart);

/** @brief Imprime una linea con la temperatura y humedad actuales. */
void SerialDisplay_PrintMeasurement(SerialDisplay *self, float temperature_c, float humidity_rh);

/** @brief Imprime un mensaje de error (p. ej. fallo de lectura I2C). */
void SerialDisplay_PrintError(SerialDisplay *self, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* SERIAL_DISPLAY_H */
