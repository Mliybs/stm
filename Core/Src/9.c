#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "ir_recv.h"
#include "ir_send.h"
#include "delay.h"

#define DELAY_MS_COUNT  4000

// HAL库必需的滴答定时器弱定义
void SysTick_Handler(void){
    HAL_IncTick();
}

int main(void){
    uint32_t i = 0;
    uint32_t uMSCount = 0;
    
    HAL_Init();
    SystemClock_Config();  // 系统时钟配置，需根据实际板卡配置
    
    systick_init();
    UART2_Init(115200);
    IR_Recv_Init();
    printf("IR Recv Init OK\r\n");
    IR_Send_Init();
    printf("IR Send Init OK\r\n");
    
    while(1){
        IR_Recv();
        
        if(uMSCount >= 10000){
            printf("IRSend:addr:1,code:123\r\n");
            IR_Send(1, 123);
            uMSCount = 0;
        }
        
        if(i >= DELAY_MS_COUNT){
            uMSCount++;
            i = 0;
        }
        else{
            i++;
        }
    }
}

// 系统时钟配置（使用外部晶振，根据实际板卡调整）
void SystemClock_Config(void){
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
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}