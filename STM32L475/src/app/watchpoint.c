/*
 * watchpoint.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#include <string.h>

#include "stm32l475xx.h" /* core_m4.h is included in stm32l475xx.h */

#include "watchpoint.h"

/* COMP: Comparator Register */
#define WP_COMP_NUM    4

typedef struct watchpoint_info_s {
    wp_point_t point;
    uint8_t used;
} wp_info_t;

/* CoreSight */
static CoreDebug_Type *s_core_debug = CoreDebug;
/* ITM */
static ITM_Type *s_itm = ITM;
/* DWT */
static DWT_Type *s_dwt = DWT;

static wp_info_t s_wp_info[WP_COMP_NUM] = {0};

void watchpoint_init(void)
{
    /* TRCENA: bit[24] = 1 */
    s_core_debug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* DWTENA: bit[3] = 1 */
    s_itm->TCR |= ITM_TCR_DWTENA_Msk;

    WP_TRACE("watchpoint init success.");
}

void watchpoint_deinit(void)
{
    /* DWTENA: bit[3] = 0 */
    s_itm->TCR &= ~ITM_TCR_DWTENA_Msk;

    /* TRCENA: bit[24] = 0 */
    s_core_debug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
}

void watchpoint_start(void)
{
    /* MON_EN: bit[16] = 0 */
    s_core_debug->DEMCR |= CoreDebug_DEMCR_MON_EN_Msk;

    NVIC_SetPriority(DebugMonitor_IRQn, 0);
    NVIC_EnableIRQ(DebugMonitor_IRQn);

    WP_TRACE("watchpoint start success.");
}

void watchpoint_stop(void)
{
    NVIC_DisableIRQ(DebugMonitor_IRQn);

    s_core_debug->DEMCR &= ~CoreDebug_DEMCR_MON_EN_Msk;
}

int watchpoint_add_point(wp_point_t *point)
{
    uint8_t idx = 0;

    if (NULL == point) {
        return -1;
    }

    for (idx = 0; idx < WP_COMP_NUM; idx++) {
        if (s_wp_info[idx].used == 0) {
            break;
        }
    }

    if (idx >= WP_COMP_NUM) {
        return -1;
    }

    memcpy((void *)&s_wp_info[idx].point, (void *)point, sizeof(wp_point_t));
    s_wp_info[idx].used = 1;

    switch (idx) {
    case 0: {
        s_dwt->COMP0 = point->addr;

        if (point->addr_type == WP_ADDR_TYPE_BYTE) {
            s_dwt->MASK0 = 0;
        } else if (point->addr_type == WP_ADDR_TYPE_HALFWORD) {
            s_dwt->MASK0 = 1;
        } else if (point->addr_type == WP_ADDR_TYPE_WORD) {
            s_dwt->MASK0 = 2;
        }

        if (point->access_type == WP_ACCESS_TYPE_RO) {
            s_dwt->FUNCTION0 = (5 << DWT_FUNCTION_FUNCTION_Pos);
        } else if (point->access_type == WP_ACCESS_TYPE_WO) {
            s_dwt->FUNCTION0 = (6 << DWT_FUNCTION_FUNCTION_Pos);
        } else if (point->access_type == WP_ACCESS_TYPE_RW) {
            s_dwt->FUNCTION0 = (7 << DWT_FUNCTION_FUNCTION_Pos);
        }

        break;
    }

    case 1: {
        s_dwt->COMP1 = point->addr;

        if (point->addr_type == WP_ADDR_TYPE_BYTE) {
            s_dwt->MASK1 = 0;
        } else if (point->addr_type == WP_ADDR_TYPE_HALFWORD) {
            s_dwt->MASK1 = 1;
        } else if (point->addr_type == WP_ADDR_TYPE_WORD) {
            s_dwt->MASK1 = 2;
        }

        if (point->access_type == WP_ACCESS_TYPE_RO) {
            s_dwt->FUNCTION1 = (5 << DWT_FUNCTION_FUNCTION_Pos);
        } else if (point->access_type == WP_ACCESS_TYPE_WO) {
            s_dwt->FUNCTION1 = (6 << DWT_FUNCTION_FUNCTION_Pos);
        } else if (point->access_type == WP_ACCESS_TYPE_RW) {
            s_dwt->FUNCTION1 = (7 << DWT_FUNCTION_FUNCTION_Pos);
        }

        break;
    }

    case 2: {
        s_dwt->COMP2 = point->addr;

        if (point->addr_type == WP_ADDR_TYPE_BYTE) {
            s_dwt->MASK2 = 0;
        } else if (point->addr_type == WP_ADDR_TYPE_HALFWORD) {
            s_dwt->MASK2 = 1;
        } else if (point->addr_type == WP_ADDR_TYPE_WORD) {
            s_dwt->MASK2 = 2;
        }

        if (point->access_type == WP_ACCESS_TYPE_RO) {
            s_dwt->FUNCTION2 = (5 << DWT_FUNCTION_FUNCTION_Pos);
        } else if (point->access_type == WP_ACCESS_TYPE_WO) {
            s_dwt->FUNCTION2 = (6 << DWT_FUNCTION_FUNCTION_Pos);
        } else if (point->access_type == WP_ACCESS_TYPE_RW) {
            s_dwt->FUNCTION2 = (7 << DWT_FUNCTION_FUNCTION_Pos);
        }

        break;
    }

    case 3: {
        s_dwt->COMP3 = point->addr;

        if (point->addr_type == WP_ADDR_TYPE_BYTE) {
            s_dwt->MASK3 = 0;
        } else if (point->addr_type == WP_ADDR_TYPE_HALFWORD) {
            s_dwt->MASK3 = 1;
        } else if (point->addr_type == WP_ADDR_TYPE_WORD) {
            s_dwt->MASK3 = 2;
        }

        if (point->access_type == WP_ACCESS_TYPE_RO) {
            s_dwt->FUNCTION3 = (5 << DWT_FUNCTION_FUNCTION_Pos);
        } else if (point->access_type == WP_ACCESS_TYPE_WO) {
            s_dwt->FUNCTION3 = (6 << DWT_FUNCTION_FUNCTION_Pos);
        } else if (point->access_type == WP_ACCESS_TYPE_RW) {
            s_dwt->FUNCTION3 = (7 << DWT_FUNCTION_FUNCTION_Pos);
        }

        break;
    }

    default:
        return -1;
    }

    WP_TRACE("watchpoint add %d success.", idx);

    return 0;
}

int watchpoint_del_point(wp_point_t *point)
{
    uint8_t idx = 0;

    if (NULL == point) {
        return -1;
    }

    for (idx = 0; idx < WP_COMP_NUM; idx++) {
        if ((s_wp_info[idx].used == 1) && (s_wp_info[idx].point.addr == point->addr)) {
            break;
        }
    }

    if (idx >= WP_COMP_NUM) {
        return -1;
    }

    memset((void *)&s_wp_info[idx].point, 0, sizeof(wp_point_t));
    s_wp_info[idx].used = 0;

    return 0;
}

/**
 * @brief 中断处理函数, 函数名称参考中断向量表
 */
void wp_exception_handler(uint32_t *pc, uint32_t *lr, uint32_t *sp)
{
    printf("=== DebugMon_Handler ===\r\n");
    printf("MSP : 0x%x, PSP: 0x%x, SP:%p\r\n", __get_MSP(), __get_PSP(), sp);
    
    printf("PC  : %p\r\n", pc);
    printf("LR  : %p\r\n", lr);

    printf("R0  : 0x%.8x\r\n", *(sp + 3));
    printf("R1  : 0x%.8x\r\n", *(sp + 2));
    printf("R2  : 0x%.8x\r\n", *(sp + 1));
    printf("R3  : 0x%.8x\r\n", *(sp + 0));

    while (1);
}
