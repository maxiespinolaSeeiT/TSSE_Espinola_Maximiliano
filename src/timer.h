/**
 ******************************************************************************
 * @file    timer.h
 * @brief   Modulo "utility" con métodos estáticos para medir tiempo sin
 *          bloquear el super-loop (RNF-01) y sin acoplar el resto del
 *          software a una implementacion concreta de reloj (RNF-03).
 ******************************************************************************
 */
#ifndef TIMER_H
#define TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Milisegundos transcurridos desde el arranque del sistema.
 *        Hoy delega en HAL_GetTick(); si se migra de HAL a otra capa
 *        (o a otro microcontrolador), solo se reescribe timer.c.
 */
uint32_t Timer_Millis(void);

/** @brief Espera bloqueante en milisegundos (solo para init/medicion). */
void Timer_Delay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H */
