/*
 * backtrace.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#include <stdio.h>
#include "stm32l4xx_hal.h"
#include "core_cm4.h"

void trigger_MemManage(void)
{   
    /* 从无效地址返回 */
    __asm volatile (
        "ldr r0, =0xdeadbeef \n"
        "bx r0"
    );
}

void trigger_BusFault(void)
{
    void (*func)(void) = NULL;
    func();
}

void trigger_UsageFault(void)
{
    volatile int a = 10;
    volatile int b = 0;
    volatile int result;

    /* 使能除零错误 */
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
    result = a / b;
}

void cust_expection_enable(int irq_type)
{
    /* HardFault 默认开启, 且优先级固定是 -1 */
    switch (irq_type) {
    case 4: /* MemManage */
        SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
        break;

    case 5: /* BusFault */
        SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
        break;

    case 6: /* UsageFault */
        SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
        break;

    default:
        printf("unknown irq type: %d.\n", irq_type);
        break;
    }
}

void cust_expection_handler(unsigned long *pc, unsigned long *lr, unsigned long *sp)
{
    xPSR_Type reg_xpsr;
    unsigned long reg_msp = 0;
    unsigned long reg_psp = 0;

    __asm volatile("mrs %0, msp"
                   : "=r"(reg_msp));
    __asm volatile("mrs %0, psp"
                   : "=r"(reg_psp));
    __asm volatile("mrs %0, xpsr"
                   : "=r"(reg_xpsr));

    /**
     * SCB->CFSR
     *  31          16 15       8 7      0
     * +--------------+----------+--------+
     * |     UFSR     |   BFSR   |  MFSR  |
     * +--------------+----------+--------+
     */
    switch (reg_xpsr.b.ISR) {
    case 3: /* HardFault */
        printf("HardFault,        HFSR : 0x%.8x.\n", SCB->HFSR);
        printf("Maybe MemManage,  MMFAR: 0x%.8x.\n", SCB->MMFAR);
        printf("Maybe BusFault,   BFAR : 0x%.8x.\n", SCB->BFAR);
        printf("Maybe UsageFault, CFSR : 0x%.8x.\n", SCB->CFSR);
        break;

    case 4: /* MemManage */
        printf("MemManage, MMFAR: 0x%.8x, CFSR: 0x%.8x.\n", SCB->MMFAR, SCB->CFSR);
        break;

    case 5: /* BusFault */
        printf("BusFault, BFAR: 0x%.8x, CFSR: 0x%.8x.\n", SCB->BFAR, SCB->CFSR);
        break;

    case 6: /* UsageFault */
        printf("UsageFault, CFSR: 0x%.8x.\n", SCB->CFSR);
        break;

    default:
        printf("unknown IRQ: %d.\n", reg_xpsr.b.ISR);
        break;
    }

    printf("MSP : 0x%x, PSP: 0x%x, SP: %p.\n", reg_msp, reg_psp, sp);
    printf("PC  : %p.\n", pc);
    printf("LR  : %p.\n", lr);

    /**
     * cortex-m4 是满减栈, 高地址是栈顶, 低地址是栈底
     * (unsigned long *)sp + 1 => sp 位移 4 字节
     */
    printf("R0  : 0x%.8x.\n", *(sp + 3));
    printf("R1  : 0x%.8x.\n", *(sp + 2));
    printf("R2  : 0x%.8x.\n", *(sp + 1));
    printf("R3  : 0x%.8x.\n", *(sp + 0));

    while (1) {
    }
}
