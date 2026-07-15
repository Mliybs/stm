#include "main.h"
#include "e2prom_at24c02_hal.h"
#include "usart.h"

#define TEST_SIZE 256
uint8_t BufWrite[TEST_SIZE];
uint8_t BufRead[TEST_SIZE];

void make_test_data(void)
{
    for (int i = 0; i < TEST_SIZE; i++) {
        BufWrite[i] = i;
        printf("0x%02X ", BufWrite[i]);
        if ((i+1) % 16 == 0) printf("\r\n");
    }
}

int check_test_data(void)
{
    for (int i = 0; i < TEST_SIZE; i++) {
        if (BufRead[i] != BufWrite[i]) {
            printf("Error at 0x%02X: read 0x%02X\r\n", i, BufRead[i]);
            return -1;
        }
    }
    return 0;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config(); // CubeMX生成的时钟配置
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init(); // 调试串口

    printf("AT24C02 HAL Test Start\r\n");

    // 1. 生成测试数据
    printf("Generating test data...\r\n");
    make_test_data();

    // 2. 写入E2PROM
    printf("Writing data...\r\n");
    AT24C02_BufferWrite(BufWrite, 0, TEST_SIZE);
    printf("Write OK\r\n");

    HAL_Delay(10);

    // 3. 读取E2PROM
    printf("Reading data...\r\n");
    AT24C02_BufferRead(BufRead, 0, TEST_SIZE);

    // 4. 校验数据
    if (check_test_data() == 0) {
        printf("TEST PASSED!\r\n");
    } else {
        printf("TEST FAILED!\r\n");
    }

    while (1) {}
}