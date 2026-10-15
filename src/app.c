/**
 ******************************************************************************
 * @file    app.c
 * @brief   Implementacion de la clase App. Aca vive toda la logica del
 *          programa: la definicion real de la estructura (privada a este
 *          archivo, siguiendo el patron de "puntero a estructura anonima"
 *          visto en clase 6 para representar objetos en C) y la maquina
 *          de estados del super-loop.
 ******************************************************************************
 */
#include "app.h"
#include "aht20.h"
#include "timer.h"
#include "serial_display.h"

/**
 * @brief Estados del ciclo de medicion (ver diagrama de estados del TP).
 *        Corresponden al patron "Observar y reaccionar" con Super Loop
 *        no bloqueante (RNF-01): WAITING solo consulta Timer_Millis(),
 *        nunca usa HAL_Delay() en ese estado.
 */
typedef enum {
    APP_STATE_INIT = 0,
    APP_STATE_WAITING,
    APP_STATE_MEASURING,
    APP_STATE_DISPLAYING,
    APP_STATE_ERROR
} AppState;

/**
 * @brief Definicion real de App. Es privada a app.c: fuera de este archivo
 *        solo se conoce como "struct App" (tipo incompleto), por lo que
 *        main.c no puede acceder a estos campos ni le importa como estan
 *        organizados (encapsulamiento).
 */
struct App {
    I2C_HandleTypeDef *hi2c_;   /* bus I2C recibido en App_Init() */
    AHT20Sensor    sensor_;
    SerialDisplay  display_;
    AppState       state_;
    uint32_t       lastReadTime_;
    uint32_t       errorTime_;
};

#define APP_SAMPLE_PERIOD_MS 5000U /* RF-01/RF-02: periodo de muestreo */
#define APP_ERROR_TIMEOUT_MS 2000U /* espera en ERROR antes de reintentar */

/* Unica instancia del programa (RNF-05: sin heap, todo estatico). */
static App s_app;

App *App_GetInstance(void)
{
    return &s_app;
}

void App_Init(App *self, I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart)
{
    SerialDisplay_Init(&self->display_, huart);

    self->hi2c_ = hi2c; /* se usa en el estado INIT de App_Run() */
    self->state_ = APP_STATE_INIT;
    self->lastReadTime_ = Timer_Millis();
    self->errorTime_ = self->lastReadTime_;
}

void App_Run(App *self)
{
    switch (self->state_) {

    case APP_STATE_WAITING:
        if ((Timer_Millis() - self->lastReadTime_) >= APP_SAMPLE_PERIOD_MS) {
            self->state_ = APP_STATE_MEASURING;
        }
        break;

    case APP_STATE_MEASURING:
        if (AHT20_ReadMeasurement(&self->sensor_)) {
            self->state_ = APP_STATE_DISPLAYING;
        } else {
            self->errorTime_ = Timer_Millis(); /* arranca la espera de ERROR */
            self->state_ = APP_STATE_ERROR;
        }
        break;

    case APP_STATE_DISPLAYING:
        SerialDisplay_PrintMeasurement(&self->display_,
                                        AHT20_GetTemperature(&self->sensor_),
                                        AHT20_GetHumidity(&self->sensor_));
        self->lastReadTime_ = Timer_Millis();
        self->state_ = APP_STATE_WAITING;
        break;

    case APP_STATE_ERROR:
        if ((Timer_Millis() - self->errorTime_) >= APP_ERROR_TIMEOUT_MS) {
            SerialDisplay_PrintError(&self->display_,
                                      "Lectura AHT20 fallida, reintentando");
            self->lastReadTime_ = Timer_Millis();
            self->state_ = APP_STATE_INIT;
        }
        break;

    case APP_STATE_INIT:
        if (AHT20_Init(&self->sensor_, self->hi2c_)) {
            self->state_ = APP_STATE_WAITING;
            self->lastReadTime_ = Timer_Millis();
        } else {
            self->state_ = APP_STATE_ERROR;
            self->errorTime_ = Timer_Millis();
            SerialDisplay_PrintError(&self->display_,
                                      "AHT20 no responde en el bus I2C");
        }
        break;

    default:
        /* Codigo defensivo: App_Init() deja INIT y las transiciones solo
         * producen estados validos, por lo que no deberia llegar aca. */
        self->state_ = APP_STATE_ERROR;
        self->errorTime_ = Timer_Millis();
        break;
    }
}
