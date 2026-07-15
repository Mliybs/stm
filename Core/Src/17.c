#include "stm32f4xx_hal.h"
#include "usart.h"
#include "fpr_zn632.h"
#include "delay.h"

void SystemClock_Config(void);
void FPR_Usage(void);

int main(void)
{
    char ch = 0;

    HAL_Init();
    SystemClock_Config();
    delay_init();

    UART2_Init(115200);
    printf("\r\n========== FPR Test Start ==========\r\n");

    FPR_Init(57600);
    printf("FPR Init Done\r\n");

    if (ZN632_VryPwd() == 0) {
        printf("Password Verify OK\r\n");
    }

    if (ZN632_ReadIndexTable() == 0) {
        printf("Read Index Table OK\r\n");
        printf("Used: ");
        for (int i = 0; i < ZN632_INDEX_MAX; i++) {
            if (ZN632_INDEX[i / 8] & (0x01 << (i % 8))) {
                printf("%d ", i);
            }
        }
        printf("\r\n");
    }

    while (1) {
        FPR_Usage();
        ch = getchar();
        getchar();  // 消耗回车

        switch (ch) {
            case '1':
                FPR_AddFinger();
                break;
            case '2':
                FPR_MatchFinger();
                break;
            case '3': {
                uint16_t id;
                printf("Input ID to delete (0-239): ");
                scanf("%d", &id);
                getchar();
                FPR_DeleteFinger(id);
                break;
            }
            case '4':
                if (ZN632_Empty() == 0) {
                    printf("All fingerprints cleared!\r\n");
                }
                break;
            default:
                printf("Invalid option!\r\n");
                break;
        }
    }
}

void FPR_Usage(void)
{
    printf("\r\n========== Menu ==========\r\n");
    printf("1 - Add Finger\r\n");
    printf("2 - Match Finger\r\n");
    printf("3 - Delete Finger\r\n");
    printf("4 - Clear All\r\n");
    printf("===========================\r\n");
    printf("Select: ");
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}