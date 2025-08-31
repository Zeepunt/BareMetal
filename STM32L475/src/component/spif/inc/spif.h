/*
 * spif.h
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#ifndef __SPIF_H__
#define __SPIF_H__

#include <stdint.h>

/* SPIF status code */
#define SPIF_SUCCESS  (0)
#define SPIF_FAIL     (-1)

typedef struct {
    char *name;      /* flash name */
    uint8_t mf_id;   /* manufacturer ID */
    uint8_t mt_id;   /* memory type ID */
    uint8_t cap_id;  /* capacity ID */
    uint32_t size;   /* flash size, unit: bytes */
} spif_flash_t;

/**
 * @brief
 * @return see SPIF status code
 */
int spif_init(void);

#endif /* __SPIF_H__ */
