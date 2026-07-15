#include "main.h"
#include "stm32f4xx.h"
#include "ir_recv.h"

// 全局定时器句柄
TIM_HandleTypeDef htim9;
UART_HandleTypeDef huart2;

// 系统时钟配置（使用HAL默认配置）
void SystemClock_Config(void);

// 串口初始化
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

// 定时器初始化
static void MX_TIM9_Init(void)
{
    __HAL_RCC_TIM9_CLK_ENABLE();

    htim9.Instance = TIM9;
    htim9.Init.Prescaler = 167;        // 168MHz/168 = 1MHz
    htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim9.Init.Period = 10000;         // 10ms
    htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim9);

    // 输入捕获配置在IR_Recv_Init中完成
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_USART2_UART_Init();
    MX_TIM9_Init();

    IR_Recv_Init();

    char msg[] = "IRRecv Init OK\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);

    while (1) {
        IR_Recv();
        HAL_Delay(1);
    }
}

// 系统时钟配置（168MHz）
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

// 硬件错误处理
void Error_Handler(void)
{
    while (1) {}
}