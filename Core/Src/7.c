#include "main.h"
#include "stm32f4xx.h"

/* 定义一帧数据，准备发送，\r\n代表回车换行，共17个字节 */
uint8_t SendBuf[] = "Hello Everyone!\r\n";

/* 句柄声明 */
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_DMA_Init(void);
void DMA_Send_Data(void);

int main(void)
{
    /* 1. HAL库初始化 */
    HAL_Init();
    
    /* 2. 配置系统时钟 */
    SystemClock_Config();
    
    /* 3. 初始化GPIO */
    MX_GPIO_Init();
    
    /* 4. 初始化DMA */
    MX_DMA_Init();
    
    /* 5. 初始化串口2（需在DMA之后初始化） */
    MX_USART2_UART_Init();
    
    /* 6. 启动DMA发送数据 */
    DMA_Send_Data();
    
    /* 7. 主循环 - CPU完全空闲 */
    while(1)
    {
        /* CPU可以处理其他任务，DMA在后台自动发送数据 */
        HAL_Delay(1000);  // 演示CPU可执行其他任务
    }
}

/*==========================================================================
 * 函数名称： SystemClock_Config
 * 功能描述： 配置系统时钟为168MHz
 *==========================================================================*/
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* 配置HSE晶振作为PLL时钟源 */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;      // 8分频
    RCC_OscInitStruct.PLL.PLLN = 336;    // 336倍频
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;  // 2分频 -> 168MHz
    RCC_OscInitStruct.PLL.PLLQ = 7;      // USB等外设使用
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* 配置系统时钟、AHB、APB分频 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // HCLK = 168MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;    // APB1 = 42MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;    // APB2 = 84MHz
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/*==========================================================================
 * 函数名称： MX_GPIO_Init
 * 功能描述： GPIO初始化（串口2引脚）
 *==========================================================================*/
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能GPIOA时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA2 - USART2_TX, PA3 - USART2_RX 复用功能配置 */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;           // 复用推挽
    GPIO_InitStruct.Pull = GPIO_PULLUP;               // 上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_50MHz;    // 速度50MHz
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;      // 复用为USART2
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/*==========================================================================
 * 函数名称： MX_DMA_Init
 * 功能描述： DMA初始化配置
 *==========================================================================*/
static void MX_DMA_Init(void)
{
    /* 1. 使能DMA1时钟 */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* 2. 配置DMA1_Stream6用于USART2_TX */
    hdma_usart2_tx.Instance = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;              // 通道4
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;     // 存储器→外设
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;         // 外设地址不递增
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;             // 存储器地址递增
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;   // 外设数据宽度：8位
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;      // 存储器数据宽度：8位
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;                    // 普通模式
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_MEDIUM;       // 中等优先级
    hdma_usart2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;      // 不使用FIFO
    hdma_usart2_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_usart2_tx.Init.MemBurst = DMA_MBURST_SINGLE;         // 存储器单次突发
    hdma_usart2_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;      // 外设单次突发
    
    /* 3. 初始化DMA */
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
        Error_Handler();
    }

    /* 4. 将DMA句柄与串口句柄关联（用于HAL库自动管理） */
    __HAL_LINKDMA(&huart2, hdmatx, hdma_usart2_tx);
}

/*==========================================================================
 * 函数名称： MX_USART2_UART_Init
 * 功能描述： 串口2初始化（使能DMA发送）
 *==========================================================================*/
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
    
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/*==========================================================================
 * 函数名称： DMA_Send_Data
 * 功能描述： 启动DMA发送数据到串口2
 *==========================================================================*/
void DMA_Send_Data(void)
{
    /* 使用HAL库的DMA发送函数，启动后立即返回，由DMA在后台完成传输 */
    HAL_UART_Transmit_DMA(&huart2, SendBuf, sizeof(SendBuf));
}

/*==========================================================================
 * 函数名称： HAL_UART_TxCpltCallback
 * 功能描述： 串口DMA发送完成回调函数（可选）
 * 输入参数： huart - 串口号
 *==========================================================================*/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        /* 发送完成后的处理：例如翻转LED、打印状态等 */
        // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        
        /* 如需循环发送，可在此处重新启动 */
        // HAL_UART_Transmit_DMA(&huart2, SendBuf, sizeof(SendBuf));
    }
}

/*==========================================================================
 * 函数名称： HAL_UART_ErrorCallback
 * 功能描述： 串口错误回调函数（可选）
 *==========================================================================*/
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        /* 错误处理代码 */
        // Error_Handler();
    }
}

/*==========================================================================
 * 函数名称： Error_Handler
 * 功能描述： 错误处理函数
 *==========================================================================*/
void Error_Handler(void)
{
    /* 用户自定义错误处理，如点亮LED指示 */
    while(1)
    {
        // 可在此添加错误指示代码
    }
}