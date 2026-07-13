#include "stm32f4xx.h"

/* 全局变量：LED流水灯延时时间，默认500ms */
uint16_t LedTime = 500;

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void delay_ms(uint32_t nms);

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void)
{
    /* 1. HAL库初始化 */
    HAL_Init();
    
    /* 2. 系统时钟配置（使用外部晶振，168MHz） */
    SystemClock_Config();
    
    /* 3. 初始化GPIO（LED + 按键外部中断） */
    MX_GPIO_Init();
    
    /* 4. LED流水灯主循环 */
    while (1)
    {
        /* LED1亮，LED2、LED3灭 */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);   /* 点亮LED1 */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_SET); /* 熄灭LED2、3 */
        delay_ms(LedTime);
        
        /* LED2亮，LED1、LED3灭 */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);   /* 点亮LED2 */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5 | GPIO_PIN_1, GPIO_PIN_SET); /* 熄灭LED1、3 */
        delay_ms(LedTime);
        
        /* LED3亮，LED1、LED2灭 */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);   /* 点亮LED3 */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5 | GPIO_PIN_0, GPIO_PIN_SET); /* 熄灭LED1、2 */
        delay_ms(LedTime);
    }
}

/**
  * @brief  GPIO初始化函数（LED + 外部中断按键）
  * @param  无
  * @retval 无
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* ==================== 1. 使能GPIO时钟 ==================== */
    __HAL_RCC_GPIOB_CLK_ENABLE();   /* LED使用GPIOB */
    __HAL_RCC_GPIOA_CLK_ENABLE();   /* 按键1使用GPIOA */
    __HAL_RCC_GPIOC_CLK_ENABLE();   /* 按键2使用GPIOC */
    __HAL_RCC_SYSCFG_CLK_ENABLE();  /* 外部中断需要SYSCFG时钟 */
    
    
    /* ==================== 2. 初始化LED (PB0, PB1, PB5) ==================== */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;    /* 推挽输出 */
    GPIO_InitStruct.Pull = GPIO_NOPULL;            /* 无上下拉 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;   /* 低速 */
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* 初始状态：所有LED熄灭（高电平熄灭） */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5, GPIO_PIN_SET);
    
    
    /* ==================== 3. 初始化按键1 (PA0) 外部中断 ==================== */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;   /* 外部中断，下降沿触发 */
    GPIO_InitStruct.Pull = GPIO_PULLUP;            /* 上拉输入（按键按下为低电平） */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* 设置中断优先级并使能 */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 2);         /* 抢占优先级0，响应优先级2 */
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    
    
    /* ==================== 4. 初始化按键2 (PC13) 外部中断 ==================== */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;   /* 外部中断，下降沿触发 */
    GPIO_InitStruct.Pull = GPIO_PULLUP;            /* 上拉输入（按键按下为低电平） */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    /* 设置中断优先级并使能 */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 2);     /* 抢占优先级0，响应优先级2 */
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
  * @brief  外部中断0服务函数（按键1: PA0）
  * @param  无
  * @retval 无
  */
void EXTI0_IRQHandler(void)
{
    /* 清除中断标志位 */
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

/**
  * @brief  GPIO外部中断回调函数
  * @param  GPIO_Pin: 触发中断的引脚
  * @retval 无
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* 按键1 (PA0) 按下，加速至100ms */
    if (GPIO_Pin == GPIO_PIN_0)
    {
        LedTime = 100;
    }
    /* 按键2 (PC13) 按下，减速至1000ms */
    else if (GPIO_Pin == GPIO_PIN_13)
    {
        LedTime = 1000;
    }
}

/**
  * @brief  系统时钟配置（使用外部晶振，主频168MHz）
  * @param  无
  * @retval 无
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* 配置HSE外部晶振作为PLL时钟源 */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;         /* HSE 8MHz / 8 = 1MHz */
    RCC_OscInitStruct.PLL.PLLN = 336;       /* 1MHz * 336 = 336MHz (VCO) */
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;  /* 336MHz / 2 = 168MHz (系统时钟) */
    RCC_OscInitStruct.PLL.PLLQ = 7;         /* 336MHz / 7 = 48MHz (USB等) */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* 配置系统时钟、AHB、APB分频 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   /* HCLK = 168MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;    /* PCLK1 = 42MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;    /* PCLK2 = 84MHz */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/**
  * @brief  简单毫秒级延时（HAL库已有HAL_Delay，此处保留原实验风格）
  * @param  nms: 延时毫秒数
  * @retval 无
  */
void delay_ms(uint32_t nms)
{
    HAL_Delay(nms);
}

/**
  * @brief  系统滴答定时器中断服务函数（HAL库需要）
  * @param  无
  * @retval 无
  */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/**
  * @brief  硬件错误中断服务函数
  * @param  无
  * @retval 无
  */
void HardFault_Handler(void)
{
    while (1) {}
}