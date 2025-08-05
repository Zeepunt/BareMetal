/*
 * Copyright (c) 2025 by Zeepunt, All Rights Reserved. 
 */
#include <stdio.h>

#include "stm32l4xx_hal.h"
#include "bsp_atk_pandora.h"

int main(void)
{
    HAL_Init();

    bsp_clock_init();
    bsp_uart_init(115200);
    bsp_button_init();

    printf("Hello world.\r\n");

    while (1) {
        
    }

    return 0;
}