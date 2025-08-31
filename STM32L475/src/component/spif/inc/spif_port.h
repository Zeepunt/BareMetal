/*
 * spif_port.h
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#ifndef __SPIF_PORT_H__
#define __SPIF_PORT_H__

#include <stdint.h>

#define SPIF_SPI_OPS_SPI      0
#define SPIF_SPI_OPS_QSPI     1

#define SPIF_SPI_INVALID_ADDR (0xFFFFFFFF)

typedef struct spif_port_spi_operations_s {
    uint8_t ops_mode;

    void (*spi_lock)(uint32_t ms);
    void (*spi_unlock)(void);

    int (*spi_init)(void);
    int (*spi_deinit)(void);

    union {
        struct {
            int (*spi_send)(const uint8_t *tx_buf, uint32_t tx_size);
            int (*spi_recv)(uint8_t *rx_buf, uint32_t rx_size);
            int (*spi_transfer)(const uint8_t *tx_buf, uint32_t tx_size, uint8_t *rx_buf, uint32_t rx_size);
        } spi;

        struct {
            int (*qspi_transfer)(uint8_t cmd, uint32_t addr, const uint8_t *tx_buf, uint32_t tx_size, uint8_t *rx_buf, uint32_t rx_size);
        } qspi;
    } ops;
} spif_port_spi_ops_t;

typedef struct spif_port_platform_operations_s {
    int (*log)(const char *format, ...);
    void (*delay_us)(uint32_t us);
    void (*delay_ms)(uint32_t ms);
} spif_port_plat_ops_t;

#endif /* __SPIF_PORT_H__ */
