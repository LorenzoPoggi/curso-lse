#include "app_tasks.h"

// Setpoint 
uint8_t setpoint = 25; // Valor inicial del setpoint (25%)

// Cola para datos del ADC
xQueueHandle cola_adc;
// Cola para selecion de valor para el display
xQueueHandle cola_display_variable;
// Cola para datos de luminosidad
xQueueHandle cola_luz;
// Cola para datos del display
xQueueHandle cola_display;
// Cola para porcentaje de luminosidad
xQueueHandle cola_luz_percent; // Agregá esto al inicio del archivo


// Semáforo para interrupción del infrarojo
xSemaphoreHandle semaforo_buzzer;
// Semáforo para interrupción del user button
xSemaphoreHandle semaforo_usr;
// Semáforo para interrupción del touch
xSemaphoreHandle semaforo_touch;
// Semáforo para contador
xSemaphoreHandle semaforo_cont;
// Semáforo mutex para el display
xSemaphoreHandle semaforo_mutex;
// Semáforo para el CNY70
xSemaphoreHandle semaforo_cny70;

// Handler para la tarea de display write
TaskHandle_t handle_display;

/**
 * @brief Inicializa todos los perifericos y colas
 */
void tarea_inic(void *params) {
	// Inicializo semáforos
	semaforo_buzzer = xSemaphoreCreateBinary();
	semaforo_usr = xSemaphoreCreateBinary();
	semaforo_touch = xSemaphoreCreateBinary();
	semaforo_cny70 = xSemaphoreCreateBinary();
	semaforo_cont = xSemaphoreCreateCounting(99, 30);
	semaforo_mutex = xSemaphoreCreateMutex();

	// Inicializo colas
	cola_adc = xQueueCreate(1, sizeof(adc_data_t));
	cola_display_variable = xQueueCreate(1, sizeof(display_variable_t));
	cola_luz = xQueueCreate(1, sizeof(uint16_t));
	cola_display = xQueueCreate(1, sizeof(uint16_t));
	cola_luz_percent = xQueueCreate(1, sizeof(uint8_t));

	
	// Inicializacion de GPIO
	wrapper_gpio_init(0);
	wrapper_gpio_init(1);
	// Inicialización del LED
	wrapper_output_init((gpio_t){LED}, true);
	// Inicialización del buzzer
	wrapper_output_init((gpio_t){BUZZER}, false);
	// Inicialización del enable del CNY70
	wrapper_output_init((gpio_t){CNY70_EN}, true);
	// Configuro el ADC
	wrapper_adc_init();
	// Configuro el display
	wrapper_display_init();
	// Configuro botones
	wrapper_btn_init();
	// Configuro interrupción por flancos para el infrarrojo
    wrapper_gpio_enable_irq((gpio_t){CNY70}, kPINT_PinIntEnableBothEdges, cny70_callback);
	// Inicializo el PWM
	wrapper_pwm_init();
	// Inicializo I2C y Bh1750
	wrapper_i2c_init();
	wrapper_bh1750_init();
	// Inicializo el pulsador capacitivo
	wrapper_touch_init();

	// Elimino tarea para liberar recursos
	vTaskDelete(NULL);
}

/**
 * @brief Tarea que ajusta el setpoint de luminosidad con S1 y S2 (25% a 75%)
 */
void tarea_setpoint(void *params) {
    while(1) {
        // S1 incrementa el setpoint
        if(!wrapper_btn_get_with_debouncing_with_pull_up((gpio_t){S1_BTN})) {
            if(setpoint < 75) setpoint++;
            vTaskDelay(pdMS_TO_TICKS(200)); // Anti-rebote
        }
        // S2 decrementa el setpoint
        if(!wrapper_btn_get_with_debouncing_with_pull_up((gpio_t){S2_BTN})) {
            if(setpoint > 25) setpoint--;
            vTaskDelay(pdMS_TO_TICKS(200)); // Anti-rebote
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Activa una secuencia de conversion cada 0.25 segundos
 */
void tarea_adc(void *params) {

	while(1) {
		// Inicio una conversion
		ADC_DoSoftwareTriggerConvSeqA(ADC0);
		// Bloqueo la tarea por 250 ms
		vTaskDelay(pdMS_TO_TICKS(250));
	}
}

/**
 * @brief Lee los valores de los botones para definir que valor mostrar
 */
void tarea_display_change(void *params) {
    display_variable_t variable = kDISPLAY_LUZ;

    while(1) {
        xQueueOverwrite(cola_display_variable, &variable);
        xSemaphoreTake(semaforo_usr, portMAX_DELAY);
        variable = (variable == kDISPLAY_LUZ) ? kDISPLAY_SETPOINT : kDISPLAY_LUZ;
    }
}

/**
 * @brief Escribe valores en el display
 */
void tarea_control(void *params) {
    display_variable_t variable;
    uint8_t lux_percent = 0;
    uint16_t val = 0;

    while(1) {
        // Lee el modo actual desde la cola
        xQueuePeek(cola_display_variable, &variable, portMAX_DELAY);

        if(variable == kDISPLAY_LUZ) {
    		// Muestra el porcentaje de luminosidad
    		xQueuePeek(cola_luz_percent, &lux_percent, portMAX_DELAY);
    		val = lux_percent;
		} else if(variable == kDISPLAY_SETPOINT) {
    		// Muestra el setpoint ajustado con S1/S2
    		val = setpoint;
		}

        xSemaphoreTake(semaforo_mutex, portMAX_DELAY);
        xQueueOverwrite(cola_display, &val);
        xSemaphoreGive(semaforo_mutex);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Tarea que escribe un número en el display
 */
void tarea_display(void *params) {
	// Variable con el dato para escribir
	uint8_t data;

	while(1) {
		// Mira el dato que haya en la cola
		if(!xQueuePeek(cola_display, &data, pdMS_TO_TICKS(100))) { continue; }
		// Muestro el número
		wrapper_display_off();
		wrapper_display_write((uint8_t)(data / 10));
		wrapper_display_on((gpio_t){COM_1});
		vTaskDelay(pdMS_TO_TICKS(10));
		wrapper_display_off();
		wrapper_display_write((uint8_t)(data % 10));
		wrapper_display_on((gpio_t){COM_2});
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/**
 * @brief Actualiza el duty del PWM
 */
void tarea_pwm(void *params) {
	// Variable para guardar los datos del ADC
	adc_data_t data = {0};

	while(1) {
		// Bloqueo hasta que haya algo que leer
		xQueuePeek(cola_adc, &data, portMAX_DELAY);
		// Calculo la diferencia
		int16_t err = 100 * (data.ref_raw - data.temp_raw) / 4095;
		// Actualizo el duty
		if(err > 0) {
			// Referencia por arriba, quiero calentar -> LED rojo
			wrapper_pwm_update_bled(0);
			wrapper_pwm_update_rled(err);
		}
		else {
			// Referencia por debajo, quiero enfriar -> LED azul
			wrapper_pwm_update_rled(0);
			wrapper_pwm_update_bled(-err);
		}
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

/**
 * @brief Lee periodicamente el valor de intensidad luminica
 */
void tarea_bh1750(void *params) {
    uint16_t lux = 0;
    uint8_t lux_percent = 0;

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(200));
        lux = wrapper_bh1750_read();

        // Conversión a porcentaje (máximo 100%)
        if (lux >= 30000) {
            lux_percent = 99;
        } else {
            lux_percent = (lux * 99) / 30000;
        }

        // Enviar el porcentaje a la cola
        xQueueOverwrite(cola_luz_percent, &lux_percent);

        // Si querés, seguí enviando el valor original a la cola anterior
        xQueueOverwrite(cola_luz, &lux);
    }
}


/**
 * @brief Tarea que parpadea el LED de acuerdo a la intensidad lumínica
 */
void tarea_blinky(void *params) {
	// Variable para guardar el tiempo en ms de bloqueo
	uint16_t blocking_time;

	while(1) {
		// Lee el último valor de luminosidad
		xQueuePeek(cola_luz, &blocking_time, portMAX_DELAY);
		// Máximo es aprox 30000 entonces 3000 ms como máximo
		blocking_time /= 10;
		// Conmuto salida
		wrapper_output_toggle((gpio_t){LED});
		// Bloqueo el tiempo que se indique de la cola
		vTaskDelay(pdMS_TO_TICKS(blocking_time));
	}
}

/**
 * @brief Tarea que hace sonar el buzzer
 */
void tarea_buzzer(void *params) {

	while(1) {
		// Intenta tomar el semáforo
		xSemaphoreTake(semaforo_buzzer, portMAX_DELAY);
		// Conmuto el buzzer
		wrapper_output_toggle((gpio_t){BUZZER});
	}
}

/**
 * @brief Tarea que decrementa el contador
 */
void tarea_counter(void *params) {
	while(1) {
		// Decrementa la cuenta cada un segundo
		xSemaphoreTake(semaforo_cont, 0);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

/**
 * @brief Tarea que manualmente controla el contador
 */
void tarea_counter_btns(void *params) {

	while(1) {
		// Intenta tomar el semáforo
		xSemaphoreTake(semaforo_touch, portMAX_DELAY);
		// Toma el mutex para bloquear la otra tarea que escribe el display
		xSemaphoreTake(semaforo_mutex, portMAX_DELAY);
		// Verifica qué pulsador se presionó
		if(wrapper_btn_get_with_debouncing_with_pull_up((gpio_t){S1_BTN})) {
			// Decrementa la cuenta del semáforo
			xSemaphoreTake(semaforo_cont, 0);
		}
		else if(wrapper_btn_get_with_debouncing_with_pull_up((gpio_t){S2_BTN})) {
			// Incrementa la cuenta del semáforo
			xSemaphoreGive(semaforo_cont);
		}
		// Escribe en el display
		uint16_t data = uxSemaphoreGetCount(semaforo_cont);
		xQueueOverwrite(cola_display, &data);
		// Demora chica para evitar que detecte muy rápido que se presionó
		vTaskDelay(pdMS_TO_TICKS(30));
		// Devuelve el mutex
		xSemaphoreGive(semaforo_mutex);
	}
}

void tarea_cny70(void *params) {
	while(1) {
		// Espera a que el infrarrojo detecte movimiento
		xSemaphoreTake(semaforo_cny70, portMAX_DELAY);
		// Hace sonar el buzzer
		wrapper_output_toggle((gpio_t){BUZZER});
	}
}