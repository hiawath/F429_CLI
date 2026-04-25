#include "led.h"


static const led_info_t *p_led_tbl = NULL;
static uint8_t           led_max_ch = 0;


void ledInit(const led_info_t *p_tbl, uint8_t ch_max)
{
  p_led_tbl  = p_tbl;
  led_max_ch = ch_max;

  for (int i=0; i<led_max_ch; i++)
  {
    ledOff(i);
  }
}

void ledOn(uint8_t ch)
{
  if (ch >= led_max_ch || p_led_tbl == NULL) return;

  HAL_GPIO_WritePin(p_led_tbl[ch].port, p_led_tbl[ch].pin, p_led_tbl[ch].on_state);
}

void ledOff(uint8_t ch)
{
  if (ch >= led_max_ch || p_led_tbl == NULL) return;

  HAL_GPIO_WritePin(p_led_tbl[ch].port, p_led_tbl[ch].pin, p_led_tbl[ch].off_state);
}

void ledToggle(uint8_t ch)
{
  if (ch >= led_max_ch || p_led_tbl == NULL) return;

  HAL_GPIO_TogglePin(p_led_tbl[ch].port, p_led_tbl[ch].pin);
}

bool ledGetStatus(uint8_t ch)
{
  if (ch >= led_max_ch || p_led_tbl == NULL) return false;

  return (HAL_GPIO_ReadPin(p_led_tbl[ch].port, p_led_tbl[ch].pin) == p_led_tbl[ch].on_state) ? true : false;
}
