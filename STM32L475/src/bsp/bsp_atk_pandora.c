/*
 * Copyright (c) 2025 by Zeepunt, All Rights Reserved. 
 */
#include <stdio.h>

#include "stm32l4xx_hal.h"
#include "stm32l475xx.h"

#include "bsp_atk_pandora.h"

#define UART_RX_BUF_LEN 256

static uint8_t s_uart_rx_buf[UART_RX_BUF_LEN];
static uint16_t s_uart_rx_status = 0;
static UART_HandleTypeDef s_uart1_handler;

#if !defined(__MICROLIB)
/* 告知链接器不从 C 库链接, 使用半主机的函数 */
__asm (".global __use_no_semihosting\n\t");

/* 定义 _sys_exit() 以避免使用半主机模式 */
void _sys_exit(int x)
{
    x = x;
}

/* __use_no_semihosting requested */
void _ttywrch(int ch)
{
    ch = ch;
}

FILE __stdout;
#endif

#if defined ( __GNUC__ ) && !defined (__clang__) 
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif 
PUTCHAR_PROTOTYPE
{
    while((USART1->ISR & 0X40) == 0);

    USART1->TDR = (uint8_t) ch;
    return ch;
}

void USART1_IRQHandler(void)
{
    uint8_t recv;

    if ((__HAL_UART_GET_FLAG(&s_uart1_handler, UART_FLAG_RXNE) != RESET)) {
        HAL_UART_Receive(&s_uart1_handler, &recv, 1, 1000);

        if ((s_uart_rx_status & 0x8000) == 0) {
            if (s_uart_rx_status & 0x4000) {
                if (0x0a != recv) {
                    s_uart_rx_status = 0;
                } else {
                    s_uart_rx_status |= 0x8000;
                }
            } else {
                if (0x0d == recv) {
                    s_uart_rx_status |= 0x4000;
                } else {
                    s_uart_rx_buf[s_uart_rx_status & 0X3FFF] = recv;
                    s_uart_rx_status++;

                    if (s_uart_rx_status > (UART_RX_BUF_LEN - 1)) {
                        s_uart_rx_status = 0;
                    }
                }
            }
        }
    }
    HAL_UART_IRQHandler(&s_uart1_handler);
}

void bsp_uart_init(uint32_t baudrate)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_Initure;

    GPIO_Initure.Pin = GPIO_PIN_9;
    GPIO_Initure.Mode = GPIO_MODE_AF_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FAST;
    GPIO_Initure.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_Initure);

    GPIO_Initure.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOA, &GPIO_Initure);

    s_uart1_handler.Instance = USART1;
    s_uart1_handler.Init.BaudRate = baudrate;
    s_uart1_handler.Init.WordLength = UART_WORDLENGTH_8B;
    s_uart1_handler.Init.StopBits = UART_STOPBITS_1;
    s_uart1_handler.Init.Parity = UART_PARITY_NONE;
    s_uart1_handler.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    s_uart1_handler.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&s_uart1_handler);

    __HAL_UART_ENABLE_IT(&s_uart1_handler, UART_IT_RXNE);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_NVIC_SetPriority(USART1_IRQn, 3, 3);
}

void bsp_clock_init(void)
{
    HAL_StatusTypeDef ret = HAL_OK;

    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    __HAL_RCC_PWR_CLK_ENABLE();

    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 20;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);

    if (HAL_OK != ret) {
        while (1);
    }

    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);

    if (HAL_OK != ret) {
        while (1);
    }

    /* Configure the main internal regulator output voltage */
    ret = HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    if (HAL_OK != ret) {
        while (1);
    }
}