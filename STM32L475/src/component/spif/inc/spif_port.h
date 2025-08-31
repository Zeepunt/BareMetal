/*
 * spif_port.h
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#ifndef __SPIF_PORT_H__
#define __SPIF_PORT_H__

#include <stdint.h>

typedef struct spif_port_spi_operations_s {
    void (*spi_lock)(uint32_t ms);
    void (*spi_unlock)(void);

    int (*spi_init)(void);
    int (*spi_deinit)(void);

    int (*spi_send)(const uint8_t *send_buf, uint32_t send_size);
    int (*spi_recv)(uint8_t *recv_buf, uint32_t recv_size);
    int (*spi_transfer)(const uint8_t *send_buf, uint32_t send_size, uint8_t *recv_buf, uint32_t recv_size);
} spif_port_spi_ops_t;

typedef struct spif_port_platform_operations_s {
    int (*log)(const char *format, ...);
    void (*delay_ms)(uint32_t ms);
} spif_port_plat_ops_t;

#endif /* __SPIF_PORT_H__ */
