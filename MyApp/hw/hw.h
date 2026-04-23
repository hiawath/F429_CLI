#ifndef __HW_DEF_H
#define __HW_DEF_H



#include "hw_def.h"
#include "led.h"
#include "stm32f4xx_hal_adc.h"
#include "uart.h"
#include "cli.h"
#include "my_gpio.h"
#include "button.h"
#include "temp.h"

extern UART_HandleTypeDef huart3;
extern ADC_HandleTypeDef hadc1;

void hwInit(void);


#endif