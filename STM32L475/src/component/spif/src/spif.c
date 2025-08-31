/*
 * spif.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#include <stdio.h>
#include "spif.h"
#include "spif_port.h"

#undef TAG
#define TAG "spif"

#define SPIF_DEBUG_ENABLE    1
#if SPIF_DEBUG_ENABLE
#define SPIF_DEBUG(tag, fmt, ...)    s_plat_ops.log("[D][" tag "] " fmt "\r\n", ##__VA_ARGS__)
#else
#define SPIF_DEBUG(tag, fmt, ...)
#endif

#define SPIF_INFO_ENABLE     1
#if SPIF_INFO_ENABLE
#define SPIF_INFO(tag, fmt, ...)     s_plat_ops.log("[I][" tag "] " fmt "\r\n", ##__VA_ARGS__)
#else
#define SPIF_INFO(tag, fmt, ...)
#endif

#define SPIF_WARN_ENABLE     1
#if SPIF_WARN_ENABLE
#define SPIF_WARN(tag, fmt, ...)     s_plat_ops.log("[W][" tag "] " fmt "\r\n", ##__VA_ARGS__)
#else
#define SPIF_WARN(tag, fmt, ...)
#endif

#define SPIF_ERROR_ENABLE    1
#if SPIF_ERROR_ENABLE
#define SPIF_ERROR(tag, fmt, ...)    s_plat_ops.log("[E][" tag "] " fmt "\r\n", ##__VA_ARGS__)
#else
#define SPIF_ERROR(tag, fmt, ...)
#endif

#define SPIF_MF_ID_GIANTEC    0xC4
#define SPIF_MF_ID_WINBOND    0xEF

/* Commands */
#define SPIF_CMD_WRITE_ENABLE          0x06
#define SPIF_CMD_WRITE_DISABLE         0x04

#define SPIF_CMD_READ_SFDP_REGISTER    0x5A
#define SPIF_CMD_READ_JEDEC_ID         0x9F

#define SPIF_CMD_READ_STATUS_REGISTER1 0x05
#define SPIF_CMD_READ_STATUS_REGISTER2 0x35
#define SPIF_CMD_READ_STATUS_REGISTER3 0x15

#define SPIF_CMD_PAGE_PROGRAM          0x02
#define SPIF_CMD_SECTOR_ERASE_4K       0x20
#define SPIF_CMD_BLOCK_ERASE_32K       0x52
#define SPIF_CMD_BLOCK_ERASE_64K       0xD8
#define SPIF_CMD_CHIP_ERASE            0xC7

#define SPIF_CMD_DUMMY                 0xFF

/* Status */
#define SPIF_STATUS_BUSY               (1 << 0)

#define SPIF_ARRAY_SIZE(x)    (sizeof(x)/sizeof(x[0])) 

typedef struct spif_flash_info_s {
    char *name;

    uint8_t mf_id;  /* Manufacturer ID */
    uint8_t mt_id;  /* Memory Type ID */
    uint8_t cap_id; /* Capacity ID */

    uint32_t chip_size;   /* unit: Byte */
    uint32_t block_size;  /* unit: Byte */
    uint32_t sector_size; /* unit: Byte */
    uint32_t page_size;   /* unit: Byte */
} spif_flash_info_t;

static spif_port_spi_ops_t s_spi_ops;
static spif_port_plat_ops_t s_plat_ops;

static spif_flash_info_t s_spif_flash_info[] = {
    /* W25Q128JV-IN/IQ/JQ (2.7V - 3.6V) */
    {
        .name    = "W25Q128JV-IN/IQ/JQ",
        .mf_id   = SPIF_MF_ID_WINBOND,
        .mt_id   = 0x40,
        .cap_id  = 0x18,
        .chip_size   = 16 * 1024 * 1024,
        .block_size  = 64 * 1024,
        .sector_size = 4 * 1024,
        .page_size   = 256,
    },

    /* GT25Q40D (1.65V - 3.6V) */
    {
        .name    = "GT25Q40D",
        .mf_id   = SPIF_MF_ID_GIANTEC,
        .mt_id   = 0x40,
        .cap_id  = 0x13,
        .chip_size   = 1 * 1024 * 1024,
        .block_size  = 32 * 1024,
        .sector_size = 4 * 1024,
        .page_size   = 256,
    },
};

static uint16_t s_spif_flash_index = 0;

static int _spif_read_jedec_id(uint8_t *buf, uint32_t buf_len)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_READ_JEDEC_ID};

    if ((buf == NULL) || (buf_len < 3)) {
        return SPIF_FAIL;
    }

    ret = s_spi_ops.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), buf, 3);
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi read id failed: %d.", ret);
        return ret;
    }

    return ret;
}

static int _spif_write_enable(void)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_WRITE_ENABLE};

    ret = s_spi_ops.spi_send(cmd, SPIF_ARRAY_SIZE(cmd));
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi write enable failed: %d.", ret);
        return ret;
    }

    return ret;
}

static int _spif_write_disable(void)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_WRITE_DISABLE};

    ret = s_spi_ops.spi_send(cmd, SPIF_ARRAY_SIZE(cmd));
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi write disable failed: %d.", ret);
        return ret;
    }

    return ret;
}

static int _spif_read_status_register1(uint8_t *status)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_READ_STATUS_REGISTER1};
    uint8_t buf[1] = {0};

    ret = s_spi_ops.spi_send(cmd, SPIF_ARRAY_SIZE(cmd));
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi read status1 w failed: %d.", ret);
        return ret;
    }

    ret = s_spi_ops.spi_recv(buf, SPIF_ARRAY_SIZE(buf));
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi read status1 r failed: %d.", ret);
        return ret;
    }
}

static int _spif_chip_erase(void)
{
    int ret = SPIF_SUCCESS;

    uint8_t status = 0;
    uint8_t cmd[] = {SPIF_CMD_CHIP_ERASE};

    ret = _spif_write_enable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    for (int i = 0; i < 500; i++) {
        ret = _spif_read_status_register1(&status);
        if (ret != SPIF_SUCCESS) {
            return ret;
        }

        if ((status & SPIF_STATUS_BUSY) == 0) {
            break;
        }

        // TODO delay some time
    }

    ret = s_spi_ops.spi_send(cmd, SPIF_ARRAY_SIZE(cmd));
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi write disable failed: %d.", ret);
        return ret;
    }

    for (int i = 0; i < 500; i++) {
        ret = _spif_read_status_register1(&status);
        if (ret != SPIF_SUCCESS) {
            return ret;
        }

        if ((status & SPIF_STATUS_BUSY) == 0) {
            break;
        }

        // TODO delay some time
    }

    ret = _spif_write_disable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    return ret;
}

static int _spif_page_program(uint32_t addr, uint8_t *data, uint32_t data_size)
{
    int ret = SPIF_SUCCESS;

    uint8_t status = 0;
    uint8_t cmd[] = {SPIF_CMD_PAGE_PROGRAM, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};

    if (data_size > s_spif_flash_info[s_spif_flash_index].page_size) {
        return SPIF_FAIL;
    }

    if (((addr & 0xFF) + data_size) > s_spif_flash_info[s_spif_flash_index].page_size) {
        return SPIF_FAIL;
    }

    ret = _spif_write_enable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    ret = s_spi_ops.spi_send(cmd, SPIF_ARRAY_SIZE(cmd));
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi page program w failed: %d.", ret);
        return ret;
    }

    ret = s_spi_ops.spi_send(data, data_size);
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi page program w failed: %d.", ret);
        return ret;
    }

    for (int i = 0; i < 500; i++) {
        ret = _spif_read_status_register1(&status);
        if (ret != SPIF_SUCCESS) {
            return ret;
        }

        if ((status & SPIF_STATUS_BUSY) == 0) {
            break;
        }

        // TODO delay some time
    }

    ret = _spif_write_disable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    return ret;
}

/**
 * @brief
 * @return see SPIF status code
 */
int spif_init(void)
{
    int ret = SPIF_SUCCESS;

    /* EC626 */
    // extern void spif_port_ec626_spi_get(spif_port_spi_ops_t *ops);
    // extern void spif_port_ec626_plat_get(spif_port_plat_ops_t *ops);
    // spif_port_ec626_spi_get(&s_spi_ops);
    // spif_port_ec626_plat_get(&s_plat_ops);

    /* STM32L475 */
    void spif_port_stm32l4xx_qspi_get(spif_port_spi_ops_t *ops);
    void spif_port_stm32l4xx_plat_get(spif_port_plat_ops_t *ops);
    spif_port_stm32l4xx_qspi_get(&s_spi_ops);
    spif_port_stm32l4xx_plat_get(&s_plat_ops);

    uint8_t buf[3] = {0};

    ret = s_spi_ops.spi_init();
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi port init failed: %d.", ret);
    }

    (void)_spif_read_jedec_id(buf, SPIF_ARRAY_SIZE(buf));

    SPIF_DEBUG(TAG, "Manufacturer : 0x%x.", buf[0]);
    SPIF_DEBUG(TAG, "Memory Type  : 0x%x.", buf[1]);
    SPIF_DEBUG(TAG, "Capacity     : 0x%x.", buf[2]);

    return ret;
}
