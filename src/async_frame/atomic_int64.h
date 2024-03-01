/*
 * =====================================================================================
 *
 *       Filename:  atomic_int64.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  11/09/2010 12:05:18 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __ATOMIC_INT64_H__
#define __ATOMIC_INT64_H__

#include <stdint.h>
#include "L5Def.h"

BEGIN_L5MODULE_NS

#if 0
static inline uint64_t __mmx_readq(volatile uint64_t *v)
{
    uint64_t r;
    __asm__ __volatile__ (
            "movq (%1), %0\n\t"
            :"=y"(r) :"r"(v));
    __asm__ __volatile__ (
            "emms\n\t"
            : :);
    return r;
}

static inline void __mmx_writeq(volatile uint64_t *v, uint64_t r)
{
    __asm__ __volatile__ (
            "movq %0, (%1)\n\t"
            : : "y"(r), "r"(v)
            );
    __asm__ __volatile__ (
            "emms\n\t"
            : :);
}

#else
static inline uint64_t __mmx_readq(volatile uint64_t *v)
{
    return *v;
}
static inline void __mmx_writeq(volatile uint64_t *v, uint64_t r)
{
    *v = r;
}

#endif

END_L5MODULE_NS

#endif
