#include "board.h" 
#include "fsl_dac.h"
#include "fsl_power.h"

#define LED_BLUE GPIO, 1, 1
#define LED_D1 GPIO, 0, 29 


int main (void) {

    // Inicializacion
	BOARD_BootClockFRO24M();

    gpio_pin_config_t out_config = { .pinDirection = kGPIO_DigitalOutput, .outputLogic = 1};

    // Habilito el puerto 1
    GPIO_PortInit(GPIO, 1);
    // Habilito el puerto 0
    GPIO_PortInit(GPIO, 0);

    // Configuro LED como salida
    GPIO_PinInit(LED_BLUE, &out_config);
    GPIO_PinInit(LED_D1, &out_config);

    // Configuro SysTick para 1 ms
    SysTick_Config(SystemCoreClock / 1000);

    while(1) {
    }
    return 0;
}

void SysTick_Handler(void) {
	// Variable para contar interrupciones
	static uint16_t i = 0;
    static uint16_t y = 0;

	// Incremento contador
	i++;
    y++;
	// Verifico si el SysTick se disparo 500 veces (medio segundo)
	if(i == 500) {
		// Reinicio el contador
		i = 0;
		// Conmuto el LED
		GPIO_PinWrite(LED_BLUE, !GPIO_PinRead(LED_BLUE));
	}

    if(y == 1500) {
		// Reinicio el contador
		y = 0;
		// Conmuto el LED
		GPIO_PinWrite(LED_D1, !GPIO_PinRead(LED_D1));
	}
}