#include "stm32f4xx.h"

/* 定义LED和蜂鸣器的GPIO引脚 */
#define LED1_PIN    GPIO_PIN_5
#define LED2_PIN    GPIO_PIN_0
#define LED3_PIN    GPIO_PIN_1
#define LED_PORT    GPIOB

#define BEEP_PIN    GPIO_PIN_8
#define BEEP_PORT   GPIOA

int main(void)
{
    /* ========== HAL库初始化 ========== */
    HAL_Init();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    
    /* ========== 初始化三个LED（PB5、PB0、PB1）========== */
    /* 开启GPIOB的时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED1_PIN | LED2_PIN | LED3_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    
    /* 预设输出高电平，确保LED初始化后是熄灭的 */
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN, GPIO_PIN_SET);
    
    /* ========== 初始化蜂鸣器（PA8）========== */
    /* 开启GPIOA的时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = BEEP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BEEP_PORT, &GPIO_InitStruct);
    
    /* 预设输出高电平，蜂鸣器不响 */
    HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_SET);
    
    /* ========== 控制输出 ========== */
    /* 点亮三个LED */
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN, GPIO_PIN_RESET);
    
    /* 蜂鸣器鸣响 */
    HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET);
    
    /* ========== 扩展：不同LED和蜂鸣器的亮灭组合 ========== */
    // 组合1：仅LED1亮，蜂鸣器响
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED2_PIN | LED3_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET);
    
    // 组合2：仅LED2亮，蜂鸣器不响
    HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED3_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_SET);
    
    // 组合3：三个LED全灭，蜂鸣器响
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET);
    
    // 组合4：LED1和LED3亮，蜂鸣器响
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED3_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET);
    
    while(1)
    {
        /* 程序停留在此循环 */
    }
}