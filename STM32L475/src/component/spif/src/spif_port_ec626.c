/*
 * spif_port_ec626.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "ec626.h"
#include "bsp.h"

#include "spif.h"
#include "spif_port.h"

extern ARM_DRIVER_USART Driver_LPUSART1;

// #if (RTE_SPI0)
// extern ARM_DRIVER_SPI Driver_SPI0;
// static ARM_DRIVER_SPI *s_spi_drv = &Driver_SPI0;
// #endif

#if (RTE_SPI1)
extern ARM_DRIVER_SPI Driver_SPI1;
static ARM_DRIVER_SPI *s_spi_drv = &Driver_SPI1;
#endif

/* SPI1 SSn: PAD_PIN12 GPIO_7 */
#define EC626_SPI_CS_GPIO_PAD         12
#define EC626_SPI_CS_GPIO_INSTANCE    RTE_SPI1_SSN_GPIO_INSTANCE
#define EC626_SPI_CS_GPIO_INDEX       RTE_SPI1_SSN_GPIO_INDEX

#define EC626_SPI_FREQUENCY           (4 * 1000 * 1000)

static int ec626_log(const char *format, ...)
{
    char str_buf[512] = {0};
    va_list list;

    va_start(list, format);
    vsnprintf(str_buf, 512, format, list);
    va_end(list);

    Driver_LPUSART1.SendPolling(str_buf, strlen((const char*)str_buf));

    return 0;
}

static void _ec626_spi_cs_pin_init(void)
{
    pad_config_t pad_config = {0};
    gpio_pin_config_t gpio_config = {0};

    PAD_GetDefaultConfig(&pad_config);
    pad_config.mux = PAD_MuxAlt0;
    PAD_SetPinConfig(EC626_SPI_CS_GPIO_PAD, &pad_config);

    gpio_config.pinDirection = GPIO_DirectionOutput;
    gpio_config.misc.initOutput = 1U;
    GPIO_PinConfig(EC626_SPI_CS_GPIO_INSTANCE, EC626_SPI_CS_GPIO_INDEX, &gpio_config);
}

static void _ec626_spi_cs_set(uint8_t level)
{
    GPIO_PinWrite(EC626_SPI_CS_GPIO_INSTANCE, (1 << EC626_SPI_CS_GPIO_INDEX), (level == 1) ? (1 << EC626_SPI_CS_GPIO_INDEX) : 0);
}

static int _ec626_spi_init(void)
{
    int ret = 0;

    _ec626_spi_cs_pin_init();

    ret = s_spi_drv->Initialize(NULL);
    if (ret != ARM_DRIVER_OK) {
        return SPIF_FAIL;
    }

    ret = s_spi_drv->PowerControl(ARM_POWER_FULL);
    if (ret != ARM_DRIVER_OK) {
        return SPIF_FAIL;
    }

    ret = s_spi_drv->Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL0_CPHA0 | ARM_SPI_DATA_BITS(8) | ARM_SPI_MSB_LSB | ARM_SPI_SS_MASTER_SW, EC626_SPI_FREQUENCY);
    if (ret != ARM_DRIVER_OK) {
        return SPIF_FAIL;
    }

    return SPIF_SUCCESS;
}

static int _ec626_spi_deinit(void)
{
    // TODO
    return SPIF_SUCCESS;
}

static void _ec626_spi_lock(uint32_t ms)
{
    // TODO
}

static void _ec626_spi_unlock(void)
{
    // TODO
}

static int _ec626_spi_recv(uint8_t *recv_buf, uint32_t recv_size)
{
    int ret = 0;

    /**
     * 1. Nor Flash 所用到的指令中, 接收的数据应该不会超过 9 个字节
     * 2. 0xFF 为 dummy 数据
     */
    uint8_t send_buf[16] = {0xFF};

    if ((recv_buf == NULL) || (recv_size == 0) || (recv_size > 16)) {
        return SPIF_FAIL;
    }

    _ec626_spi_cs_set(0);

    /**
     * 1. 对于硬件 SPI 来说, 只是调用 Receive 函数是没有时钟信号输出的
     * 2. 只有调用 Transfer 函数, 在发送的时候同时接收, 才能有时钟信号输出
     */
    ret = s_spi_drv->Transfer(send_buf, recv_buf, recv_size);
    if (ret != ARM_DRIVER_OK) {
        return SPIF_FAIL;
    }

    _ec626_spi_cs_set(1);

    return SPIF_SUCCESS;
}

static int _ec626_spi_send(const uint8_t *send_buf, uint32_t send_size)
{
    int ret = 0;

    /* Nor Flash 所用到的指令中, 接收的数据应该不会超过 9 个字节 */
    uint8_t recv_buf[16] = {0x0};

    if ((send_buf == NULL) || (send_size == 0) || (send_size > 16)) {
        return SPIF_FAIL;
    }

    _ec626_spi_cs_set(0);

    /* 尽管调用 Send 函数是有时钟输出的, 为了统一, 这里也改为调用 Transfer 函数 */
    ret = s_spi_drv->Transfer(send_buf, recv_buf, send_size);
    if (ret != ARM_DRIVER_OK) {
        return SPIF_FAIL;
    }

    _ec626_spi_cs_set(1);

    return SPIF_SUCCESS;
}

int _ec626_spi_transfer(const uint8_t *send_buf, uint32_t send_size, uint8_t *recv_buf, uint32_t recv_size)
{
    int ret = 0;

    _ec626_spi_cs_set(0);

    ret = _ec626_spi_send(send_buf, send_size);
    if (ret != ARM_DRIVER_OK) {
        return SPIF_FAIL;
    }

    ret = _ec626_spi_recv(recv_buf, recv_size);
    if (ret != ARM_DRIVER_OK) {
        return SPIF_FAIL;
    }

    _ec626_spi_cs_set(1);

    return SPIF_SUCCESS;
}

void spif_port_ec626_spi_get(spif_port_spi_ops_t *ops)
{
    if (ops == NULL) {
        return;
    }

    ops->spi_init = _ec626_spi_init;
    ops->spi_deinit = _ec626_spi_deinit;
    ops->spi_lock = _ec626_spi_lock;
    ops->spi_unlock = _ec626_spi_unlock;
    ops->spi_send = _ec626_spi_send;
    ops->spi_recv = _ec626_spi_recv;
    ops->spi_transfer = _ec626_spi_transfer;
}

void spif_port_ec626_plat_get(spif_port_plat_ops_t *ops)
{
    if (ops == NULL) {
        return;
    }

    ops->log = ec626_log;
    ops->delay_ms = NULL;
}
