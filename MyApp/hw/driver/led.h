#ifndef SRC_HW_DRIVER_LED_H_
#define SRC_HW_DRIVER_LED_H_

#include "hw_def.h"


typedef struct
{
  GPIO_TypeDef *port;
  uint16_t      pin;
  GPIO_PinState on_state;
  GPIO_PinState off_state;
} led_info_t;


void ledInit(const led_info_t *p_tbl, uint8_t ch_max);
void ledOn(uint8_t ch);
void ledOff(uint8_t ch);
void ledToggle(uint8_t ch);
bool ledGetStatus(uint8_t ch);

#endif
