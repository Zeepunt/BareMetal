/*
 * watchpoint.h
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <stdio.h>
#include <stdint.h>

#if 0
#define WP_TRACE(fmt, ...)                 \
    do {                                   \
        printf(fmt "\r\n", ##__VA_ARGS__); \
    } while (0)
#else
#define WP_TRACE(fmt, ...)
#endif

typedef enum watchpoint_addrress_type_e {
    WP_ADDR_TYPE_BYTE     = 0, /* 8bit */
    WP_ADDR_TYPE_HALFWORD = 1, /* 16bit */
    WP_ADDR_TYPE_WORD     = 2, /* 32bit */
} wp_addr_type_t;

typedef enum watchpoint_access_type_e {
    WP_ACCESS_TYPE_RO = 0, /* read only */
    WP_ACCESS_TYPE_WO = 1, /* write only */
    WP_ACCESS_TYPE_RW = 2, /* read and write */
} wp_access_type_t;

typedef struct watchpoint_point_s {
    wp_addr_type_t addr_type;
    wp_access_type_t access_type;
    uint32_t addr;
} wp_point_t;

void watchpoint_init(void);
void watchpoint_deinit(void);

void watchpoint_start(void);
void watchpoint_stop(void);

int watchpoint_add_point(wp_point_t *point);
int watchpoint_del_point(wp_point_t *point);

#endif /* __WATCHPOINT_H__ */
