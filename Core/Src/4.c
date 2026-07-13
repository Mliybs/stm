#include "stm32f4xx.h"
#include <string.h>

/* 变量定义区 */
#define RX_BUF_SIZE  64

UART_HandleTypeDef huart2;           // 串口句柄

uint8_t rx_buf[RX_BUF_SIZE];         // 接收缓存
uint8_t rx_temp;                     // 中断接收单字节缓冲
uint16_t rx_index = 0;               // 接收计数器
uint8_t rx_frame_flag = 0;           // 收到完整帧标志
uint8_t od_flag = 0;                 // 收到回车符标志
uint8_t error_flag = 0;              // 错误指令标志

/* 函数声明区 */
void SystemClock_Config(void);
void GPIO_Init(void);
void USART2_Init(void);
void USART2_Send_Frame(uint8_t* data, uint16_t len);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void delay_ms(uint32_t ms);

/* 主函数 */
int main(void)
{
    /* 1. HAL库初始化 */
    HAL_Init();
    
    /* 2. 系统时钟配置 */
    SystemClock_Config();
    
    /* 3. GPIO初始化（LED和蜂鸣器） */
    GPIO_Init();
    
    /* 4. 串口2初始化 */
    USART2_Init();
    
    /* 5. 发送初始欢迎信息 */
    USART2_Send_Frame((uint8_t*)"Hello Everyone!\r\n", 17);
    
    /* 6. 启动串口中断接收（接收1个字节） */
    HAL_UART_Receive_IT(&huart2, &rx_temp, 1);
    
    /* 7. 主循环 */
    while (1)
    {
        if (rx_frame_flag) 
        {
            rx_frame_flag = 0;
            
            // 判断数据长度是否为4（BEEP、LED1、LED2、LED3都是4个字符）
            if (4 == rx_index) 
            {
                // 指令：BEEP - 控制蜂鸣器
                if (('B' == rx_buf[0]) && ('E' == rx_buf[1]) && 
                    ('E' == rx_buf[2]) && ('P' == rx_buf[3])) 
                {
                    // 蜂鸣器响（PA8低电平有效）
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
                    delay_ms(50);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
                    USART2_Send_Frame((uint8_t*)"蜂鸣器\r\n", 8);
                }
                // 指令：LEDx - 控制LED
                else if (('L' == rx_buf[0]) && ('E' == rx_buf[1]) && ('D' == rx_buf[2])) 
                {
                    if ('1' == rx_buf[3]) 
                    {
                        // 点亮LED1（PB5低电平有效）
                        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
                        USART2_Send_Frame((uint8_t*)"LED1\r\n", 6);
                    }
                    else if ('2' == rx_buf[3]) 
                    {
                        // 点亮LED2（PB0低电平有效）
                        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                        USART2_Send_Frame((uint8_t*)"LED2\r\n", 6);
                    }
                    else if ('3' == rx_buf[3]) 
                    {
                        // 点亮LED3（PB1低电平有效）
                        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
                        USART2_Send_Frame((uint8_t*)"LED3\r\n", 6);
                    } 
                    else 
                    {
                        error_flag = 1;  // LED编号错误
                    }
                    
                    // 延时200ms后熄灭所有LED
                    delay_ms(200);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5 | GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_SET);
                } 
                else 
                {
                    error_flag = 1;  // 指令不匹配
                }
            } 
            else 
            {
                error_flag = 1;  // 长度不正确
            }
            
            // 发送错误信息
            if (error_flag) 
            {
                error_flag = 0;
                USART2_Send_Frame((uint8_t*)"指令错误！\r\n", 12);
            }
            
            // 清空接收缓存（重置索引）
            rx_index = 0;
        }
    }
}

/* 系统时钟配置（使用外部晶振或内部HSI） */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* 启用PWR时钟 */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* 配置HSI作为PLL时钟源，系统时钟168MHz */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* 配置系统时钟、AHB和APB总线时钟 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/* GPIO初始化（LED和蜂鸣器） */
void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 1. 使能GPIO时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* 2. 配置LED1-3（PB5, PB0, PB1）为推挽输出 */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* 3. 配置蜂鸣器（PA8）为推挽输出 */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* 4. 初始状态：LED熄灭，蜂鸣器不响（高电平） */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5 | GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
}

/* 串口2初始化 */
void USART2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 1. 使能USART2和GPIOA时钟 */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* 2. 配置PA2(TX)和PA3(RX)为复用功能 */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;  // STM32F4中USART2复用功能为AF7
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* 3. 配置USART2参数 */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
    
    /* 4. 配置NVIC中断 */
    HAL_NVIC_SetPriority(USART2_IRQn, 1, 3);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/* 串口2发送函数 */
void USART2_Send_Frame(uint8_t* data, uint16_t len)
{
    HAL_UART_Transmit(&huart2, data, len, 0xFFFF);
}

/* 串口2中断服务函数（HAL库实际调用的是这个） */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

/* 串口接收中断回调函数 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) 
    {
        /* 已经收到过回车符（0x0D） */
        if (od_flag) 
        {
            /* 检测到换行符（0x0A），表示一帧数据结束 */
            if (rx_temp == 0x0A) 
            {
                rx_frame_flag = 1;        // 置位帧接收完成标志
                rx_buf[rx_index] = '\0';  // 添加字符串结束符
                od_flag = 0;              // 清除回车标志
            }
            /* 注意：如果回车后不是换行符，此处忽略该字符（丢弃） */
        }
        else  // 还未收到回车符
        {
            /* 收到回车符（0x0D） */
            if (rx_temp == 0x0D) 
            {
                od_flag = 1;  // 置位回车标志
            }
            else 
            {
                /* 正常数据存入缓存 */
                if (rx_index < RX_BUF_SIZE - 1) 
                {
                    rx_buf[rx_index++] = rx_temp;
                }
                /* 如果缓存溢出，丢弃后续数据 */
            }
        }
        
        /* 重新启动中断接收，准备接收下一个字节 */
        HAL_UART_Receive_IT(&huart2, &rx_temp, 1);
    }
}

/* 简单毫秒延时函数（使用SysTick） */
void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}