# Tests unitarios de `app.c`
**Espínola Maximiliano Ariel**

**SIU: e2603**

## Descripción

Este módulo contiene los tests unitarios de `app.c`, encargado de implementar la máquina de estados del *super-loop* de una aplicación embebida que utiliza un sensor de temperatura y humedad AHT20.

Las pruebas verifican el comportamiento de la aplicación durante la inicialización, la espera entre mediciones, la adquisición y visualización de datos y la recuperación ante errores.

## Objetivos

- Verificar las transiciones entre los estados de la aplicación.
- Comprobar que el período de muestreo sea de **5000 ms**.
- Validar la adquisición y visualización de temperatura y humedad.
- Verificar la recuperación ante fallas de inicialización y lectura del sensor.
- Comprobar el comportamiento del temporizador ante el desbordamiento de un contador de 32 bits.
- Ejecutar las pruebas de forma determinista, sin depender del hardware ni del tiempo real.

## Máquina de estados

La aplicación contempla los siguientes estados:

| Estado | Descripción |
|---|---|
| `INIT` | Inicialización del sensor AHT20. |
| `WAITING` | Espera hasta que se cumpla el período de muestreo. |
| `MEASURING` | Solicitud y adquisición de una medición. |
| `DISPLAYING` | Obtención y visualización de temperatura y humedad. |
| `ERROR` | Gestión de errores y espera antes de reintentar. |

### Transiciones principales

- `INIT → WAITING`: inicialización exitosa del sensor.
- `INIT → ERROR`: falla la inicialización del sensor.
- `WAITING → WAITING`: todavía no transcurrieron 5000 ms.
- `WAITING → MEASURING`: se cumple el período de muestreo.
- `MEASURING → DISPLAYING`: lectura exitosa.
- `MEASURING → ERROR`: falla la lectura.
- `DISPLAYING → WAITING`: finaliza la visualización y se reinicia el período.
- `ERROR → ERROR`: todavía no transcurrieron 2000 ms.
- `ERROR → INIT`: se cumple el tiempo de espera y se permite reintentar la inicialización.

## Herramientas y aislamiento

Las pruebas utilizan **Unity** como framework de tests unitarios y **CMock** para simular las dependencias externas de `app.c`.

| Dependencia | Mecanismo de simulación |
|---|---|
| Sensor AHT20 | `mock_aht20.h` |
| Temporizador | `mock_timer.h` |
| Salida serial | `mock_serial_display.h` |

El reloj se simula mediante un callback de `Timer_Millis()`. La variable `now` permite establecer el instante de ejecución de cada ciclo sin esperar en tiempo real.

La función auxiliar `tick(t)` actualiza el reloj simulado y ejecuta una llamada a `App_Run(app)`.

Las expectativas de CMock permiten verificar las llamadas realizadas, sus argumentos y los valores de retorno. De esta manera, el estado interno de la máquina no necesita exponerse a los tests.

## Casos de prueba

### Test 1 — Inicialización y período de muestreo

**Función:** `test_WAITING_espera_5000ms_y_pasa_a_MEASURING_en_el_limite`

Verifica la inicialización exitosa y el comportamiento en los límites del período de muestreo.

- A los 4999 ms, la aplicación debe permanecer en `WAITING`.
- A los 5000 ms, debe pasar a `MEASURING`.
- En el siguiente ciclo, debe solicitar una medición al sensor.

### Test 2 — Lectura y visualización exitosa

**Función:** `test_lectura_ok_imprime_medicion_y_reinicia_el_periodo`

Comprueba la transición de `MEASURING` a `DISPLAYING`, la visualización de los valores simulados y el retorno a `WAITING`.

Valores utilizados:
- Temperatura: `23.5 °C`.
- Humedad relativa: `55.0 %`.

También verifica que el siguiente período de muestreo se cuente desde la visualización y no desde el arranque de la aplicación.

### Test 3 — Falla de inicialización y recuperación

**Función:** `test_INIT_fallido_reporta_espera_2s_y_reintenta`

Simula una falla de inicialización del sensor.

- La aplicación debe pasar de `INIT` a `ERROR`.
- Debe informar que el sensor no responde en el bus I²C.
- Antes de los 2000 ms, no debe ejecutar el reintento.
- Al cumplirse los 2000 ms, debe informar el reintento y volver a `INIT`.
- Una inicialización exitosa posterior debe permitir regresar a `WAITING`.

### Test 4 — Falla durante la medición

**Función:** `test_lectura_fallida_en_MEASURING_respeta_2s_antes_de_reportar`

Simula una lectura fallida durante el funcionamiento normal.

Verifica que la aplicación pase a `ERROR`, respete el tiempo de espera de 2000 ms antes de informar el reintento y vuelva a intentar la inicialización del sensor.

### Test 5 — Desbordamiento del contador de milisegundos

**Función:** `test_WAITING_respeta_5000ms_con_desbordamiento_del_contador`

Comprueba que el período de muestreo se respete incluso cuando el contador de 32 bits pasa de su valor máximo a cero.

La prueba verifica que la aplicación continúe esperando a los 4999 ms transcurridos y habilite la medición al alcanzar los 5000 ms.

Este comportamiento requiere que la implementación calcule el tiempo transcurrido mediante una resta sin signo compatible con el desbordamiento.

## Trazabilidad de transiciones

| Transición | Test |
|---|---|
| T1 — `INIT → WAITING` | 1, 2, 3, 4 y 5 |
| T2 — `INIT → ERROR` | 3 |
| T3 — `WAITING → WAITING` | 1, 2 y 5 |
| T4 — `WAITING → MEASURING` | 1, 2 y 5 |
| T5 — `MEASURING → DISPLAYING` | 2 |
| T6 — `MEASURING → ERROR` | 4 |
| T7 — `DISPLAYING → WAITING` | 2 |
| T8 — `ERROR → ERROR` | 3 y 4 |
| T9 — `ERROR → INIT` | 3 y 4 |
| T10 — Estado por defecto → `ERROR` | No cubierto |

La transición T10 corresponde a una rama defensiva que se considera inalcanzable a través de la API pública utilizada por estas pruebas.

## Criterios de verificación

Los tests se consideran exitosos cuando se cumplen las expectativas de CMock y Unity:

- No se producen llamadas inesperadas a las dependencias simuladas.
- Se ejecutan las llamadas esperadas con los argumentos definidos.
- Se respetan los límites temporales de muestreo y recuperación.
- Se cumplen las transiciones previstas para cada escenario.

Los porcentajes de cobertura de líneas y ramas se obtienen mediante la herramienta de cobertura configurada en el entorno de compilación. La cobertura de transiciones constituye un criterio de verificación complementario.

## Alcance y limitaciones

Estas pruebas verifican la lógica de control de `app.c` de forma aislada. No validan la comunicación I²C física, la precisión del sensor, el funcionamiento eléctrico de los periféricos ni la transmisión UART real.

La inicialización, las mediciones, el tiempo y la salida serial se simulan para lograr pruebas repetibles e independientes del hardware.

## Conclusión

El conjunto de pruebas aplica la técnica **State Transition Testing (STT)** para verificar el comportamiento normal y los escenarios de error de la máquina de estados. Incluye pruebas de valores límite, recuperación ante fallas y robustez frente al desbordamiento del temporizador, contribuyendo a validar el funcionamiento determinista y no bloqueante de la aplicación.
