#include "stm32f4xx.h"

/* 定义LED引脚 */
#define LED1_PIN    GPIO_PIN_5
#define LED2_PIN    GPIO_PIN_0
#define LED3_PIN    GPIO_PIN_1
#define LED_PORT    GPIOB

#define BEEP_PIN    GPIO_PIN_8
#define BEEP_PORT   GPIOA

/* 函数声明 */
void delay_us(uint32_t nus);
void delay_ms(uint32_t nms);

int main(void)
{
    /* ========== HAL库初始化 ========== */
    HAL_Init();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    
    /* ========== 初始化三个LED ========== */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED1_PIN | LED2_PIN | LED3_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    
    /* 预设输出高电平，确保LED初始化后是熄灭的 */
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN, GPIO_PIN_SET);
    
    /* ========== 初始化蜂鸣器（用于扩展功能）========== */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = BEEP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BEEP_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_SET);
    
    /* ========== 主循环：向左流水灯 + LED1亮时蜂鸣器发声 ========== */
    while(1)
    {
        /* 第1步：LED1点亮，LED2、LED3熄灭，蜂鸣器响 */
        HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);          // 点亮LED1
        HAL_GPIO_WritePin(LED_PORT, LED2_PIN | LED3_PIN, GPIO_PIN_SET); // 熄灭LED2、LED3
        HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET);         // 蜂鸣器响
        delay_ms(500);
        
        /* 第2步：LED2点亮，LED1、LED3熄灭，蜂鸣器不响 */
        HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);          // 点亮LED2
        HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED3_PIN, GPIO_PIN_SET); // 熄灭LED1、LED3
        HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_SET);           // 蜂鸣器不响
        delay_ms(500);
        
        /* 第3步：LED3点亮，LED1、LED2熄灭，蜂鸣器不响 */
        HAL_GPIO_WritePin(LED_PORT, LED3_PIN, GPIO_PIN_RESET);          // 点亮LED3
        HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED2_PIN, GPIO_PIN_SET); // 熄灭LED1、LED2
        HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_SET);           // 蜂鸣器不响
        delay_ms(500);
    }
}

/* ========== 延时函数 ========== */
void delay_us(uint32_t nus)
{
    uint32_t i;
    while(nus--)
    {
        i = 31;
        while(i--);
    }
}

void delay_ms(uint32_t nms)
{
    uint32_t i;
    while(nms--)
    {
        i = 33800;
        while(i--);
    }
}