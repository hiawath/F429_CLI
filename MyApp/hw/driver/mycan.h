#ifndef SRC_HW_DRIVER_MYCAN_H_
#define SRC_HW_DRIVER_MYCAN_H_

#include "hw_def.h"


typedef struct
{
  uint32_t id;
  uint8_t  dlc;
  uint8_t  data[8];
  uint8_t  format; // 0: Standard, 1: Extended
  uint8_t  type;   // 0: Data, 1: Remote
} can_msg_t;

typedef struct
{
  CAN_HandleTypeDef *hcan;
  uint32_t           baud;
} can_info_t;


bool canInit(const can_info_t *p_tbl, uint8_t ch_max);
bool canOpen(uint8_t ch);
bool canIsOpened(uint8_t ch);

uint32_t canAvailable(uint8_t ch);
bool     canReceive(uint8_t ch, can_msg_t *p_msg);
bool     canSend(uint8_t ch, can_msg_t *p_msg);

uint32_t canGetErrorCount(uint8_t ch);
uint32_t canGetRxCount(uint8_t ch);
uint32_t canGetTxCount(uint8_t ch);

#endif
