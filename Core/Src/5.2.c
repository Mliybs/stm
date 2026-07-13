#include "stm32f4xx.h"

TIM_HandleTypeDef htim3;
uint16_t PWM_Value = 0;

/* 简单延时 */
void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/* TIM3 PWM初始化 */
void MX_TIM3_Init(void)
{
    // 定时器基础配置
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 84 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 500 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim3);
    
    // PWM通道配置（通道2）
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;  // 初始占空比0
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);
    
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
}

/* GPIO初始化 */
void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // KEY1 PA0 输入
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // KEY2 PC13 输入
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // PB5 PWM输出
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // PB5复用为TIM3
    __HAL_AFIO_REMAP_TIM3_PARTIAL();  // 或使用 GPIO_AF_TIM3
}

int main(void)
{
    HAL_Init();
    MX_GPIO_Init();
    MX_TIM3_Init();
    
    // 测试不同占空比（呼吸效果）
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 499);
    HAL_Delay(500);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 300);
    HAL_Delay(500);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 100);
    HAL_Delay(500);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    HAL_Delay(500);
    
    while(1)
    {
        // KEY1按下 增加亮度
        if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
        {
            if(PWM_Value < 499)
            {
                PWM_Value++;
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, PWM_Value);
            }
        }
        // KEY2按下 降低亮度
        else if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
        {
            if(PWM_Value > 0)
            {
                PWM_Value--;
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, PWM_Value);
            }
        }
        HAL_Delay(20);
    }
}