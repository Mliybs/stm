/**
  ******************************************************************************
  * @file    fpr_zn632_control.c
  * @author  Embedded Team
  * @version V1.0.0
  * @date    2026-07-15
  * @brief   ZN632指纹识别模块驱动控制源文件
  * @note    基于HAL库实现，提供完整的指纹录入、匹配、删除等功能
  ******************************************************************************
  * @attention
  * 硬件连接:
  *   - ZN632模块TXD -> STM32 PA10 (USART1_RX)
  *   - ZN632模块RXD -> STM32 PA9  (USART1_TX)
  *   - ZN632模块VCC -> 3.3V
  *   - ZN632模块GND -> GND
  *   - ZN632模块PWR -> PC9 (电源控制)
  ******************************************************************************
  */

#include "fpr_zn632.h"
#include <string.h>
#include <stdio.h>

/*============================ 宏定义 ============================*/

/* 命令码定义 */
#define CMD_GetImage        0x01    /* 获取指纹图像 */
#define CMD_GenChar         0x02    /* 生成特征文件 */
#define CMD_Match           0x03    /* 指纹比对(1:1) */
#define CMD_Search          0x04    /* 搜索指纹 */
#define CMD_RegModel        0x05    /* 合成模板 */
#define CMD_StoreChar       0x06    /* 储存模板 */
#define CMD_LoadChar        0x07    /* 读取模板 */
#define CMD_UpChar          0x08    /* 上传特征 */
#define CMD_DownChar        0x09    /* 下载特征 */
#define CMD_UpImage         0x0A    /* 上传图像 */
#define CMD_DownImage       0x0B    /* 下载图像 */
#define CMD_DeletChar       0x0C    /* 删除模板 */
#define CMD_Empty           0x0D    /* 清空指纹库 */
#define CMD_WriteReg        0x0E    /* 写寄存器 */
#define CMD_ReadSysPara     0x0F    /* 读系统参数 */
#define CMD_VryPwd          0x13    /* 密码验证 */
#define CMD_GetRandomCode   0x14    /* 获取随机数 */
#define CMD_SetChipAddr     0x15    /* 设置芯片地址 */
#define CMD_ReadINFpage     0x16    /* 读取信息页 */
#define CMD_Port_Control    0x17    /* 端口控制 */
#define CMD_HighSpeedSearch 0x1B    /* 高速搜索 */
#define CMD_ReadIndexTable  0x1F    /* 读取索引表 */

/* 数据包结构常量 */
#define PKT_START1          0xEF    /* 包头标识1 */
#define PKT_START2          0x01    /* 包头标识2 */
#define PKT_CMD             0x01    /* 命令包标识 */
#define PKT_DATA            0x02    /* 数据包标识 */
#define PKT_ACK             0x07    /* 应答包标识 */
#define PKT_END             0x08    /* 结束包标识 */

/*============================ 静态变量 ============================*/

/* 设备地址（默认0xFFFFFFFF） */
static uint8_t s_device_addr[4] = {0xFF, 0xFF, 0xFF, 0xFF};

/* 接收缓冲区 */
static uint8_t s_rx_buf[FPR_RX_BUF_SIZE];
static uint32_t s_rx_len = 0;

/* 指纹索引表（32字节，每bit表示一个指纹槽位） */
static uint8_t s_index_table[FPR_INDEX_TABLE_SIZE] = {0};

/* 指纹库最大容量 */
#define FPR_MAX_INDEX       240

/* 退出标志（用于中断长时间操作） */
static volatile uint8_t s_exit_flag = 0;

/* 模块初始化状态 */
static uint8_t s_is_initialized = 0;

/*============================ 外部变量引用 ============================*/

extern UART_HandleTypeDef huart1;  /* USART1句柄（指纹模块） */

/*============================ 静态函数声明 ============================*/

static void fpr_send_data(uint8_t *data, uint16_t len);
static void fpr_send_head(void);
static int16_t fpr_check_ack(void);
static void fpr_clear_rx_buf(void);
static void fpr_gpio_init(void);
static uint16_t fpr_calc_checksum(uint8_t *data, uint16_t len);
static void fpr_print_error(int16_t error_code);

/*============================ 公共接口函数 ============================*/

/**
  * @brief  初始化指纹识别模块
  * @param  baud: 串口波特率（通常为56700）
  * @retval FPR_OK: 初始化成功
  *         FPR_ERROR: 初始化失败
  */
int16_t FPR_Init(uint32_t baud)
{
    /* 如果已初始化，先复位 */
    if (s_is_initialized) {
        FPR_Deinit();
    }
    
    /* 初始化GPIO（电源控制） */
    fpr_gpio_init();
    
    /* 清空接收缓冲区 */
    fpr_clear_rx_buf();
    
    /* 清空索引表 */
    memset(s_index_table, 0, sizeof(s_index_table));
    
    /* 复位退出标志 */
    s_exit_flag = 0;
    
    /* 标记已初始化 */
    s_is_initialized = 1;
    
    /* 上电并等待握手信号 */
    return FPR_PowerOn();
}

/**
  * @brief  反初始化指纹识别模块
  * @param  无
  * @retval 无
  */
void FPR_Deinit(void)
{
    /* 关闭电源 */
    FPR_PowerOff();
    
    /* 复位状态标志 */
    s_is_initialized = 0;
    s_rx_len = 0;
    s_exit_flag = 0;
}

/**
  * @brief  指纹模块上电
  * @param  无
  * @retval FPR_OK: 上电成功（收到0x55握手信号）
  *         FPR_ERROR: 上电超时
  */
int16_t FPR_PowerOn(void)
{
    uint32_t timeout = 0;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    /* 清空接收缓冲区 */
    fpr_clear_rx_buf();
    
    /* 拉低电源控制脚，给模块供电 */
    HAL_GPIO_WritePin(FPR_PWR_GPIO_Port, FPR_PWR_Pin, GPIO_PIN_RESET);
    
    /* 等待模块稳定 */
    HAL_Delay(50);
    
    /* 等待握手信号（0x55） */
    while (timeout < FPR_POWER_ON_TIMEOUT) {
        if (s_rx_buf[0] == 0x55) {
            fpr_clear_rx_buf();
            return FPR_OK;
        }
        HAL_Delay(1);
        timeout++;
    }
    
    return FPR_ERROR_TIMEOUT;
}

/**
  * @brief  指纹模块断电
  * @param  无
  * @retval 无
  */
void FPR_PowerOff(void)
{
    HAL_GPIO_WritePin(FPR_PWR_GPIO_Port, FPR_PWR_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
}

/**
  * @brief  验证模块密码
  * @param  password: 4字节密码（默认全0）
  * @retval FPR_OK: 验证成功
  *         FPR_ERROR_ACK: 验证失败
  *         FPR_ERROR_TIMEOUT: 超时
  */
int16_t FPR_VerifyPassword(uint8_t *password)
{
    uint8_t cmd[10];
    uint16_t checksum;
    int16_t ret;
    uint8_t i;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;           /* 包标识：命令包 */
    cmd[1] = 0x00;              /* 包长度高字节 */
    cmd[2] = 0x07;              /* 包长度低字节（7字节有效数据） */
    cmd[3] = CMD_VryPwd;        /* 命令码 */
    
    /* 密码（4字节，默认全0） */
    for (i = 0; i < 4; i++) {
        cmd[4 + i] = password ? password[i] : 0;
    }
    
    /* 计算校验和 */
    checksum = fpr_calc_checksum(cmd, 8);
    cmd[8] = checksum >> 8;
    cmd[9] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 10);
    
    /* 等待模块处理 */
    HAL_Delay(300);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret != FPR_OK) {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/**
  * @brief  读取指纹索引表
  * @param  无
  * @retval FPR_OK: 读取成功
  *         FPR_ERROR_ACK: 应答错误
  *         FPR_ERROR_TIMEOUT: 超时
  */
int16_t FPR_ReadIndexTable(void)
{
    uint8_t cmd[7];
    uint16_t checksum;
    int16_t ret;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;
    cmd[1] = 0x00;
    cmd[2] = 0x04;
    cmd[3] = CMD_ReadIndexTable;
    cmd[4] = 0x00;              /* 页码0（ZN632只支持页0） */
    
    /* 计算校验和 */
    checksum = fpr_calc_checksum(cmd, 5);
    cmd[5] = checksum >> 8;
    cmd[6] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 7);
    
    /* 等待模块处理 */
    HAL_Delay(300);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret == FPR_OK) {
        /* 保存索引表数据（32字节） */
        memcpy(s_index_table, s_rx_buf + 10, FPR_INDEX_TABLE_SIZE);
    } else {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/**
  * @brief  获取一个空闲的指纹槽位ID
  * @param  无
  * @retval 0-239: 空闲槽位ID
  *         0xFF: 指纹库已满
  */
uint16_t FPR_GetFreeSlot(void)
{
    uint16_t slot = 0;
    uint8_t byte_idx, bit_idx;
    
    for (byte_idx = 0; byte_idx < FPR_INDEX_TABLE_SIZE; byte_idx++) {
        for (bit_idx = 0; bit_idx < 8; bit_idx++) {
            if (!(s_index_table[byte_idx] & (1 << bit_idx))) {
                return slot;
            }
            slot++;
        }
    }
    
    return FPR_INDEX_FULL;
}

/**
  * @brief  检查指定槽位是否已被占用
  * @param  slot: 槽位ID (0-239)
  * @retval 1: 已占用  0: 空闲
  */
uint8_t FPR_IsSlotUsed(uint16_t slot)
{
    uint8_t byte_idx, bit_idx;
    
    if (slot >= FPR_MAX_INDEX) {
        return 0;
    }
    
    byte_idx = slot / 8;
    bit_idx = slot % 8;
    
    return (s_index_table[byte_idx] & (1 << bit_idx)) ? 1 : 0;
}

/**
  * @brief  标记槽位为已使用
  * @param  slot: 槽位ID (0-239)
  * @retval 无
  */
void FPR_SetSlotUsed(uint16_t slot)
{
    uint8_t byte_idx, bit_idx;
    
    if (slot >= FPR_MAX_INDEX) {
        return;
    }
    
    byte_idx = slot / 8;
    bit_idx = slot % 8;
    s_index_table[byte_idx] |= (1 << bit_idx);
}

/**
  * @brief  标记槽位为空闲
  * @param  slot: 槽位ID (0-239)
  * @retval 无
  */
void FPR_ClearSlotUsed(uint16_t slot)
{
    uint8_t byte_idx, bit_idx;
    
    if (slot >= FPR_MAX_INDEX) {
        return;
    }
    
    byte_idx = slot / 8;
    bit_idx = slot % 8;
    s_index_table[byte_idx] &= ~(1 << bit_idx);
}

/**
  * @brief  获取指纹图像
  * @param  无
  * @retval FPR_OK: 成功获取图像
  *         FPR_ERROR_NO_FINGER: 未检测到手指
  *         FPR_ERROR_IMAGE_FAIL: 图像采集失败
  *         其他: 参考错误码定义
  */
int16_t FPR_GetImage(void)
{
    uint8_t cmd[6];
    uint16_t checksum;
    int16_t ret;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;
    cmd[1] = 0x00;
    cmd[2] = 0x03;
    cmd[3] = CMD_GetImage;
    
    checksum = fpr_calc_checksum(cmd, 4);
    cmd[4] = checksum >> 8;
    cmd[5] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 6);
    
    /* 等待模块处理（需要较长时间） */
    HAL_Delay(500);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret != FPR_OK) {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/**
  * @brief  生成指纹特征
  * @param  buffer_id: 特征缓冲区ID (1 或 2)
  * @retval FPR_OK: 生成成功
  *         FPR_ERROR_FEATURE_FAIL: 生成特征失败
  *         其他: 参考错误码定义
  */
int16_t FPR_GenChar(uint8_t buffer_id)
{
    uint8_t cmd[7];
    uint16_t checksum;
    int16_t ret;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    if (buffer_id != 1 && buffer_id != 2) {
        return FPR_ERROR_PARAM;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;
    cmd[1] = 0x00;
    cmd[2] = 0x04;
    cmd[3] = CMD_GenChar;
    cmd[4] = buffer_id;
    
    checksum = fpr_calc_checksum(cmd, 5);
    cmd[5] = checksum >> 8;
    cmd[6] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 7);
    
    /* 等待模块处理 */
    HAL_Delay(500);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret != FPR_OK) {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/**
  * @brief  指纹1:1比对
  * @param  无
  * @retval FPR_OK: 比对成功（匹配）
  *         FPR_ERROR_NOT_MATCH: 不匹配
  *         其他: 参考错误码定义
  */
int16_t FPR_Match(void)
{
    uint8_t cmd[6];
    uint16_t checksum;
    int16_t ret;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;
    cmd[1] = 0x00;
    cmd[2] = 0x03;
    cmd[3] = CMD_Match;
    
    checksum = fpr_calc_checksum(cmd, 4);
    cmd[4] = checksum >> 8;
    cmd[5] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 6);
    
    /* 等待模块处理 */
    HAL_Delay(500);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret != FPR_OK) {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/**
  * @brief  合成指纹模板
  * @param  无
  * @retval FPR_OK: 合成成功
  *         FPR_ERROR_MERGE_FAIL: 合并失败
  *         其他: 参考错误码定义
  */
int16_t FPR_RegModel(void)
{
    uint8_t cmd[6];
    uint16_t checksum;
    int16_t ret;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;
    cmd[1] = 0x00;
    cmd[2] = 0x03;
    cmd[3] = CMD_RegModel;
    
    checksum = fpr_calc_checksum(cmd, 4);
    cmd[4] = checksum >> 8;
    cmd[5] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 6);
    
    /* 等待模块处理 */
    HAL_Delay(500);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret != FPR_OK) {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/**
  * @brief  储存指纹模板到指纹库
  * @param  buffer_id: 特征缓冲区ID (1 或 2)
  * @param  slot: 储存槽位 (0-239)
  * @retval FPR_OK: 储存成功
  *         FPR_ERROR_SLOT_FULL: 槽位已满
  *         FPR_ERROR_WRITE_FLASH: Flash写入失败
  *         其他: 参考错误码定义
  */
int16_t FPR_StoreChar(uint8_t buffer_id, uint16_t slot)
{
    uint8_t cmd[9];
    uint16_t checksum;
    int16_t ret;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    if (buffer_id != 1 && buffer_id != 2) {
        return FPR_ERROR_PARAM;
    }
    
    if (slot >= FPR_MAX_INDEX) {
        return FPR_ERROR_PARAM;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;
    cmd[1] = 0x00;
    cmd[2] = 0x06;
    cmd[3] = CMD_StoreChar;
    cmd[4] = buffer_id;
    cmd[5] = slot >> 8;
    cmd[6] = slot & 0xFF;
    
    checksum = fpr_calc_checksum(cmd, 7);
    cmd[7] = checksum >> 8;
    cmd[8] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 9);
    
    /* 等待模块处理 */
    HAL_Delay(300);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret == FPR_OK) {
        /* 标记槽位已使用 */
        FPR_SetSlotUsed(slot);
    } else {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/**
  * @brief  高速搜索指纹库
  * @param  buffer_id: 特征缓冲区ID (1 或 2)
  * @param  p_slot: 输出匹配到的槽位ID
  * @param  p_score: 输出匹配分数
  * @retval FPR_OK: 搜索成功（找到匹配）
  *         FPR_ERROR_NOT_FOUND: 未找到匹配
  *         其他: 参考错误码定义
  */
int16_t FPR_HighSpeedSearch(uint8_t buffer_id, uint16_t *p_slot, uint16_t *p_score)
{
    uint8_t cmd[11];
    uint16_t checksum;
    int16_t ret;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    if (buffer_id != 1 && buffer_id != 2) {
        return FPR_ERROR_PARAM;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;
    cmd[1] = 0x00;
    cmd[2] = 0x08;
    cmd[3] = CMD_HighSpeedSearch;
    cmd[4] = buffer_id;
    cmd[5] = 0x00;              /* 起始页高字节 */
    cmd[6] = 0x00;              /* 起始页低字节 */
    cmd[7] = FPR_MAX_INDEX >> 8;/* 搜索页数高字节 */
    cmd[8] = FPR_MAX_INDEX;     /* 搜索页数低字节 */
    
    checksum = fpr_calc_checksum(cmd, 9);
    cmd[9] = checksum >> 8;
    cmd[10] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 11);
    
    /* 等待模块处理 */
    HAL_Delay(300);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret == FPR_OK) {
        /* 提取匹配结果 */
        if (p_slot) {
            *p_slot = (s_rx_buf[10] << 8) | s_rx_buf[11];
        }
        if (p_score) {
            *p_score = (s_rx_buf[12] << 8) | s_rx_buf[13];
        }
    } else {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/**
  * @brief  删除指纹模板
  * @param  slot: 要删除的槽位ID
  * @param  count: 删除数量（通常为1）
  * @retval FPR_OK: 删除成功
  *         FPR_ERROR_DELETE_FAIL: 删除失败
  *         其他: 参考错误码定义
  */
int16_t FPR_DeleteChar(uint16_t slot, uint16_t count)
{
    uint8_t cmd[9];
    uint16_t checksum;
    int16_t ret;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    if (slot >= FPR_MAX_INDEX) {
        return FPR_ERROR_PARAM;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;
    cmd[1] = 0x00;
    cmd[2] = 0x06;
    cmd[3] = CMD_DeletChar;
    cmd[4] = slot >> 8;
    cmd[5] = slot & 0xFF;
    cmd[6] = count >> 8;
    cmd[7] = count & 0xFF;
    
    checksum = fpr_calc_checksum(cmd, 8);
    cmd[8] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 9);
    
    /* 等待模块处理 */
    HAL_Delay(300);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret == FPR_OK) {
        /* 清除槽位标记 */
        for (uint16_t i = 0; i < count; i++) {
            FPR_ClearSlotUsed(slot + i);
        }
    } else {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/**
  * @brief  清空整个指纹库
  * @param  无
  * @retval FPR_OK: 清空成功
  *         FPR_ERROR_EMPTY_FAIL: 清空失败
  */
int16_t FPR_EmptyDatabase(void)
{
    uint8_t cmd[6];
    uint16_t checksum;
    int16_t ret;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    /* 构造命令包 */
    cmd[0] = PKT_CMD;
    cmd[1] = 0x00;
    cmd[2] = 0x03;
    cmd[3] = CMD_Empty;
    
    checksum = fpr_calc_checksum(cmd, 4);
    cmd[4] = checksum >> 8;
    cmd[5] = checksum & 0xFF;
    
    /* 发送命令 */
    fpr_send_head();
    fpr_send_data(cmd, 6);
    
    /* 等待模块处理 */
    HAL_Delay(300);
    
    /* 检查应答 */
    ret = fpr_check_ack();
    if (ret == FPR_OK) {
        /* 清空索引表 */
        memset(s_index_table, 0, sizeof(s_index_table));
    } else {
        fpr_print_error(ret);
    }
    
    fpr_clear_rx_buf();
    return ret;
}

/*============================ 高级功能函数 ============================*/

/**
  * @brief  录入新指纹（完整流程）
  * @param  无
  * @retval FPR_OK: 录入成功
  *         FPR_ERROR: 录入失败
  *         其他: 参考错误码定义
  * @note   完整流程：采集图像(2次) -> 生成特征 -> 合成模板 -> 储存
  */
int16_t FPR_EnrollFinger(void)
{
    int16_t ret;
    uint16_t slot;
    uint16_t match_slot;
    uint16_t score;
    uint8_t step;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    /* 检查是否有空闲槽位 */
    slot = FPR_GetFreeSlot();
    if (slot == FPR_INDEX_FULL) {
        printf("Fingerprint database is full!\r\n");
        return FPR_ERROR_SLOT_FULL;
    }
    
    /* 采集两次指纹 */
    for (step = 1; step <= 2; step++) {
        printf("Please place finger on sensor (Step %d/2)...\r\n", step);
        
        /* 获取图像 */
        ret = FPR_GetImage();
        if (ret != FPR_OK) {
            printf("Get image failed (code: %d)\r\n", ret);
            return ret;
        }
        
        /* 生成特征 */
        ret = FPR_GenChar(step);
        if (ret != FPR_OK) {
            printf("Generate character failed (code: %d)\r\n", ret);
            return ret;
        }
        
        /* 检查指纹是否已注册（仅在第一次采集时搜索） */
        if (step == 1) {
            ret = FPR_HighSpeedSearch(1, &match_slot, &score);
            if (ret == FPR_OK) {
                printf("Finger already registered at slot %d!\r\n", match_slot);
                return FPR_ERROR_ALREADY_EXIST;
            }
        }
        
        printf("Finger captured successfully (Step %d/2)\r\n", step);
    }
    
    /* 合成模板 */
    printf("Merging fingerprint templates...\r\n");
    ret = FPR_RegModel();
    if (ret != FPR_OK) {
        printf("Merge failed (code: %d)\r\n", ret);
        return ret;
    }
    
    /* 储存模板 */
    printf("Storing template to slot %d...\r\n", slot);
    ret = FPR_StoreChar(2, slot);
    if (ret != FPR_OK) {
        printf("Store failed (code: %d)\r\n", ret);
        return ret;
    }
    
    printf("Finger enrolled successfully! Slot: %d\r\n", slot);
    return FPR_OK;
}

/**
  * @brief  匹配指纹（完整流程）
  * @param  无
  * @retval FPR_OK: 匹配成功
  *         FPR_ERROR_NOT_FOUND: 未找到匹配
  *         其他: 参考错误码定义
  */
int16_t FPR_IdentifyFinger(void)
{
    int16_t ret;
    uint16_t slot;
    uint16_t score;
    
    if (!s_is_initialized) {
        return FPR_ERROR;
    }
    
    printf("Please place finger on sensor...\r\n");
    
    /* 获取图像 */
    ret = FPR_GetImage();
    if (ret != FPR_OK) {
        printf("Get image failed (code: %d)\r\n", ret);
        return ret;
    }
    
    /* 生成特征 */
    ret = FPR_GenChar(1);
    if (ret != FPR_OK) {
        printf("Generate character failed (code: %d)\r\n", ret);
        return ret;
    }
    
    /* 搜索指纹库 */
    ret = FPR_HighSpeedSearch(1, &slot, &score);
    if (ret == FPR_OK) {
        printf("Finger matched! Slot: %d, Score: %d\r\n", slot, score);
        return FPR_OK;
    } else if (ret == FPR_ERROR_NOT_FOUND) {
        printf("Finger not found in database!\r\n");
        return FPR_ERROR_NOT_FOUND;
    } else {
        printf("Search failed (code: %d)\r\n", ret);
        return ret;
    }
}

/**
  * @brief  阻塞等待手指按下（带超时）
  * @param  timeout_ms: 超时时间（毫秒）
  * @retval FPR_OK: 检测到手指
  *         FPR_ERROR_TIMEOUT: 超时
  *         FPR_ERROR_NO_FINGER: 未检测到手指
  */
int16_t FPR_WaitFingerPress(uint32_t timeout_ms)
{
    int16_t ret;
    uint32_t start_time = HAL_GetTick();
    
    while (HAL_GetTick() - start_time < timeout_ms) {
        ret = FPR_GetImage();
        if (ret == FPR_OK) {
            return FPR_OK;
        } else if (ret != FPR_ERROR_NO_FINGER) {
            return ret;
        }
        HAL_Delay(50);
    }
    
    return FPR_ERROR_TIMEOUT;
}

/**
  * @brief  获取指纹库使用信息
  * @param  p_used: 输出已使用数量
  * @param  p_total: 输出总容量
  * @retval 无
  */
void FPR_GetDatabaseInfo(uint16_t *p_used, uint16_t *p_total)
{
    uint16_t used = 0;
    
    for (uint16_t i = 0; i < FPR_MAX_INDEX; i++) {
        if (FPR_IsSlotUsed(i)) {
            used++;
        }
    }
    
    if (p_used) *p_used = used;
    if (p_total) *p_total = FPR_MAX_INDEX;
}

/**
  * @brief  获取最后接收到的原始数据
  * @param  p_buf: 输出缓冲区指针
  * @param  p_len: 输出数据长度
  * @retval 无
  */
void FPR_GetRawData(uint8_t **p_buf, uint32_t *p_len)
{
    if (p_buf) *p_buf = s_rx_buf;
    if (p_len) *p_len = s_rx_len;
}

/**
  * @brief  设置退出标志（用于中断长时间操作）
  * @param  flag: 1-退出 0-继续
  * @retval 无
  */
void FPR_SetExitFlag(uint8_t flag)
{
    s_exit_flag = flag;
}

/**
  * @brief  获取退出标志
  * @param  无
  * @retval 当前退出标志值
  */
uint8_t FPR_GetExitFlag(void)
{
    return s_exit_flag;
}

/*============================ 静态函数实现 ============================*/

/**
  * @brief  GPIO初始化（电源控制）
  * @param  无
  * @retval 无
  */
static void fpr_gpio_init(void)
{
    /* 电源控制引脚已在CubeMX中配置为输出 */
    /* 初始状态：高电平（关闭模块电源） */
    HAL_GPIO_WritePin(FPR_PWR_GPIO_Port, FPR_PWR_Pin, GPIO_PIN_SET);
}

/**
  * @brief  发送数据
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @retval 无
  */
static void fpr_send_data(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart1, data, len, 100);
}

/**
  * @brief  发送命令头
  * @param  无
  * @retval 无
  */
static void fpr_send_head(void)
{
    uint8_t head[6];
    
    head[0] = 0xEF;                     /* 包标识1 */
    head[1] = 0x01;                     /* 包标识2 */
    head[2] = s_device_addr[0];         /* 设备地址 */
    head[3] = s_device_addr[1];
    head[4] = s_device_addr[2];
    head[5] = s_device_addr[3];
    
    fpr_send_data(head, 6);
}

/**
  * @brief  检查应答包
  * @param  无
  * @retval 确认码
  */
static int16_t fpr_check_ack(void)
{
    uint16_t i;
    uint16_t len;
    uint16_t sum = 0;
    uint16_t calc_sum = 0;
    
    /* 检查包长度 */
    if (s_rx_len < 10) {
        return FPR_ERROR_PACKET;
    }
    
    /* 检查包头 */
    if (s_rx_buf[0] != 0xEF || s_rx_buf[1] != 0x01) {
        return FPR_ERROR_PACKET;
    }
    
    /* 检查地址 */
    if (s_rx_buf[2] != s_device_addr[0] || s_rx_buf[3] != s_device_addr[1] ||
        s_rx_buf[4] != s_device_addr[2] || s_rx_buf[5] != s_device_addr[3]) {
        return FPR_ERROR_ADDR;
    }
    
    /* 检查包类型 */
    if (s_rx_buf[6] != PKT_ACK) {
        return FPR_ERROR_PACKET;
    }
    
    /* 获取数据长度 */
    len = (s_rx_buf[7] << 8) | s_rx_buf[8];
    
    /* 检查数据长度是否匹配 */
    if (s_rx_len < (9 + len)) {
        return FPR_ERROR_PACKET;
    }
    
    /* 计算校验和 */
    for (i = 0; i <= len; i++) {
        sum += s_rx_buf[6 + i];
    }
    calc_sum = (s_rx_buf[9 + len - 2] << 8) | s_rx_buf[9 + len - 1];
    
    if (sum != calc_sum) {
        return FPR_ERROR_CHECKSUM;
    }
    
    /* 返回确认码 */
    return s_rx_buf[9];
}

/**
  * @brief  清空接收缓冲区
  * @param  无
  * @retval 无
  */
static void fpr_clear_rx_buf(void)
{
    memset(s_rx_buf, 0, sizeof(s_rx_buf));
    s_rx_len = 0;
}

/**
  * @brief  计算校验和
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @retval 校验和（16位）
  */
static uint16_t fpr_calc_checksum(uint8_t *data, uint16_t len)
{
    uint16_t sum = 0;
    
    for (uint16_t i = 0; i < len; i++) {
        sum += data[i];
    }
    
    return sum;
}

/**
  * @brief  打印错误信息
  * @param  error_code: 错误码
  * @retval 无
  */
static void fpr_print_error(int16_t error_code)
{
    switch (error_code) {
        case FPR_OK:
            printf("OK\r\n");
            break;
        case FPR_ERROR_NO_FINGER:
            printf("No finger detected\r\n");
            break;
        case FPR_ERROR_IMAGE_FAIL:
            printf("Image capture failed\r\n");
            break;
        case FPR_ERROR_FEATURE_FAIL:
            printf("Feature extraction failed\r\n");
            break;
        case FPR_ERROR_NOT_MATCH:
            printf("Finger does not match\r\n");
            break;
        case FPR_ERROR_NOT_FOUND:
            printf("Finger not found in database\r\n");
            break;
        case FPR_ERROR_MERGE_FAIL:
            printf("Template merge failed\r\n");
            break;
        case FPR_ERROR_SLOT_FULL:
            printf("Database is full\r\n");
            break;
        case FPR_ERROR_WRITE_FLASH:
            printf("Flash write error\r\n");
            break;
        case FPR_ERROR_ALREADY_EXIST:
            printf("Finger already exists in database\r\n");
            break;
        case FPR_ERROR_TIMEOUT:
            printf("Operation timeout\r\n");
            break;
        case FPR_ERROR_CHECKSUM:
            printf("Checksum error\r\n");
            break;
        case FPR_ERROR_PACKET:
            printf("Packet error\r\n");
            break;
        case FPR_ERROR_ADDR:
            printf("Address error\r\n");
            break;
        case FPR_ERROR_PARAM:
            printf("Parameter error\r\n");
            break;
        default:
            printf("Unknown error: %d (0x%02X)\r\n", error_code, error_code);
            break;
    }
}

/*============================ UART中断接收处理 ============================*/

/**
  * @brief  串口接收中断回调（在stm32f4xx_it.c中调用）
  * @param  huart: UART句柄
  * @retval 无
  * @note   此函数应放在中断回调中调用
  */
void FPR_UART_RxCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        /* 数据已由HAL库接收到s_rx_buf中 */
        /* s_rx_len已在HAL回调中更新 */
    }
}

/**
  * @brief  启动中断接收
  * @param  无
  * @retval 无
  * @note   在主循环开始前调用
  */
void FPR_StartRxInterrupt(void)
{
    HAL_UART_Receive_IT(&huart1, s_rx_buf, 1);
}

/**
  * @brief  HAL库接收完成回调（重定向）
  * @param  huart: UART句柄
  * @retval 无
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        s_rx_len++;
        /* 继续接收下一个字节 */
        HAL_UART_Receive_IT(&huart1, s_rx_buf + s_rx_len, 1);
    }
}