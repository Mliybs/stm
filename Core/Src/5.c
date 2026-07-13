#include "stm32f4xx.h"

TIM_HandleTypeDef htim3;

/* 简单延时 */
void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/* 定时器初始化函数 */
void MX_TIM3_Init(void)
{
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 8400 - 1;          // 分频系数
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 5000 - 1;             // 自动重装载值
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim3);
}

/* 中断优先级配置 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
    if(htim_base->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM3_IRQn, 1, 3);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
    }
}

int main(void)
{
    HAL_Init();
    
    /* 1. 配置LED1 PB5 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);  // 初始熄灭

    /* 2. 初始化定时器TIM3并启动 */
    MX_TIM3_Init();
    HAL_TIM_Base_Start_IT(&htim3);

    while(1)
    {
        // 主循环为空
    }
}

/* TIM3中断服务函数 */
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}

/* 定时器溢出回调函数 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint8_t flag = 0;
    
    if(htim->Instance == TIM3)
    {
        if(flag)
        {
            flag = 0;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);  // 点亮
        }
        else
        {
            flag = 1;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);    // 熄灭
        }
    }
}