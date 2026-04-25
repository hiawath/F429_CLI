#include "mycan.h"


#define CAN_MAX_CH      2


typedef struct
{
  CAN_HandleTypeDef *hcan;
  osMessageQueueId_t  rx_q;
  bool                is_open;
  uint32_t            err_cnt;
  uint32_t            rx_cnt;
  uint32_t            tx_cnt;
} can_obj_t;


static const can_info_t *p_can_tbl = NULL;
static uint8_t           can_max_ch = 0;
static can_obj_t         can_obj[CAN_MAX_CH];


bool canInit(const can_info_t *p_tbl, uint8_t ch_max)
{
  p_can_tbl  = p_tbl;
  can_max_ch = ch_max;

  for (int i=0; i<CAN_MAX_CH; i++)
  {
    can_obj[i].is_open = false;
    can_obj[i].rx_q = NULL;
    can_obj[i].rx_cnt = 0;
    can_obj[i].tx_cnt = 0;
    can_obj[i].err_cnt = 0;
  }

  for (int i=0; i<can_max_ch; i++)
  {
    if (i >= CAN_MAX_CH) break;

    can_obj[i].hcan = p_can_tbl[i].hcan;
    
    if (can_obj[i].rx_q == NULL) {
      can_obj[i].rx_q = osMessageQueueNew(32, sizeof(can_msg_t), NULL);
    }
  }

  return true;
}

bool canOpen(uint8_t ch)
{
  if (ch >= can_max_ch || ch >= CAN_MAX_CH) return false;

  CAN_HandleTypeDef *hcan = can_obj[ch].hcan;
  CAN_FilterTypeDef  sFilterConfig;

  // 필터 설정: 모든 메시지 수신 (ID 0, Mask 0)
  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = 0x0000;
  sFilterConfig.FilterIdLow = 0x0000;
  sFilterConfig.FilterMaskIdHigh = 0x0000;
  sFilterConfig.FilterMaskIdLow = 0x0000;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.SlaveStartFilterBank = 14;

  if (HAL_CAN_ConfigFilter(hcan, &sFilterConfig) != HAL_OK)
  {
    return false;
  }

  if (HAL_CAN_Start(hcan) != HAL_OK)
  {
    return false;
  }

  if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING | 
                                         CAN_IT_ERROR | 
                                         CAN_IT_BUSOFF | 
                                         CAN_IT_LAST_ERROR_CODE) != HAL_OK)
  {
    return false;
  }

  can_obj[ch].is_open = true;
  return true;
}

bool canIsOpened(uint8_t ch)
{
  if (ch >= can_max_ch || ch >= CAN_MAX_CH) return false;
  return can_obj[ch].is_open;
}

uint32_t canAvailable(uint8_t ch)
{
  if (ch >= can_max_ch || ch >= CAN_MAX_CH || can_obj[ch].rx_q == NULL) return 0;

  return osMessageQueueGetCount(can_obj[ch].rx_q);
}

bool canReceive(uint8_t ch, can_msg_t *p_msg)
{
  if (ch >= can_max_ch || ch >= CAN_MAX_CH || can_obj[ch].rx_q == NULL) return false;

  if (osMessageQueueGet(can_obj[ch].rx_q, p_msg, NULL, 0) == osOK) {
    return true;
  }
  return false;
}

bool canSend(uint8_t ch, can_msg_t *p_msg)
{
  if (ch >= can_max_ch || ch >= CAN_MAX_CH || !can_obj[ch].is_open) return false;

  CAN_TxHeaderTypeDef tx_header;
  uint32_t tx_mailbox;

  tx_header.StdId = p_msg->id;
  tx_header.ExtId = p_msg->id;
  tx_header.RTR   = (p_msg->type == 1) ? CAN_RTR_REMOTE : CAN_RTR_DATA;
  tx_header.IDE   = (p_msg->format == 1) ? CAN_ID_EXT : CAN_ID_STD;
  tx_header.DLC   = p_msg->dlc;
  tx_header.TransmitGlobalTime = DISABLE;

  if (HAL_CAN_AddTxMessage(can_obj[ch].hcan, &tx_header, p_msg->data, &tx_mailbox) != HAL_OK)
  {
    can_obj[ch].err_cnt++;
    return false;
  }

  can_obj[ch].tx_cnt++;
  return true;
}

uint32_t canGetErrorCount(uint8_t ch)
{
  if (ch >= can_max_ch || ch >= CAN_MAX_CH) return 0;
  return can_obj[ch].err_cnt;
}

uint32_t canGetRxCount(uint8_t ch)
{
  if (ch >= can_max_ch || ch >= CAN_MAX_CH) return 0;
  return can_obj[ch].rx_cnt;
}

uint32_t canGetTxCount(uint8_t ch)
{
  if (ch >= can_max_ch || ch >= CAN_MAX_CH) return 0;
  return can_obj[ch].tx_cnt;
}


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  for (int i=0; i<can_max_ch; i++)
  {
    if (i >= CAN_MAX_CH) break;

    if (hcan->Instance == can_obj[i].hcan->Instance)
    {
      CAN_RxHeaderTypeDef rx_header;
      can_msg_t msg;

      if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, msg.data) == HAL_OK)
      {
        msg.id     = (rx_header.IDE == CAN_ID_STD) ? rx_header.StdId : rx_header.ExtId;
        msg.dlc    = rx_header.DLC;
        msg.format = (rx_header.IDE == CAN_ID_STD) ? 0 : 1;
        msg.type   = (rx_header.RTR == CAN_RTR_DATA) ? 0 : 1;

        if (can_obj[i].rx_q != NULL)
        {
          osMessageQueuePut(can_obj[i].rx_q, &msg, 0, 0);
        }
        can_obj[i].rx_cnt++;
      }
      break;
    }
  }
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
  for (int i=0; i<can_max_ch; i++)
  {
    if (i >= CAN_MAX_CH) break;

    if (hcan->Instance == can_obj[i].hcan->Instance)
    {
      can_obj[i].err_cnt++;
      break;
    }
  }
}
