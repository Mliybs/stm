#include <stdio.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"

// HAL库必需的全局变量（用于HAL_Init等）
GPIO_InitTypeDef GPIO_InitStruct;

void SystemClock_Config(void);

int main(void){
    HAL_Init();                         // HAL库初始化
    SystemClock_Config();               // 配置系统时钟168MHz
    delay_init(168);                    // 延时初始化
    
    UART2_Init(115200);                 // 串口初始化
    DHT11_Init();                       // DHT11初始化
    
    printf("DHT11 Init OK!\r\n");
    delay_ms(1000);
    
    DHT11_Data data;
    while(1){
        if(DHT11_ReadData(&data) == 0){
            printf("HUMI:%d.%d_TEMP:%d.%d\r\n",
                   data.humi_int, data.humi_deci,
                   data.temp_int, data.temp_deci);
        } else {
            printf("Read DHT11 ERROR!\r\n");
        }
        delay_ms(10000);
    }
}

/*
 * 函数名：SystemClock_Config
 * 描述  ：配置系统时钟168MHz（HSE 8M -> PLL -> 168M）
 */
void SystemClock_Config(void){
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // 配置HSE + PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 8;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    // 配置系统时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;   // 42MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;   // 84MHz
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/*
 * 函数名：SysTick_Handler
 * 描述  ：HAL库SysTick中断处理（弱定义，需实现）
 */
void SysTick_Handler(void){
    HAL_IncTick();
}