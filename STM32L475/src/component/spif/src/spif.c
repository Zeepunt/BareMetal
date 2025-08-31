/*
 * spif.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#include <stdio.h>
#include <string.h>
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

#define SPIF_CMD_READ_DATA             0x03
#define SPIF_CMD_FAST_READ             0x0B

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

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), buf, 3);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], SPIF_SPI_INVALID_ADDR, NULL, 0, buf, 3);
    }
    
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi read jedec id failed: %d.", ret);
        return ret;
    }

    return ret;
}

static int _spif_read_status_register1(uint8_t *status)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_READ_STATUS_REGISTER1};
    uint8_t buf[1] = {0};

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), buf, 1);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], SPIF_SPI_INVALID_ADDR, NULL, 0, buf, 1);
    }

    *status = buf[0];

    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi read status_register1 failed: %d.", ret);
        return ret;
    }

    return ret;
}

static int _spif_write_enable(void)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_WRITE_ENABLE};

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), NULL, 0);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], SPIF_SPI_INVALID_ADDR, NULL, 0, NULL, 0);
    }

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

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), NULL, 0);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], SPIF_SPI_INVALID_ADDR, NULL, 0, NULL, 0);
    }

    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi write disable failed: %d.", ret);
        return ret;
    }

    return ret;
}

static int _spif_wait_idle(void)
{
    int ret = SPIF_SUCCESS;
    uint8_t status = 0xFF;

    for (int i = 0; i < 500; i++) {
        ret = _spif_read_status_register1(&status);
        if (ret != SPIF_SUCCESS) {
            return ret;
        }

        if ((status & SPIF_STATUS_BUSY) == 0) {
            break;
        }

        s_plat_ops.delay_us(50);
    }

    return ret;
}

static int _spif_chip_erase(void)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_CHIP_ERASE};

    ret = _spif_write_enable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    _spif_wait_idle();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), NULL, 0);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], SPIF_SPI_INVALID_ADDR, NULL, 0, NULL, 0);
    }

    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "spi chip erase failed: %d.", ret);
        return ret;
    }

    _spif_wait_idle();

    ret = _spif_write_disable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    return ret;
}

int spif_block_erase_32(uint32_t addr)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_BLOCK_ERASE_32K, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};

    ret = _spif_write_enable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    _spif_wait_idle();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), NULL, 0);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], addr, NULL, 0, NULL, 0);
    }

    _spif_wait_idle();

    ret = _spif_write_disable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    return ret;
}

int spif_block_erase_64(uint32_t addr)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_BLOCK_ERASE_64K, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};

    ret = _spif_write_enable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    _spif_wait_idle();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), NULL, 0);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], addr, NULL, 0, NULL, 0);
    }

    _spif_wait_idle();

    ret = _spif_write_disable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    return ret;
}

int spif_sector_erase(uint32_t addr)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_SECTOR_ERASE_4K, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};

    ret = _spif_write_enable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    _spif_wait_idle();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), NULL, 0);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], addr, NULL, 0, NULL, 0);
    }

    _spif_wait_idle();

    ret = _spif_write_disable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    return ret;
}

int spif_page_program(uint32_t addr, uint8_t *data, uint32_t data_size)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_PAGE_PROGRAM, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};

    if (data_size > s_spif_flash_info[s_spif_flash_index].page_size) {
        SPIF_ERROR(TAG, "invalid data size.");
        return SPIF_FAIL;
    }

    if (((addr & 0xFF) + data_size) > s_spif_flash_info[s_spif_flash_index].page_size) {
        SPIF_ERROR(TAG, "page program out of range.");
        return SPIF_FAIL;
    }

    ret = _spif_write_enable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    ret = _spif_wait_idle();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), NULL, 0);
        
        ret = s_spi_ops.ops.spi.spi_send(data, data_size);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], addr, data, data_size, NULL, 0);
    }

    ret = _spif_wait_idle();

    ret = _spif_write_disable();
    if (ret != SPIF_SUCCESS) {
        return ret;
    }

    return ret;
}

int spif_read(uint32_t addr, uint8_t *data, uint32_t data_size)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_READ_DATA, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), data, data_size);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], addr, NULL, 0, data, data_size);
    }

    return ret;
}

int spif_fast_read(uint32_t addr, uint8_t *data, uint32_t data_size)
{
    int ret = SPIF_SUCCESS;

    uint8_t cmd[] = {SPIF_CMD_FAST_READ, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};

    if (s_spi_ops.ops_mode == SPIF_SPI_OPS_SPI) {
        ret = s_spi_ops.ops.spi.spi_transfer(cmd, SPIF_ARRAY_SIZE(cmd), data, data_size);
    } else if (s_spi_ops.ops_mode == SPIF_SPI_OPS_QSPI) {
        ret = s_spi_ops.ops.qspi.qspi_transfer(cmd[0], addr, NULL, 0, data, data_size);
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

    for (uint16_t i = 0; i < SPIF_ARRAY_SIZE(s_spif_flash_info); i++) {
        if ((buf[0] == s_spif_flash_info[i].mf_id) &&
            (buf[1] == s_spif_flash_info[i].mt_id) &&
            (buf[2] == s_spif_flash_info[i].cap_id)) {
            SPIF_INFO(TAG, "Flash: %s, Size: %d KB, Block: %d KB, Sector: %d KB, Page: %d B.",
                      s_spif_flash_info[i].name,
                      s_spif_flash_info[i].chip_size / 1024,
                      s_spif_flash_info[i].block_size / 1024,
                      s_spif_flash_info[i].sector_size / 1024,
                      s_spif_flash_info[i].page_size);
            s_spif_flash_index = i;
            break;
        }
    }

    return ret;
}

void spif_page_test(uint32_t page_addr)
{
    int ret = SPIF_SUCCESS;

    uint8_t page_tx_buf[256] = {0};
    uint8_t page_rx_buf[256] = {0};

    uint32_t sector_addr = page_addr & (~0xFFF); /* 4K sector */
    uint32_t block_addr = page_addr & (~0x7FFF); /* 32K block */

    SPIF_DEBUG(TAG, "Page Test 1: page: 0x%04X, sector: 0x%04X, block: 0x%04X\r\n", page_addr, sector_addr, block_addr);

    /* Test 1: normal page program */
    SPIF_DEBUG(TAG, "Test 1: normal page program\r\n");

    memset(page_tx_buf, 0x11, 256);
    SPIF_DEBUG(TAG, "page program data: 0x%2X", page_tx_buf[0]);
    ret = spif_page_program(page_addr, page_tx_buf, 256);
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "page program failed: %d.", ret);
        return;
    }

    memset(page_rx_buf, 0x00, 256);
    ret = spif_read(page_addr, page_rx_buf, 256);
    if (ret != SPIF_SUCCESS) {
        SPIF_ERROR(TAG, "page read failed: %d.", ret);
        return;
    }

    SPIF_DEBUG(TAG, "page[  0:3  ] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[0], page_rx_buf[1], page_rx_buf[2], page_rx_buf[3]);
    SPIF_DEBUG(TAG, "page[252:255] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[252], page_rx_buf[253], page_rx_buf[254], page_rx_buf[255]);

    /* Test 2: sector erase */
    SPIF_DEBUG(TAG, "Test 2: sector erase\r\n");

    spif_sector_erase(sector_addr);
    memset(page_rx_buf, 0x00, 256);
    spif_read(page_addr, page_rx_buf, 256);

    SPIF_DEBUG(TAG, "page[  0:3  ] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[0], page_rx_buf[1], page_rx_buf[2], page_rx_buf[3]);
    SPIF_DEBUG(TAG, "page[252:255] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[252], page_rx_buf[253], page_rx_buf[254], page_rx_buf[255]);

    /* Test 3: page program overrun 1 */
    SPIF_DEBUG(TAG, "Test 3: page program overrun 1\r\n");

    memset(page_tx_buf, 0x22, 18); /* 18 = 0x12 */
    SPIF_DEBUG(TAG, "page program data: 0x%2X", page_tx_buf[0]);
    spif_page_program(page_addr, page_tx_buf, 18);

    memset(page_rx_buf, 0x00, 18);
    spif_read(page_addr, page_rx_buf, 18);

    SPIF_DEBUG(TAG, "page[  0:3  ] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[0], page_rx_buf[1], page_rx_buf[2], page_rx_buf[3]);
    SPIF_DEBUG(TAG, "page[ 16:19 ] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[16], page_rx_buf[17], page_rx_buf[18], page_rx_buf[19]);
    SPIF_DEBUG(TAG, "page[252:255] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[252], page_rx_buf[253], page_rx_buf[254], page_rx_buf[255]);

    memset(page_tx_buf, 0x11, 256); /* 18 = 0x12 */
    SPIF_DEBUG(TAG, "page program data: 0x%2X", page_tx_buf[0]);
    spif_page_program(page_addr + 0x12, page_tx_buf, 256);

    memset(page_rx_buf, 0x00, 256);
    spif_read(page_addr, page_rx_buf, 256);

    SPIF_DEBUG(TAG, "page[  0:3  ] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[0], page_rx_buf[1], page_rx_buf[2], page_rx_buf[3]);
    SPIF_DEBUG(TAG, "page[ 16:19 ] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[16], page_rx_buf[17], page_rx_buf[18], page_rx_buf[19]);
    SPIF_DEBUG(TAG, "page[252:255] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[252], page_rx_buf[253], page_rx_buf[254], page_rx_buf[255]);

    /* Test 4: page program overrun 2 */
    SPIF_DEBUG(TAG, "Test 4: page program overrun 2\r\n");

    spif_sector_erase(sector_addr);

    memset(page_tx_buf, 0x11, 256);
    SPIF_DEBUG(TAG, "page program data: 0x%2X", page_tx_buf[0]);
    spif_page_program(page_addr, page_tx_buf, 256);

    memset(page_rx_buf, 0x00, 256);
    spif_read(page_addr, page_rx_buf, 256);

    SPIF_DEBUG(TAG, "page[  0:3  ] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[0], page_rx_buf[1], page_rx_buf[2], page_rx_buf[3]);
    SPIF_DEBUG(TAG, "page[252:255] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[252], page_rx_buf[253], page_rx_buf[254], page_rx_buf[255]);

    memset(page_tx_buf, 0x22, 256);
    SPIF_DEBUG(TAG, "page program data: 0x%2X", page_tx_buf[0]);
    spif_page_program(page_addr, page_tx_buf, 256);

    memset(page_rx_buf, 0x00, 256);
    spif_read(page_addr, page_rx_buf, 256);

    SPIF_DEBUG(TAG, "page[  0:3  ] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[0], page_rx_buf[1], page_rx_buf[2], page_rx_buf[3]);
    SPIF_DEBUG(TAG, "page[252:255] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[252], page_rx_buf[253], page_rx_buf[254], page_rx_buf[255]);

    /* Test 5: block erase */
    SPIF_DEBUG(TAG, "Test 5: block erase\r\n");

    spif_block_erase_32(block_addr);
    memset(page_rx_buf, 0x00, 256);
    spif_read(page_addr, page_rx_buf, 256);

    SPIF_DEBUG(TAG, "page[  0:3  ] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[0], page_rx_buf[1], page_rx_buf[2], page_rx_buf[3]);
    SPIF_DEBUG(TAG, "page[252:255] 0x%02X 0x%02X 0x%02X 0x%02X", page_rx_buf[252], page_rx_buf[253], page_rx_buf[254], page_rx_buf[255]);
}