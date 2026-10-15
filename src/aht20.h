/**
 ******************************************************************************
 * @file    aht20.h
 * @brief   Driver "orientado a objetos" (en C) para el sensor AHT20
 *          (temperatura + humedad, bus I2C).
 *
 * Implementa el patron visto en clase: cada objeto es un puntero a una
 * estructura opaca (handle). Los atributos son privados al .c y solo se
 * accede a ellos a traves de los metodos publicos declarados en aht20.h.
 ******************************************************************************
 */
#ifndef AHT20_H
#define AHT20_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/* Direccion I2C de 7 bits del AHT20 (fija por hardware) */
#define AHT20_I2C_ADDRESS   (0x38U << 1)

/**
 * @brief Estructura "publica" del objeto AHT20Sensor.
 *
 * Los campos con sufijo _ son privados por convencion (RNF-04) y no
 * deben modificarse fuera de aht20.c; se acceden mediante los metodos
 * AHT20_GetTemperature() / AHT20_GetHumidity().
 */
typedef struct {
    I2C_HandleTypeDef *hi2c_;      /* bus I2C utilizado (HAL)            */
    uint8_t            address_;   /* direccion I2C del dispositivo      */
    float               temperature_;
    float               humidity_;
    bool                calibrated_;
} AHT20Sensor;

/* ---- Metodos publicos (API del "objeto" AHT20Sensor) ---------------- */

/**
 * @brief Inicializa el objeto sensor y verifica/calibra el AHT20.
 * @param self  instancia del sensor (ya con hi2c_ asignado por el caller)
 * @retval true si el sensor respondió y quedó calibrado, false si no.
 */
bool AHT20_Init(AHT20Sensor *self, I2C_HandleTypeDef *hi2c);

/**
 * @brief Dispara una medición y actualiza temperature_/humidity_.
 *        Es una operación semi-bloqueante (~80 ms) por el tiempo de
 *        conversión propio del sensor; se llama desde el estado
 *        MEASURING del super-loop, no dentro de un ISR.
 * @retval true si la lectura fue valida (ACK + CRC de estado ok).
 */
bool AHT20_ReadMeasurement(AHT20Sensor *self);

/** @brief Última temperatura valida leida, en grados Celsius. */
float AHT20_GetTemperature(const AHT20Sensor *self);

/** @brief Última humedad relativa valida leida, en %RH. */
float AHT20_GetHumidity(const AHT20Sensor *self);

#ifdef __cplusplus
}
#endif

#endif /* AHT20_H */
