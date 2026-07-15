#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "nfc_pn532.h"
#include "delay.h"

/* 外部UART句柄（由CubeMX生成） */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

#define TEST_SIZE 16
uint8_t BufRead[TEST_SIZE]  = {0};
uint8_t BufWrite[TEST_SIZE] = {0};

/* 重定向printf到USART2（调试串口） */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 100);
    return ch;
}

void make_test_data(void)
{
    uint32_t i = 0;
    
    printf("Test Data:\r\n");
    for(i = 0; i < TEST_SIZE; i++)
    {
        BufWrite[i] = i;
        printf("0x%02X ", BufWrite[i]);
        if((i + 1) % 16 == 0)
        {
            printf("\r\n");
        }
    }
}

void check_test_data(void)
{
    uint32_t i = 0;
    
    printf("Read Data:\r\n");
    for(i = 0; i < TEST_SIZE; i++)
    {
        if(BufRead[i] != BufWrite[i])
        {
            printf("0x%02X ", BufRead[i]);
            printf("<<< NOT correct\r\n");
            break;
        }
        printf("0x%02X ", BufRead[i]);
        if((i + 1) % 16 == 0)
        {
            printf("\r\n");
        }
    }
    
    printf("\r\n");
    if(i >= TEST_SIZE)
    {
        printf("====== TEST OK ======\r\n");
    }
    else
    {
        printf("====== TEST FAIL ======\r\n");
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();  /* CubeMX生成的时钟配置 */
    
    /* 初始化UART（由CubeMX生成） */
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    
    /* 初始化NFC模块 */
    NFC_Init(115200);
    printf("NFC Init OK\r\n");

    /* 唤醒PN532 */
    NFC_WakeUp();
    
    /* 生成测试数据 */
    make_test_data();
    
    while(1)
    {
        /* 写入块2 */
        if(NFC_Write(2, BufWrite) < 0)
        {
            printf("Write failed, retry...\r\n");
            continue;
        }
        printf("Write OK\r\n");

        HAL_Delay(5000);

        /* 读取块2 */
        if(NFC_Read(2, BufRead) < 0)
        {
            printf("Read failed, retry...\r\n");
            continue;
        }
        printf("Read OK\r\n");

        /* 比对数据 */
        check_test_data();
        
        HAL_Delay(30000);
    }
}