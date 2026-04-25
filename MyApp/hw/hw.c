#include "hw.h"


static const led_info_t led_tbl[] = 
{
  {GPIOB, GPIO_PIN_0,  GPIO_PIN_SET, GPIO_PIN_RESET}, // LD1
  {GPIOB, GPIO_PIN_7,  GPIO_PIN_SET, GPIO_PIN_RESET}, // LD2
  {GPIOB, GPIO_PIN_14, GPIO_PIN_SET, GPIO_PIN_RESET}, // LD3
};


static const uart_info_t uart_tbl[] = 
{
  {&huart3, 115200},
};

static const can_info_t can_tbl[] = 
{
  {&hcan1, 500000},
};


void hwInit(void){
    ledInit(led_tbl, sizeof(led_tbl)/sizeof(led_info_t));
    uartInit(uart_tbl, sizeof(uart_tbl)/sizeof(uart_info_t));
    canInit(can_tbl, sizeof(can_tbl)/sizeof(can_info_t));
    buttonInit();
    tempInit(&hadc1);
}
