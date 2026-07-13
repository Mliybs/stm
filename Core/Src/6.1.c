#include "stm32f4xx.h"

// 全局变量
TIM_HandleTypeDef htim3;
ADC_HandleTypeDef hadc1;
uint16_t Adc_Value = 0;
uint16_t PWM_Value = 0;

// 函数声明
void SystemClock_Config(void);
void GPIO_Init(void);
void PWM_Init(void);
void ADC_Init(void);
uint16_t Get_ADC(void);
uint16_t Get_ADC_Average(uint8_t times);
void delay_ms(uint32_t ms);

/**************************************************************
 * 主函数
 **************************************************************/
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    GPIO_Init();
    PWM_Init();
    ADC_Init();
    
    // 测试LED
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 499);
    HAL_Delay(500);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 300);
    HAL_Delay(500);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 100);
    HAL_Delay(500);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    
    while(1)
    {
        Adc_Value = Get_ADC_Average(10);
        PWM_Value = 455 - Adc_Value / 9;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, PWM_Value);
        HAL_Delay(100);
    }
}

/**************************************************************
 * 系统时钟配置（168MHz）
 **************************************************************/
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/**************************************************************
 * GPIO初始化
 **************************************************************/
void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // PC1 - ADC输入（模拟模式）
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // PB5 - PWM输出（复用推挽）
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**************************************************************
 * PWM初始化（TIM3_CH2, PB5）
 **************************************************************/
void PWM_Init(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();
    
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 84 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 500 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim3);
    
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);
    
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
}

/**************************************************************
 * ADC初始化（ADC1, Channel_11, PC1）
 **************************************************************/
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

/**************************************************************
 * 单次ADC转换
 **************************************************************/
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

/**************************************************************
 * 多次ADC转换取平均值
 **************************************************************/
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