/*
 * spif_port_stm32l4xx.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "stm32l475xx.h"
#include "stm32l4xx_hal.h"

#include "spif.h"
#include "spif_port.h"

#define SPIF_QSPI_FLASH_SIZE    (POSITION_VAL(0x1000000))

static QSPI_HandleTypeDef s_qspi_handler;

static int _stm32l4xx_log(const char *format, ...)
{
    va_list list;

    va_start(list, format);
    vprintf(format, list);
    va_end(list);

    return 0;
}

static void _stm32l4xx_qspi_cs_set(uint8_t level)
{
    return;
}

static int _stm32l4xx_qspi_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /**
     * PE10: QUADSPI_CLK
     * PE11: QUADSPI_NCS
     * PE12: QUADSPI_BK1_IO0
     * PE13: QUADSPI_BK1_IO1
     * PE14: QUADSPI_BK1_IO2
     * PE15: QUADSPI_BK1_IO3
     */
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;

    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    s_qspi_handler.Instance = QUADSPI;
    /**
     * QPSI 分频比
     * QSPI Frequency = 80 / (0 + 1) = 40 MHz
     */
    s_qspi_handler.Init.ClockPrescaler = 0;
    /* QSPI FIFO = 4 Byte */
    s_qspi_handler.Init.FifoThreshold = 4;
    /* 采样移位半个周期 (DDR 模式下, 必须设置为 0) */
    s_qspi_handler.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    /* SPI FLASH 大小 */
    s_qspi_handler.Init.FlashSize = SPIF_QSPI_FLASH_SIZE - 1;
    /* 片选高电平时间为 4 个时钟 (12.5 * 4 = 50 ns), tSHSL = 50 ns */
    s_qspi_handler.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_4_CYCLE;
    /* 模式 0 */
    s_qspi_handler.Init.ClockMode = QSPI_CLOCK_MODE_0;

    if (HAL_QSPI_Init(&s_qspi_handler) != HAL_OK) {
        return SPIF_FAIL;
    }

    return SPIF_SUCCESS;
}

static int _stm32l4xx_qspi_deinit(void)
{
    // TODO
    return SPIF_SUCCESS;
}

static void _stm32l4xx_qspi_lock(uint32_t ms)
{
    // TODO
}

static void _stm32l4xx_qspi_unlock(void)
{
    // TODO
}

static int _stm32l4xx_qspi_recv(uint8_t *recv_buf, uint32_t recv_size)
{
    s_qspi_handler.Instance->DLR = recv_size - 1;

    if (HAL_QSPI_Receive(&s_qspi_handler, recv_buf, 5000) != HAL_OK) {
        return SPIF_FAIL;
    }

    return SPIF_SUCCESS;
}

static int _stm32l4xx_qspi_send(const uint8_t *send_buf, uint32_t send_size)
{
    s_qspi_handler.Instance->DLR = send_size - 1;

    if (HAL_QSPI_Transmit(&s_qspi_handler, send_buf, 5000) != HAL_OK) {
        return SPIF_FAIL;
    }

    return SPIF_SUCCESS;
}

int _stm32l4xx_qspi_transfer(const uint8_t *send_buf, uint32_t send_size, uint8_t *recv_buf, uint32_t recv_size)
{
    int ret = 0;

    ret = _stm32l4xx_qspi_send(send_buf, send_size);
    if (ret != SPIF_SUCCESS) {
        return SPIF_FAIL;
    }

    ret = _stm32l4xx_qspi_recv(recv_buf, recv_size);
    if (ret != SPIF_SUCCESS) {
        return SPIF_FAIL;
    }

    return SPIF_SUCCESS;
}

void spif_port_stm32l4xx_qspi_get(spif_port_spi_ops_t *ops)
{
    if (ops == NULL) {
        return;
    }

    ops->spi_init = _stm32l4xx_qspi_init;
    ops->spi_deinit = _stm32l4xx_qspi_deinit;
    ops->spi_lock = _stm32l4xx_qspi_lock;
    ops->spi_unlock = _stm32l4xx_qspi_unlock;
    ops->spi_send = _stm32l4xx_qspi_send;
    ops->spi_recv = _stm32l4xx_qspi_recv;
    ops->spi_transfer = _stm32l4xx_qspi_transfer;
}

void spif_port_stm32l4xx_plat_get(spif_port_plat_ops_t *ops)
{
    if (ops == NULL) {
        return;
    }

    ops->log = _stm32l4xx_log;
    ops->delay_ms = NULL;
}
