/*
 * bsp_atk_pandora.h
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#ifndef __BSP_ATK_PANDORA_H__
#define __BSP_ATK_PANDORA_H__

#include <stdint.h>

void bsp_uart_init(uint32_t baudrate);

void bsp_clock_init(void);

void bsp_button_init(void);

void bsp_systick_init(void);

void bsp_delay_us(uint32_t us);

#endif /* __BSP_ATK_PANDORA_H__ */
