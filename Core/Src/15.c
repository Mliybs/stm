#include <stdio.h>
#include "stm32f4xx.h"
#include "xpt2046.h"
#include "usart.h"

extern UART_HandleTypeDef huart2;

/**
  * @brief  主函数
  */
int main(void)
{
    HAL_Init();                     // 初始化
    SystemClock_Config();           // 系统时钟配置
    
    UART2_Init(115200);             // 串口初始化
    XPT2046_Init();                 // 触摸屏初始化
    
    printf("XPT2046 HAL Test\r\n");
    
    while (1) {
        XPT2046_DetectTouch();
        HAL_Delay(50);              // 50ms轮询
    }
}