/**
 ******************************************************************************
 * @file    app.h
 * @brief   "Clase" App: contiene TODA la lógica del programa (maquina de
 *          estados del super-loop, orquestación de AHT20Sensor, Timer y
 *          SerialDisplay). Es el único modulo del TP que main.c conoce.
 *
 * Diseño (RNF-02, SRP): main.c no sabe nada de I2C, UART, ni del protocolo
 * del AHT20 -- solo inicializa la HAL y le entrega los handles a App_Init().
 * Todo lo demas (que hacer con esos handles, cuando leer, que imprimir)
 * vive acá adentro.
 ******************************************************************************
 */
#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/**
 * @brief Instancia del programa. Es opaca para el resto del sistema:
 *        main.c solo declara una variable de este tipo y la pasa por
 *        puntero a App_Init()/App_Run(); nunca toca sus campos.
 */
typedef struct App App;

/**
 * @brief Devuelve un puntero a la (única) instancia estatica de App.
 *        Evita que main.c tenga que conocer el tamano de la estructura
 *        (que queda totalmente privada en app.c).
 */
App *App_GetInstance(void);

/**
 * @brief Inicializa el programa: configura SerialDisplay y AHT20Sensor
 *        sobre los handles de HAL ya inicializados por main(), y deja
 *        el sistema listo para arrancar el super-loop.
 * @param self  instancia devuelta por App_GetInstance()
 * @param hi2c  handle de I2C ya inicializado (MX_I2C1_Init)
 * @param huart handle de UART ya inicializado (MX_USART2_UART_Init)
 */
void App_Init(App *self, I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart);

/**
 * @brief Un tick del super-loop de la aplicación. Se llama en cada vuelta
 *        del while(1) de main(); internamente avanza la maquina de estados
 *        (INIT/WAITING/MEASURING/DISPLAYING/ERROR) sin bloquear salvo en
 *        MEASURING (ver informe, seccion de modelado de comportamiento).
 */
void App_Run(App *self);

#ifdef __cplusplus
}
#endif

#endif /* APP_H */
