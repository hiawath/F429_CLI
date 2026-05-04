#include "uart.h"


#define UART_MAX_CH     4


typedef struct
{
  UART_HandleTypeDef *huart;
  osMessageQueueId_t  rx_q;
  osMutexId_t         tx_mutex;
  osSemaphoreId_t     tx_sem; // 송신 완료 대기용 세마포어
  uint8_t             rx_byte;
  bool                is_open;
} uart_obj_t;


static const uart_info_t *p_uart_tbl = NULL;
static uint8_t            uart_max_ch = 0;
static uart_obj_t         uart_obj[UART_MAX_CH];


bool uartInit(const uart_info_t *p_tbl, uint8_t ch_max)
{
  p_uart_tbl  = p_tbl;
  uart_max_ch = ch_max;

  for (int i=0; i<UART_MAX_CH; i++)
  {
    uart_obj[i].is_open = false;
    uart_obj[i].rx_q = NULL;
    uart_obj[i].tx_mutex = NULL;
    uart_obj[i].tx_sem = NULL;
  }

  for (int i=0; i<uart_max_ch; i++)
  {
    if (i >= UART_MAX_CH) break;

    uart_obj[i].huart = p_uart_tbl[i].huart;
    
    if (uart_obj[i].rx_q == NULL) {
      uart_obj[i].rx_q = osMessageQueueNew(256, sizeof(uint8_t), NULL);
    }
    if (uart_obj[i].tx_mutex == NULL) {
      uart_obj[i].tx_mutex = osMutexNew(NULL);
    }
    if (uart_obj[i].tx_sem == NULL) {
      uart_obj[i].tx_sem = osSemaphoreNew(1, 0, NULL); // 최대 1, 초기값 0
    }

    uartOpen(i, p_uart_tbl[i].baud);
  }

  return true;
}

bool uartOpen(uint8_t ch, uint32_t baud)
{
  if (ch >= uart_max_ch || ch >= UART_MAX_CH) return false;

  UART_HandleTypeDef *huart = uart_obj[ch].huart;

  if (huart->Init.BaudRate != baud)
  {
    huart->Init.BaudRate = baud;
    if (HAL_UART_DeInit(huart) != HAL_OK) return false;
    if (HAL_UART_Init(huart) != HAL_OK) return false;
  }

  uart_obj[ch].is_open = true;
  HAL_UART_Receive_IT(huart, &uart_obj[ch].rx_byte, 1);

  return true;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
{
  if (ch >= uart_max_ch || ch >= UART_MAX_CH || !uart_obj[ch].is_open) return 0;

  uint32_t ret = 0;
  
  if (uart_obj[ch].tx_mutex != NULL) {
    osMutexAcquire(uart_obj[ch].tx_mutex, osWaitForever);
  }
  
  // Interrupt 방식 송신 요청
  if (HAL_UART_Transmit_IT(uart_obj[ch].huart, p_data, length) == HAL_OK) {
    // 송신 완료 인터럽트(Callback)가 세마포어를 풀어줄 때까지 대기 (CPU 점유율 0%)
    if (uart_obj[ch].tx_sem != NULL) {
      osSemaphoreAcquire(uart_obj[ch].tx_sem, osWaitForever);
    }
    ret = length;
  }
  
  if (uart_obj[ch].tx_mutex != NULL) {
    osMutexRelease(uart_obj[ch].tx_mutex);
  }

  return ret;
}

uint32_t uartPrintf(uint8_t ch, char *fmt, ...)
{
  char buf[256];
  va_list args;
  int len;

  va_start(args, fmt);
  len = vsnprintf(buf, 256, fmt, args);
  va_end(args);

  return uartWrite(ch, (uint8_t *)buf, len);
}

uint32_t uartAvailable(uint8_t ch)
{
  if (ch >= uart_max_ch || ch >= UART_MAX_CH || uart_obj[ch].rx_q == NULL) return 0;

  return osMessageQueueGetCount(uart_obj[ch].rx_q);
}

uint8_t uartRead(uint8_t ch)
{
  uint8_t ret = 0;
  if (ch >= uart_max_ch || ch >= UART_MAX_CH || uart_obj[ch].rx_q == NULL) return 0;

  osMessageQueueGet(uart_obj[ch].rx_q, &ret, NULL, 0);
  return ret;
}

bool uartReadBlock(uint8_t ch, uint8_t *p_data, uint32_t timeout)
{
  if (ch >= uart_max_ch || ch >= UART_MAX_CH || uart_obj[ch].rx_q == NULL) return false;

  if (osMessageQueueGet(uart_obj[ch].rx_q, p_data, NULL, timeout) == osOK) {
    return true;
  }
  return false;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  for (int i=0; i<uart_max_ch; i++)
  {
    if (i >= UART_MAX_CH) break;

    if (huart->Instance == uart_obj[i].huart->Instance)
    {
      if (uart_obj[i].tx_sem != NULL) {
        osSemaphoreRelease(uart_obj[i].tx_sem);
      }
      break;
    }
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  for (int i=0; i<uart_max_ch; i++)
  {
    if (i >= UART_MAX_CH) break;

    if (huart->Instance == uart_obj[i].huart->Instance)
    {
      if (uart_obj[i].rx_q != NULL) {
        osMessageQueuePut(uart_obj[i].rx_q, &uart_obj[i].rx_byte, 0, 0); 
      }
      HAL_UART_Receive_IT(uart_obj[i].huart, &uart_obj[i].rx_byte, 1);
      break;
    }
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  for (int i=0; i<uart_max_ch; i++)
  {
    if (i >= UART_MAX_CH) break;

    if (huart->Instance == uart_obj[i].huart->Instance)
    {
      HAL_UART_Receive_IT(uart_obj[i].huart, &uart_obj[i].rx_byte, 1);
      break;
    }
  }
}
