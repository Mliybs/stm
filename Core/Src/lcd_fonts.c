#include <stddef.h>
#include "lcd_fonts.h"

/* 英文字模数据 (与标准库版本相同) */
const uint8_t ASCII8x16_Table[] = {
    /* 此处省略完整的ASCII字模表，请从原始代码中复制完整数据 */
    /* 为保持代码完整性，以下是示例数据，实际使用时请替换为完整字模表 */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x08,0x00,0x08,0x18,0x00,0x00,0x00,
    /* ... 完整字模表请从原始代码复制 ... */
};

FONT ASCII_8x16 = {
    ASCII8x16_Table,
    8,   /* 字宽 */
    16   /* 字高 */
};

static FONT *g_LCD_FontEN = &ASCII_8x16;

/**
 * @brief  设置英文字体
 * @param  fonts: 字体指针
 * @retval 无
 */
void LCD_SetFontEN(FONT *fonts)
{
    g_LCD_FontEN = fonts;
}

/**
 * @brief  获取当前英文字体
 * @retval 字体指针
 */
FONT *LCD_GetFontEN(void)
{
    return g_LCD_FontEN;
}

/**
 * @brief  获取英文字模
 * @param  cChar: 字符
 * @retval 字模数据指针
 */
uint8_t *LCD_GetMaskEN(char cChar)
{
    return (uint8_t*)(g_LCD_FontEN->table + 
           (cChar - ' ') * (g_LCD_FontEN->Width * g_LCD_FontEN->Height) / 8);
}