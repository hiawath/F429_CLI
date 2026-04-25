#include "hw.h"


static const led_info_t led_tbl[] = 
{
  {GPIOB, GPIO_PIN_0,  GPIO_PIN_SET, GPIO_PIN_RESET}, // LD1
  {GPIOB, GPIO_PIN_7,  GPIO_PIN_SET, GPIO_PIN_RESET}, // LD2
  {GPIOB, GPIO_PIN_14, GPIO_PIN_SET, GPIO_PIN_RESET}, // LD3
};


void hwInit(void){
    ledInit(led_tbl, sizeof(led_tbl)/sizeof(led_info_t));
    uartInit(&huart3);
    buttonInit();
    tempInit(&hadc1);
}
