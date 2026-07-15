/**
  ******************************************************************************
  * @file    fpr_zn632_control.h
  * @author  Embedded Team
  * @version V1.0.0
  * @date    2026-07-15
  * @brief   ZN632指纹识别模块驱动控制头文件
  ******************************************************************************
  */

#ifndef __FPR_ZN632_CONTROL_H
#define __FPR_ZN632_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "main.h"

/*============================ 引脚定义 ============================*/
#define FPR_PWR_Pin         GPIO_PIN_9
#define FPR_PWR_GPIO_Port   GPIOC

/*============================ 缓冲区大小 ============================*/
#define FPR_RX_BUF_SIZE         128     /* 接收缓冲区大小 */
#define FPR_INDEX_TABLE_SIZE    32      /* 索引表大小（32字节=256位） */

/*============================ 超时定义 ============================*/
#define FPR_POWER_ON_TIMEOUT    2000    /* 上电等待超时（毫秒） */

/*============================ 错误码定义 ============================*/
#define FPR_OK                  0x00    /* 操作成功 */
#define FPR_ERROR               -1      /* 通用错误 */
#define FPR_ERROR_NO_FINGER     0x02    /* 未检测到手指 */
#define FPR_ERROR_IMAGE_FAIL    0x03    /* 图像采集失败 */
#define FPR_ERROR_FEATURE_FAIL  0x06    /* 特征提取失败 */
#define FPR_ERROR_NOT_MATCH     0x08    /* 指纹不匹配 */
#define FPR_ERROR_NOT_FOUND     0x09    /* 未找到指纹 */
#define FPR_ERROR_MERGE_FAIL    0x0A    /* 模板合并失败 */
#define FPR_ERROR_SLOT_FULL     0x1F    /* 指纹库已满 */
#define FPR_ERROR_WRITE_FLASH   0x18    /* Flash写入失败 */
#define FPR_ERROR_ALREADY_EXIST 0x10    /* 指纹已存在 */
#define FPR_ERROR_TIMEOUT       -2      /* 操作超时 */
#define FPR_ERROR_CHECKSUM      -3      /* 校验和错误 */
#define FPR_ERROR_PACKET        -4      /* 数据包错误 */
#define FPR_ERROR_ADDR          -5      /* 地址错误 */
#define FPR_ERROR_PARAM         -6      /* 参数错误 */
#define FPR_ERROR_DELETE_FAIL   0x10    /* 删除失败 */
#define FPR_ERROR_EMPTY_FAIL    0x11    /* 清空失败 */

#define FPR_INDEX_FULL          0xFFFF  /* 索引表已满 */

/*============================ 公共函数声明 ============================*/

/**
  * @brief  初始化指纹识别模块
  * @param  baud: 串口波特率
  * @retval FPR_OK: 成功  FPR_ERROR: 失败
  */
int16_t FPR_Init(uint32_t baud);

/**
  * @brief  反初始化指纹模块
  * @param  无
  * @retval 无
  */
void FPR_Deinit(void);

/**
  * @brief  指纹模块上电
  * @param  无
  * @retval FPR_OK: 成功  FPR_ERROR_TIMEOUT: 超时
  */
int16_t FPR_PowerOn(void);

/**
  * @brief  指纹模块断电
  * @param  无
  * @retval 无
  */
void FPR_PowerOff(void);

/**
  * @brief  验证模块密码
  * @param  password: 4字节密码（NULL表示默认密码0000）
  * @retval FPR_OK: 成功  其他: 错误码
  */
int16_t FPR_VerifyPassword(uint8_t *password);

/**
  * @brief  读取指纹索引表
  * @param  无
  * @retval FPR_OK: 成功  其他: 错误码
  */
int16_t FPR_ReadIndexTable(void);

/**
  * @brief  获取空闲槽位
  * @param  无
  * @retval 0-239: 空闲ID  FPR_INDEX_FULL: 已满
  */
uint16_t FPR_GetFreeSlot(void);

/**
  * @brief  检查槽位是否被占用
  * @param  slot: 槽位ID
  * @retval 1: 已占用  0: 空闲
  */
uint8_t FPR_IsSlotUsed(uint16_t slot);

/**
  * @brief  标记槽位为已使用
  * @param  slot: 槽位ID
  * @retval 无
  */
void FPR_SetSlotUsed(uint16_t slot);

/**
  * @brief  清除槽位使用标记
  * @param  slot: 槽位ID
  * @retval 无
  */
void FPR_ClearSlotUsed(uint16_t slot);

/**
  * @brief  获取指纹图像
  * @param  无
  * @retval FPR_OK: 成功  其他: 错误码
  */
int16_t FPR_GetImage(void);

/**
  * @brief  生成指纹特征
  * @param  buffer_id: 缓冲区ID (1 或 2)
  * @retval FPR_OK: 成功  其他: 错误码
  */
int16_t FPR_GenChar(uint8_t buffer_id);

/**
  * @brief  指纹1:1比对
  * @param  无
  * @retval FPR_OK: 匹配  FPR_ERROR_NOT_MATCH: 不匹配
  */
int16_t FPR_Match(void);

/**
  * @brief  合成指纹模板
  * @param  无
  * @retval FPR_OK: 成功  其他: 错误码
  */
int16_t FPR_RegModel(void);

/**
  * @brief  储存指纹模板
  * @param  buffer_id: 缓冲区ID (1 或 2)
  * @param  slot: 槽位ID (0-239)
  * @retval FPR_OK: 成功  其他: 错误码
  */
int16_t FPR_StoreChar(uint8_t buffer_id, uint16_t slot);

/**
  * @brief  高速搜索指纹库
  * @param  buffer_id: 缓冲区ID (1 或 2)
  * @param  p_slot: 输出匹配槽位
  * @param  p_score: 输出匹配分数
  * @retval FPR_OK: 找到匹配  FPR_ERROR_NOT_FOUND: 未找到
  */
int16_t FPR_HighSpeedSearch(uint8_t buffer_id, uint16_t *p_slot, uint16_t *p_score);

/**
  * @brief  删除指纹模板
  * @param  slot: 槽位ID
  * @param  count: 删除数量
  * @retval FPR_OK: 成功  其他: 错误码
  */
int16_t FPR_DeleteChar(uint16_t slot, uint16_t count);

/**
  * @brief  清空指纹库
  * @param  无
  * @retval FPR_OK: 成功  其他: 错误码
  */
int16_t FPR_EmptyDatabase(void);

/*============================ 高级功能函数 ============================*/

/**
  * @brief  录入新指纹（完整流程）
  * @param  无
  * @retval FPR_OK: 成功  其他: 错误码
  */
int16_t FPR_EnrollFinger(void);

/**
  * @brief  匹配指纹（完整流程）
  * @param  无
  * @retval FPR_OK: 匹配成功  FPR_ERROR_NOT_FOUND: 未找到
  */
int16_t FPR_IdentifyFinger(void);

/**
  * @brief  等待手指按下
  * @param  timeout_ms: 超时时间（毫秒）
  * @retval FPR_OK: 检测到手指  FPR_ERROR_TIMEOUT: 超时
  */
int16_t FPR_WaitFingerPress(uint32_t timeout_ms);

/**
  * @brief  获取指纹库使用信息
  * @param  p_used: 输出已使用数量
  * @param  p_total: 输出总容量
  * @retval 无
  */
void FPR_GetDatabaseInfo(uint16_t *p_used, uint16_t *p_total);

/**
  * @brief  获取原始接收数据
  * @param  p_buf: 输出缓冲区指针
  * @param  p_len: 输出数据长度
  * @retval 无
  */
void FPR_GetRawData(uint8_t **p_buf, uint32_t *p_len);

/**
  * @brief  设置退出标志
  * @param  flag: 1-退出 0-继续
  * @retval 无
  */
void FPR_SetExitFlag(uint8_t flag);

/**
  * @brief  获取退出标志
  * @param  无
  * @retval 当前退出标志
  */
uint8_t FPR_GetExitFlag(void);

/**
  * @brief  启动中断接收
  * @param  无
  * @retval 无
  */
void FPR_StartRxInterrupt(void);

/**
  * @brief  UART接收回调（在中断中调用）
  * @param  huart: UART句柄
  * @retval 无
  */
void FPR_UART_RxCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* __FPR_ZN632_CONTROL_H */