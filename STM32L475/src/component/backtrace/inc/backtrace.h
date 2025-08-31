/*
 * backtrace.h
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 Zeepunt
 */
#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__

void cust_expection_enable(int irq_type);

void cust_expection_handler(unsigned long *pc, unsigned long *lr, unsigned long *sp);

#endif /* __BACKTRACE_H__ */
