#include <stddef.h>
#include "lcd_ili9341.h"
#include "lcd_fonts.h"
#include "main.h"

/***************************************************************************************
 * FSMC地址定义
 * FSMC_Bank1_NORSRAM1 地址范围: 0x60000000 ~ 0x63FFFFFF
 * FSMC_A16 接LCD的DC(寄存器/数据选择)脚
 * 寄存器基地址 = 0x60000000
 * RAM基地址 = 0x60020000 (A16=1)
 ****************************************************************************************/

/* FSMC Bank1 NORSRAM用于LCD命令操作的地址 (A16=0) */
#define FSMC_Addr_ILI9341_CMD   ((uint32_t)0x60000000)

/* FSMC Bank1 NORSRAM用于LCD数据操作的地址 (A16=1) */
#define FSMC_Addr_ILI9341_DATA  ((uint32_t)0x60020000)

/* 复位引脚 - PB0 (根据实际原理图修改) */
#define ILI9341_RST_PORT    GPIOB
#define ILI9341_RST_PIN     GPIO_PIN_0

/* 背光引脚 - PB1 (根据实际原理图修改) */
#define ILI9341_BK_PORT     GPIOB
#define ILI9341_BK_PIN      GPIO_PIN_1

#define ILI9341_LESS_PIXEL  240    /* 液晶屏较短方向的像素宽度 */
#define ILI9341_MORE_PIXEL  320    /* 液晶屏较长方向的像素宽度 */

/* ILI9341命令定义 */
#define CMD_RESET           0x01
#define CMD_SLEEP_OUT       0x11
#define CMD_GAMMA           0x26
#define CMD_DISPLAY_OFF     0x28
#define CMD_DISPLAY_ON      0x29
#define CMD_COLUMN_ADDR     0x2A
#define CMD_PAGE_ADDR       0x2B
#define CMD_GRAM            0x2C
#define CMD_TEARING_OFF     0x34
#define CMD_TEARING_ON      0x35
#define CMD_DISPLAY_INVERSION 0xB4
#define CMD_MAC             0x36    /* 内存访问控制 */
#define CMD_PIXEL_FORMAT    0x3A
#define CMD_WDB             0x51
#define CMD_WCD             0x53
#define CMD_RGB_INTERFACE   0xB0
#define CMD_FRC             0xB1    /* Frame Rate Control 帧率控制 */
#define CMD_BPC             0xB5
#define CMD_DFC             0xB6
#define CMD_ETMOD           0xB7
#define CMD_POWER1          0xC0    /* 功耗控制1 */
#define CMD_POWER2          0xC1    /* 功耗控制2 */
#define CMD_VCOM1           0xC5
#define CMD_VCOM2           0xC7
#define CMD_POWERA          0xCB
#define CMD_POWERB          0xCF
#define CMD_PGAMMA          0xE0
#define CMD_NGAMMA          0xE1
#define CMD_DTCA            0xE8
#define CMD_DTCB            0xEA
#define CMD_POWER_SEQ       0xED
#define CMD_3GAMMA_EN       0xF2
#define CMD_INTERFACE       0xF6
#define CMD_PRC             0xF7
#define CMD_VERTICAL_SCROLL 0x33
#define CMD_SetCoordinateX  0x2A     /* 设置X坐标 */
#define CMD_SetCoordinateY  0x2B     /* 设置Y坐标 */
#define CMD_SetPixel        0x2C     /* 填充像素 */

/* 延时宏定义 */
#define ILI9341_DELAY_MS(x) HAL_Delay(x)

/* 写命令/数据宏定义 */
#define ILI9341_WriteCmd(usCmd)   do { \
    *(__IO uint16_t*)(FSMC_Addr_ILI9341_CMD) = usCmd; \
} while(0)

#define ILI9341_WriteData(usData) do { \
    *(__IO uint16_t*)(FSMC_Addr_ILI9341_DATA) = usData; \
} while(0)

#define ILI9341_ReadData()        (*(__IO uint16_t*)(FSMC_Addr_ILI9341_DATA))

/* 全局变量 */
uint8_t g_LCD_ScanMode = 2;
static uint16_t g_LCD_TextColor = BLACK;   /* 前景色 */
static uint16_t g_LCD_BackColor = WHITE;   /* 背景色 */
uint16_t g_LCD_LenX = ILI9341_LESS_PIXEL;
uint16_t g_LCD_LenY = ILI9341_MORE_PIXEL;

/* 外部变量引用 */
extern SRAM_HandleTypeDef hsram1;

/* 静态函数声明 */
static void ILI9341_GPIO_Config(void);
static void ILI9341_Rst(void);
static void ILI9341_REG_Config(void);
static void ILI9341_FillColor(uint32_t ulAmout_Point, uint16_t usColor);
static void ILI9341_BackLight(uint8_t uOnOff);
static void ILI9341_GramScan(uint8_t ucOption);
static void ILI9341_OpenWindow(uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight);

/**
 * @brief  初始化ILI9341的IO引脚
 * @param  无
 * @retval 无
 */
static void ILI9341_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 使能GPIO时钟 */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();  /* 背光和复位引脚在GPIOB上 */
    
    /* GPIOD配置 - FSMC数据线和控制线 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 |
                          GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                          GPIO_PIN_11 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
    /* GPIOE配置 - FSMC数据线 */
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                          GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |
                          GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    
    /* 背光引脚 - 通用输出 */
    GPIO_InitStruct.Pin = ILI9341_BK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ILI9341_BK_PORT, &GPIO_InitStruct);
    
    /* 复位引脚 - 通用输出 */
    GPIO_InitStruct.Pin = ILI9341_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ILI9341_RST_PORT, &GPIO_InitStruct);
}

/**
 * @brief  ILI9341背光LED控制
 * @param  uOnOff: ENABLE使能背光, DISABLE禁用背光
 * @retval 无
 */
static void ILI9341_BackLight(uint8_t uOnOff)
{
    if (uOnOff == ENABLE) {
        HAL_GPIO_WritePin(ILI9341_BK_PORT, ILI9341_BK_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(ILI9341_BK_PORT, ILI9341_BK_PIN, GPIO_PIN_SET);
    }
}

/**
 * @brief  ILI9341 软件复位
 * @param  无
 * @retval 无
 */
static void ILI9341_Rst(void)
{
    HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_RESET);
    ILI9341_DELAY_MS(5);
    HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_SET);
    ILI9341_DELAY_MS(5);
}

/**
 * @brief  初始化ILI9341寄存器
 * @param  无
 * @retval 无
 */
static void ILI9341_REG_Config(void)
{
    ILI9341_WriteCmd(CMD_RESET);
    ILI9341_DELAY_MS(120);
    
    ILI9341_WriteCmd(CMD_DISPLAY_OFF);
    
    ILI9341_WriteCmd(CMD_PIXEL_FORMAT);
    ILI9341_WriteData(0x55);  /* 16bit/pixel */
    
    ILI9341_WriteCmd(CMD_SLEEP_OUT);
    ILI9341_DELAY_MS(100);
    ILI9341_WriteCmd(CMD_DISPLAY_ON);
    ILI9341_DELAY_MS(100);
    
    ILI9341_WriteCmd(CMD_GRAM);
    ILI9341_DELAY_MS(5);
}

/**
 * @brief  在ILI9341显示器上开辟一个窗口
 * @param  usX: 窗口的起点X坐标
 * @param  usY: 窗口的起点Y坐标
 * @param  usWidth: 窗口的宽度
 * @param  usHeight: 窗口的高度
 * @retval 无
 */
static void ILI9341_OpenWindow(uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight)
{
    ILI9341_WriteCmd(CMD_SetCoordinateX);
    ILI9341_WriteData(usX >> 8);
    ILI9341_WriteData(usX & 0xFF);
    ILI9341_WriteData((usX + usWidth - 1) >> 8);
    ILI9341_WriteData((usX + usWidth - 1) & 0xFF);
    
    ILI9341_WriteCmd(CMD_SetCoordinateY);
    ILI9341_WriteData(usY >> 8);
    ILI9341_WriteData(usY & 0xFF);
    ILI9341_WriteData((usY + usHeight - 1) >> 8);
    ILI9341_WriteData((usY + usHeight - 1) & 0xFF);
}

/**
 * @brief  设置ILI9341的GRAM的扫描方向
 * @param  ucOption: 选择GRAM的扫描方向 (0-7)
 * @retval 无
 */
static void ILI9341_GramScan(uint8_t ucOption)
{
    if (ucOption > 7) {
        return;
    }
    
    g_LCD_ScanMode = ucOption;
    
    if (ucOption % 2 == 0) {
        g_LCD_LenX = ILI9341_LESS_PIXEL;
        g_LCD_LenY = ILI9341_MORE_PIXEL;
    } else {
        g_LCD_LenX = ILI9341_MORE_PIXEL;
        g_LCD_LenY = ILI9341_LESS_PIXEL;
    }
    
    ILI9341_WriteCmd(CMD_MAC);
    ILI9341_WriteData(0x08 | (ucOption << 5));
    
    ILI9341_OpenWindow(0, 0, g_LCD_LenX, g_LCD_LenY);
    ILI9341_WriteCmd(CMD_SetPixel);
}

/**
 * @brief  在ILI9341显示器上以某一颜色填充像素点
 * @param  ulAmout_Point: 要填充颜色的像素点的总数目
 * @param  usColor: 颜色
 * @retval 无
 */
static void ILI9341_FillColor(uint32_t ulAmout_Point, uint16_t usColor)
{
    uint32_t i;
    ILI9341_WriteCmd(CMD_SetPixel);
    
    for (i = 0; i < ulAmout_Point; i++) {
        ILI9341_WriteData(usColor);
    }
}

/**
 * @brief  LCD初始化函数
 * @param  无
 * @retval 无
 */
void LCD_Init(void)
{
    ILI9341_GPIO_Config();
    
    /* 注意: FSMC已经在STM32CubeMX中配置好了, 不需要再初始化 */
    /* 如果需要在代码中配置, 可以调用 HAL_SRAM_Init(&hsram1) */
    
    ILI9341_Rst();
    ILI9341_BackLight(ENABLE);
    ILI9341_REG_Config();
    ILI9341_GramScan(g_LCD_ScanMode);
}

/**
 * @brief  获取LCD的X方向长度
 * @retval X方向像素数
 */
uint16_t LCD_GetLenX(void)
{
    return g_LCD_LenX;
}

/**
 * @brief  获取LCD的Y方向长度
 * @retval Y方向像素数
 */
uint16_t LCD_GetLenY(void)
{
    return g_LCD_LenY;
}

/**
 * @brief  设置LCD的前景(字体)及背景颜色
 * @param  TextColor: 前景颜色
 * @param  BackColor: 背景颜色
 * @retval 无
 */
void LCD_SetColors(uint16_t TextColor, uint16_t BackColor)
{
    g_LCD_TextColor = TextColor;
    g_LCD_BackColor = BackColor;
}

/**
 * @brief  获取LCD的前景(字体)及背景颜色
 * @param  TextColor: 存储前景颜色的指针
 * @param  BackColor: 存储背景颜色的指针
 * @retval 无
 */
void LCD_GetColors(uint16_t *TextColor, uint16_t *BackColor)
{
    *TextColor = g_LCD_TextColor;
    *BackColor = g_LCD_BackColor;
}

/**
 * @brief  设置LCD的前景(字体)颜色
 * @param  Color: 前景颜色
 * @retval 无
 */
void LCD_SetTextColor(uint16_t Color)
{
    g_LCD_TextColor = Color;
}

/**
 * @brief  设置LCD的背景颜色
 * @param  Color: 背景颜色
 * @retval 无
 */
void LCD_SetBackColor(uint16_t Color)
{
    g_LCD_BackColor = Color;
}

/**
 * @brief  对LCD显示器的某一窗口以某种颜色进行清屏
 * @param  usX: 窗口起点X坐标
 * @param  usY: 窗口起点Y坐标
 * @param  usWidth: 窗口宽度
 * @param  usHeight: 窗口高度
 * @retval 无
 */
void LCD_Clear(uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight)
{
    ILI9341_OpenWindow(usX, usY, usWidth, usHeight);
    ILI9341_FillColor(usWidth * usHeight, g_LCD_BackColor);
}

/**
 * @brief  对LCD显示器的某一点以某种颜色进行填充
 * @param  usX: 点的X坐标
 * @param  usY: 点的Y坐标
 * @retval 无
 */
void LCD_SetPixel(uint16_t usX, uint16_t usY)
{
    if ((usX < g_LCD_LenX) && (usY < g_LCD_LenY)) {
        ILI9341_OpenWindow(usX, usY, 1, 1);
        ILI9341_FillColor(1, g_LCD_TextColor);
    }
}

/**
 * @brief  获取LCD某一点的颜色值
 * @param  usX: 点的X坐标
 * @param  usY: 点的Y坐标
 * @retval 该点的颜色值
 */
uint16_t LCD_GetPixel(uint16_t usX, uint16_t usY)
{
    uint16_t color;
    
    ILI9341_OpenWindow(usX, usY, 1, 1);
    ILI9341_WriteCmd(CMD_GRAM);
    color = ILI9341_ReadData();
    color = ILI9341_ReadData();  /* 第一个读取的是dummy数据 */
    color = ILI9341_ReadData();
    
    return color;
}

/**
 * @brief  在LCD显示器上显示一个字模
 * @param  usX: 字符起始X坐标
 * @param  usY: 字符起始Y坐标
 * @param  usWidth: 字模宽度
 * @param  usHeight: 字模高度
 * @param  pMask: 字模数据指针
 * @retval 无
 */
static void LCD_DispMask(uint16_t usX, uint16_t usY, uint16_t usWidth, int16_t usHeight, const uint8_t *pMask)
{
    uint16_t i, j;
    uint16_t uFontLength;
    
    ILI9341_OpenWindow(usX, usY, usWidth, usHeight);
    ILI9341_WriteCmd(CMD_SetPixel);
    
    uFontLength = usWidth * usHeight / 8;
    
    for (i = 0; i < uFontLength; i++) {
        for (j = 0; j < 8; j++) {
            if (pMask[i] & (0x80 >> j)) {
                ILI9341_WriteData(g_LCD_TextColor);
            } else {
                ILI9341_WriteData(g_LCD_BackColor);
            }
        }
    }
}

/**
 * @brief  在LCD显示器上显示字符串
 * @param  usX: 起始X坐标
 * @param  usY: 起始Y坐标
 * @param  uDir: 显示方向 (0=X方向, 非0=Y方向)
 * @param  pStr: 要显示的字符串
 * @retval 无
 */
void LCD_DispStringEN(uint16_t usX, uint16_t usY, uint8_t uDir, char *pStr)
{
    uint8_t *pMask;
    uint16_t uCharWidth = LCD_GetFontEN()->Width;
    uint16_t uCharHeight = LCD_GetFontEN()->Height;
    uint16_t uX = usX;
    uint16_t uY = usY;
    
    while (*pStr != '\0') {
        pMask = LCD_GetMaskEN(*pStr);
        pStr++;
        
        if (g_LCD_LenX - uX < uCharWidth) {
            uX = 0;
            uY += uCharHeight;
        }
        if (g_LCD_LenY - uY < uCharHeight) {
            uY = 0;
        }
        
        LCD_DispMask(uX, uY, uCharWidth, uCharHeight, pMask);
        
        if (uDir == 0) {
            uX += uCharWidth;
        } else {
            uX += uCharHeight;
        }
    }
}

/**
 * @brief  画线
 * @param  usX1: 起点X坐标
 * @param  usY1: 起点Y坐标
 * @param  usX2: 终点X坐标
 * @param  usY2: 终点Y坐标
 * @retval 无
 */
void LCD_DrawLine(uint16_t usX1, uint16_t usY1, uint16_t usX2, uint16_t usY2)
{
    int16_t dx, dy, sx, sy, err, e2;
    uint16_t x1 = usX1, y1 = usY1, x2 = usX2, y2 = usY2;
    
    dx = (x1 < x2) ? (x2 - x1) : (x1 - x2);
    dy = (y1 < y2) ? (y2 - y1) : (y1 - y2);
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    err = dx - dy;
    
    while (1) {
        LCD_SetPixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

/**
 * @brief  画矩形
 * @param  usX_Start: 起始X坐标
 * @param  usY_Start: 起始Y坐标
 * @param  usWidth: 宽度
 * @param  usHeight: 高度
 * @param  ucFilled: 是否填充 (1填充, 0不填充)
 * @retval 无
 */
void LCD_DrawRectangle(uint16_t usX_Start, uint16_t usY_Start, uint16_t usWidth, uint16_t usHeight, uint8_t ucFilled)
{
    if (ucFilled) {
        LCD_Clear(usX_Start, usY_Start, usWidth, usHeight);
    } else {
        LCD_DrawLine(usX_Start, usY_Start, usX_Start + usWidth - 1, usY_Start);
        LCD_DrawLine(usX_Start, usY_Start, usX_Start, usY_Start + usHeight - 1);
        LCD_DrawLine(usX_Start + usWidth - 1, usY_Start, usX_Start + usWidth - 1, usY_Start + usHeight - 1);
        LCD_DrawLine(usX_Start, usY_Start + usHeight - 1, usX_Start + usWidth - 1, usY_Start + usHeight - 1);
    }
}

/**
 * @brief  画圆 (Bresenham算法)
 * @param  usX_Center: 圆心X坐标
 * @param  usY_Center: 圆心Y坐标
 * @param  usRadius: 半径
 * @param  ucFilled: 是否填充 (1填充, 0不填充)
 * @retval 无
 */
void LCD_DrawCircle(uint16_t usX_Center, uint16_t usY_Center, uint16_t usRadius, uint8_t ucFilled)
{
    int16_t x = 0, y = usRadius;
    int16_t d = 3 - 2 * usRadius;
    
    if (ucFilled) {
        while (x <= y) {
            LCD_DrawLine(usX_Center - x, usY_Center - y, usX_Center + x, usY_Center - y);
            LCD_DrawLine(usX_Center - x, usY_Center + y, usX_Center + x, usY_Center + y);
            LCD_DrawLine(usX_Center - y, usY_Center - x, usX_Center + y, usY_Center - x);
            LCD_DrawLine(usX_Center - y, usY_Center + x, usX_Center + y, usY_Center + x);
            if (d < 0) {
                d += 4 * x + 6;
            } else {
                d += 4 * (x - y) + 10;
                y--;
            }
            x++;
        }
    } else {
        while (x <= y) {
            LCD_SetPixel(usX_Center + x, usY_Center + y);
            LCD_SetPixel(usX_Center - x, usY_Center + y);
            LCD_SetPixel(usX_Center + x, usY_Center - y);
            LCD_SetPixel(usX_Center - x, usY_Center - y);
            LCD_SetPixel(usX_Center + y, usY_Center + x);
            LCD_SetPixel(usX_Center - y, usY_Center + x);
            LCD_SetPixel(usX_Center + y, usY_Center - x);
            LCD_SetPixel(usX_Center - y, usY_Center - x);
            if (d < 0) {
                d += 4 * x + 6;
            } else {
                d += 4 * (x - y) + 10;
                y--;
            }
            x++;
        }
    }
}