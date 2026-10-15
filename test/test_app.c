/**
 ******************************************************************************
 * @file    test_app.c
 * @brief   Tests unitarios de app.c (maquina de estados del super-loop).
 *
 * Tecnica de diseno: State Transition Testing (STT). Cada test cubre un
 * grupo de transiciones del diagrama de estados (ver tabla en el informe):

 *   T1  INIT      --[AHT20_Init ok]----------> WAITING      -> test 1
 *   T2  INIT      --[AHT20_Init falla]-------> ERROR        -> test 3
 *   T3  WAITING   --[dt <  5000 ms]----------> WAITING      -> test 1 y 5
 *   T4  WAITING   --[dt >= 5000 ms]----------> MEASURING    -> test 1 y 5
 *   T5  MEASURING --[lectura ok]-------------> DISPLAYING   -> test 2
 *   T6  MEASURING --[lectura falla]----------> ERROR        -> test 4
 *   T7  DISPLAYING-----------------------------> WAITING    -> test 2
 *   T8  ERROR     --[dt <  2000 ms]----------> ERROR        -> test 3 y 4
 *   T9  ERROR     --[dt >= 2000 ms]----------> INIT         -> test 3 y 4
 *   T10 default   ---------------------------> ERROR        (inalcanzable por
 *       la API publica: codigo defensivo, queda fuera de la cobertura)
 *
 * Aislamiento: AHT20Sensor, Timer y SerialDisplay estan mockeados con CMock.
 * El estado interno de App es privado, asi que se verifica por COMPORTAMIENTO:
 * CMock falla ante cualquier llamada no esperada, por lo que si la maquina
 * esta en un estado distinto al previsto, el test falla.
 *
 * El reloj se simula con un stub (callback) sobre Timer_Millis(): los tests no
 * dependen de CUANTAS veces app.c consulta el tiempo, solo del valor "now".
 ******************************************************************************
 */
#include "unity.h"

#include "app.h"
#include "mock_aht20.h"
#include "mock_timer.h"
#include "mock_serial_display.h"

#define T0                  100000U   /* instante inicial arbitrario (ms)   */
#define SAMPLE_PERIOD_MS      5000U   /* RF-01/RF-02                         */
#define ERROR_TIMEOUT_MS      2000U

static App               *app;
static I2C_HandleTypeDef  hi2c;
static UART_HandleTypeDef huart;
static uint32_t           now;        /* reloj simulado                      */

/* ---- Fakes / helpers ---------------------------------------------------- */
static uint32_t fake_millis(int cmock_num_calls)
{
    (void)cmock_num_calls;
    return now;
}

/** Avanza el reloj simulado a t y ejecuta un tick del super-loop. */
static void tick(uint32_t t)
{
    now = t;
    App_Run(app);
}

static void expect_aht20_init(bool result)
{
    AHT20_Init_ExpectAndReturn(NULL, &hi2c, result);
    AHT20_Init_IgnoreArg_self();
}

static void expect_read_measurement(bool result)
{
    AHT20_ReadMeasurement_ExpectAndReturn(NULL, result);
    AHT20_ReadMeasurement_IgnoreArg_self();
}

static void expect_print_error(const char *msg)
{
    SerialDisplay_PrintError_Expect(NULL, msg);
    SerialDisplay_PrintError_IgnoreArg_self();
}

/** Deja la app en WAITING con lastReadTime_ = t. */
static void go_to_waiting(uint32_t t)
{
    expect_aht20_init(true);
    tick(t);                                  /* INIT -> WAITING */
}

/* ---- Fixture ------------------------------------------------------------ */
void setUp(void)
{
    now = T0;
    Timer_Millis_StubWithCallback(fake_millis);

    SerialDisplay_Init_Expect(NULL, &huart);
    SerialDisplay_Init_IgnoreArg_self();

    app = App_GetInstance();
    App_Init(app, &hi2c, &huart);             /* estado = INIT */
}

void tearDown(void)
{
}

/* ========================================================================== */
/* Test 1 - T1, T3, T4: INIT ok y periodo de muestreo (valores limite)       */
/* Trazabilidad: RF-01 / RF-02 (periodo de 5000 ms), RNF-01 (no bloqueante)  */
/* ========================================================================== */
void test_WAITING_espera_5000ms_y_pasa_a_MEASURING_en_el_limite(void)
{
    go_to_waiting(T0);                        /* T1 */

    tick(T0 + SAMPLE_PERIOD_MS - 1U);         /* T3: 4999 ms -> sigue WAITING.
                                                 Si midiera, CMock fallaria  */
    tick(T0 + SAMPLE_PERIOD_MS);              /* T4: 5000 ms -> MEASURING    */

    expect_read_measurement(true);            /* prueba de que esta en
                                                 MEASURING: el tick siguiente
                                                 invoca al sensor            */
    tick(T0 + SAMPLE_PERIOD_MS);
}

/* ========================================================================== */
/* Test 2 - T5, T7: lectura OK -> DISPLAYING -> WAITING (timer reiniciado)   */
/* Trazabilidad: RF-01 (medir y mostrar T/HR cada 5 s)                       */
/* ========================================================================== */
void test_lectura_ok_imprime_medicion_y_reinicia_el_periodo(void)
{
    const uint32_t t_display = T0 + SAMPLE_PERIOD_MS + 10U;

    go_to_waiting(T0);
    tick(T0 + SAMPLE_PERIOD_MS);              /* WAITING -> MEASURING        */

    expect_read_measurement(true);
    tick(T0 + SAMPLE_PERIOD_MS);              /* T5: MEASURING -> DISPLAYING */

    AHT20_GetTemperature_ExpectAndReturn(NULL, 23.5f);
    AHT20_GetTemperature_IgnoreArg_self();
    AHT20_GetHumidity_ExpectAndReturn(NULL, 55.0f);
    AHT20_GetHumidity_IgnoreArg_self();
    SerialDisplay_PrintMeasurement_Expect(NULL, 23.5f, 55.0f);
    SerialDisplay_PrintMeasurement_IgnoreArg_self();
    tick(t_display);                          /* T7: DISPLAYING -> WAITING   */

    /* El periodo se cuenta desde la impresion, no desde el arranque */
    tick(t_display + SAMPLE_PERIOD_MS - 1U);  /* sigue WAITING               */
    tick(t_display + SAMPLE_PERIOD_MS);       /* -> MEASURING                */
    expect_read_measurement(true);
    tick(t_display + SAMPLE_PERIOD_MS);       /* confirma MEASURING          */
}

/* ========================================================================== */
/* Test 3 - T2, T8, T9: sensor ausente al arrancar -> ERROR -> reintento     */
/* Trazabilidad: RNF de robustez (recuperacion ante falla del bus I2C)       */
/* ========================================================================== */
void test_INIT_fallido_reporta_espera_2s_y_reintenta(void)
{
    expect_aht20_init(false);
    expect_print_error("AHT20 no responde en el bus I2C");
    tick(T0);                                 /* T2: INIT -> ERROR           */

    tick(T0 + ERROR_TIMEOUT_MS - 1U);         /* T8: 1999 ms -> silencio     */

    expect_print_error("Lectura AHT20 fallida, reintentando");
    tick(T0 + ERROR_TIMEOUT_MS);              /* T9: 2000 ms -> INIT         */

    expect_aht20_init(true);                  /* reintenta la inicializacion */
    tick(T0 + ERROR_TIMEOUT_MS);              /* INIT -> WAITING             */
}

/* ========================================================================== */
/* Test 4 - T6, T8, T9: falla de lectura en MEASURING -> ERROR               */
/* Debe respetar los 2 s de ERROR antes de reportar/reintentar.              */
/* ========================================================================== */
void test_lectura_fallida_en_MEASURING_respeta_2s_antes_de_reportar(void)
{
    const uint32_t t_fail = T0 + SAMPLE_PERIOD_MS;

    go_to_waiting(T0);
    tick(t_fail);                             /* WAITING -> MEASURING        */

    expect_read_measurement(false);
    tick(t_fail);                             /* T6: MEASURING -> ERROR      */

    tick(t_fail + ERROR_TIMEOUT_MS - 1U);     /* T8: no debe reportar aun    */

    expect_print_error("Lectura AHT20 fallida, reintentando");
    tick(t_fail + ERROR_TIMEOUT_MS);          /* T9: reporta y vuelve a INIT */

    expect_aht20_init(true);
    tick(t_fail + ERROR_TIMEOUT_MS);          /* reintento exitoso           */
}

/* ========================================================================== */
/* Test 5 - T3, T4: desbordamiento del contador de ms (guarda G5)            */
/* El periodo de 5000 ms debe respetarse aunque Timer_Millis() pase de       */
/* 0xFFFFFFFF a 0 entre lastReadTime_ y el instante actual (resta sin signo).*/
/* Trazabilidad: RF-01 / RF-02 (periodo de 5000 ms), robustez del reloj      */
/* ========================================================================== */
void test_WAITING_respeta_5000ms_con_desbordamiento_del_contador(void)
{
    const uint32_t t_wrap = 0xFFFFFF00U;      /* a 256 ms de desbordar       */

    go_to_waiting(t_wrap);                    /* T1: lastReadTime_ = t_wrap  */

    tick(t_wrap + SAMPLE_PERIOD_MS - 1U);     /* T3: 4999 ms (el contador ya
                                                 desbordo) -> sigue WAITING   */
    tick(t_wrap + SAMPLE_PERIOD_MS);          /* T4: 5000 ms -> MEASURING    */

    expect_read_measurement(true);            /* prueba de que esta en
                                                 MEASURING                   */
    tick(t_wrap + SAMPLE_PERIOD_MS);
}