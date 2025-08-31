/*
 * main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#include <stdio.h>

#include "stm32l4xx_hal.h"
#include "bsp_atk_pandora.h"

#include "spif.h"

int main(void)
{
    HAL_Init();

    bsp_clock_init();
    bsp_systick_init();
    bsp_uart_init(115200);
    bsp_button_init();

    spif_init();

    while (1) {
        bsp_delay_us(1);
        // printf("Hello world.\r\n");
    }

    return 0;
}