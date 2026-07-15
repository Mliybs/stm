#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "lcd_ili9341.h"

/* 外部函数声明 */
void UART2_Init(uint32_t bound);

/**
 * @brief  系统时钟配置（使用外部晶振）
 */
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

/**
 * @brief  主函数
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    UART2_Init(115200);
    printf("LCD TEST (HAL Library)\r\n");
    
    LCD_Init();
    
    LCD_SetFontEN(&ASCII_8x16);
    LCD_SetColors(RED, BLACK);
    LCD_Clear(0, 0, LCD_GetLenX(), LCD_GetLenY());
    
    LCD_DispStringEN(0, LINE_EN(0), 0, "3.2 inch LCD:");
    LCD_DispStringEN(0, LINE_EN(1), 0, "Image resolution:240x320 px");
    LCD_DispStringEN(0, LINE_EN(2), 0, "HAL Library Version");
    
    /* 图形测试 */
    LCD_SetColors(YELLOW, BLACK);
    LCD_DrawLine(10, 180, 230, 180);
    LCD_DrawRectangle(10, 200, 100, 40, 0);
    LCD_SetColors(GREEN, BLACK);
    LCD_DrawCircle(180, 220, 30, 0);
    
    printf("LCD Test Completed!\r\n");
    
    while (1) {
        HAL_Delay(500);
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);  /* 背光闪烁指示运行 */
    }
}