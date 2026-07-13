#include "stm32f4xx.h"

ADC_HandleTypeDef hadc1;
uint16_t Adc_Value = 0;
uint16_t Delay_Time = 0;

void SystemClock_Config(void);
void GPIO_Init(void);
void ADC_Init(void);
uint16_t Get_ADC(void);
uint16_t Get_ADC_Average(uint8_t times);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    ADC_Init();
    
    while(1)
    {
        Adc_Value = Get_ADC_Average(10);
        
        // ADC值0~4095映射到延时100ms~2000ms
        Delay_Time = 100 + (uint16_t)((uint32_t)Adc_Value * 1900 / 4095);
        
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);  // LED亮
        HAL_Delay(Delay_Time);
        
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);    // LED灭
        HAL_Delay(Delay_Time);
    }
}

void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // PC1 - ADC输入
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // PB5 - LED输出
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

void ADC_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();
    
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV6;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);
}

uint16_t Get_ADC(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_11;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    return HAL_ADC_GetValue(&hadc1);
}

uint16_t Get_ADC_Average(uint8_t times)
{
    uint32_t sum = 0;
    for(uint8_t i = 0; i < times; i++)
    {
        sum += Get_ADC();
        HAL_Delay(5);
    }
    return sum / times;
}

void SystemClock_Config(void)  // 同上
{
    // 与实验6-1相同
}