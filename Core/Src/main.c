#include "stm32f4xx.h"

/* 定义LED引脚 */
#define LED1_PIN    GPIO_PIN_5
#define LED2_PIN    GPIO_PIN_0
#define LED3_PIN    GPIO_PIN_1
#define LED_PORT    GPIOB

/* 定义按键引脚 */
#define KEY1_PIN    GPIO_PIN_0
#define KEY1_PORT   GPIOA
#define KEY2_PIN    GPIO_PIN_13
#define KEY2_PORT   GPIOC
#define KEY3_PIN    GPIO_PIN_6
#define KEY3_PORT   GPIOC
#define KEY4_PIN    GPIO_PIN_7
#define KEY4_PORT   GPIOC

/* 按键状态变量 */
uint8_t KeyValue_1, KeyValue_2, KeyValue_3, KeyValue_4;

/* 函数声明 */
void delay_us(uint32_t nus);
void delay_ms(uint32_t nms);

int main(void)
{
    /* ========== HAL库初始化 ========== */
    HAL_Init();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    
    /* ========== 配置四个按键KEY1-KEY4所在的GPIO ========== */
    /* 开启GPIOA和GPIOC的时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 配置KEY1（PA0）为输入上拉 */
    GPIO_InitStruct.Pin = KEY1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY1_PORT, &GPIO_InitStruct);
    
    /* 配置KEY2（PC13）、KEY3（PC6）、KEY4（PC7）为输入上拉 */
    GPIO_InitStruct.Pin = KEY2_PIN | KEY3_PIN | KEY4_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    /* ========== 配置初始化三个LED ========== */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = LED1_PIN | LED2_PIN | LED3_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN, GPIO_PIN_SET);
    
    /* ========== 主循环：带按键抬起检测 ========== */
    while(1)
    {
        /* 读取四个按键的GPIO值 */
        KeyValue_1 = HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN);
        KeyValue_2 = HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN);
        KeyValue_3 = HAL_GPIO_ReadPin(KEY3_PORT, KEY3_PIN);
        KeyValue_4 = HAL_GPIO_ReadPin(KEY4_PORT, KEY4_PIN);
        
        /* 判断有无按键按下（按键按下为低电平） */
        if((GPIO_PIN_RESET == KeyValue_1) || (GPIO_PIN_RESET == KeyValue_2) || 
           (GPIO_PIN_RESET == KeyValue_3) || (GPIO_PIN_RESET == KeyValue_4))
        {
            delay_ms(20);  // 消抖动
            
            /* 再次读取确认 */
            KeyValue_1 = HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN);
            KeyValue_2 = HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN);
            KeyValue_3 = HAL_GPIO_ReadPin(KEY3_PORT, KEY3_PIN);
            KeyValue_4 = HAL_GPIO_ReadPin(KEY4_PORT, KEY4_PIN);
            
            /* ===== 按键抬起检测 ===== */
            /* 处理KEY1：按下后等待抬起，再点亮LED1 */
            if(GPIO_PIN_RESET == KeyValue_1)
            {
                /* 等待按键抬起 */
                while(GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN))
                {
                    delay_ms(10);
                }
                /* 按键已抬起，点亮LED1 */
                HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
            }
            /* 处理KEY2：按下后等待抬起，再点亮LED2 */
            else if(GPIO_PIN_RESET == KeyValue_2)
            {
                while(GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN))
                {
                    delay_ms(10);
                }
                HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
            }
            /* 处理KEY3：按下后等待抬起，再点亮LED3 */
            else if(GPIO_PIN_RESET == KeyValue_3)
            {
                while(GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY3_PORT, KEY3_PIN))
                {
                    delay_ms(10);
                }
                HAL_GPIO_WritePin(LED_PORT, LED3_PIN, GPIO_PIN_RESET);
            }
            /* 处理KEY4：按下后等待抬起，再熄灭三个LED */
            else if(GPIO_PIN_RESET == KeyValue_4)
            {
                while(GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY4_PORT, KEY4_PIN))
                {
                    delay_ms(10);
                }
                HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN, GPIO_PIN_SET);
            }
        }
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