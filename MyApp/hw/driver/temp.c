#include "temp.h"



ADC_HandleTypeDef* hadc;
// DMA가 지속적으로 값을 갱신할 버퍼 (값 1개)
static volatile uint32_t adc_dma_buf[1] = {0};

bool tempInit(ADC_HandleTypeDef* adcHandle)
{
    hadc = adcHandle;
     // ADC와 DMA 초기화는 CubeMX에서 이미 설정되어 있다고 가정합니다.
     // 여기서는 온도 센서 관련 추가 설정이나 검증이 필요한 경우에만 코드를 작성합니다.
    // 초기에는 전력 소모를 막기 위해 DMA를 켜지 않습니다.
    return true;
}

void tempStartAuto(void)
{
    // DMA Continuous 모드 시작
    HAL_ADC_Start_DMA(hadc, (uint32_t*)adc_dma_buf, 1);
}

void tempStopAuto(void)
{
    // DMA 완전 정지 (ADC 전력 절약)
    HAL_ADC_Stop_DMA(hadc);
}

float tempReadAuto(void)
{
    uint32_t adc_val = adc_dma_buf[0];
    float vsense = ((float)adc_val / 4095.0f) * 3.3f;
    return ((vsense - 0.76f) / 0.0025f) + 25.0f;
}

float tempReadSingle(void)
{
    uint32_t adc_val = 0;
    
    // DMA 없이 Polling 방식으로 딱 1회 전력을 소모하여 읽음
    HAL_ADC_Start(hadc);
    if (HAL_ADC_PollForConversion(hadc, 10) == HAL_OK)
    {
        adc_val = HAL_ADC_GetValue(hadc);
    }
    HAL_ADC_Stop(hadc);
    
    float vsense = ((float)adc_val / 4095.0f) * 3.3f;
    return ((vsense - 0.76f) / 0.0025f) + 25.0f;
}
