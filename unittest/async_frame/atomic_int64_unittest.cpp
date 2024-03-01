/*
 * =====================================================================================
 *
 *       Filename:  atomic_int64_unittest.cpp
 *
 *    Description:  atomic_int64.h单元测试
 *
 *        Version:  1.0
 *        Created:  11/23/2010 03:08:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ericsha (shakaibo), ericsha@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <async_frame/atomic_int64.h>

USING_L5MODULE_NS;

/**
 * 原子性读取64位整型测试
 *
 * __mmx_readq参数必定不为NULL，由上层调用保证
 *
 */
TEST(AtomicInt64, __mmx_readq) {

    uint64_t v , r;

    v = 0;
    r = __mmx_readq(&v);
    ASSERT_EQ(v, r);

    v = 9999;
    r = __mmx_readq(&v);
    ASSERT_EQ(v, r);

    v = 0xFFFFFFF;
    r = __mmx_readq(&v);
    ASSERT_EQ(v, r);

    v = 0xFFFFFF;
    r = __mmx_readq(&v);
    ASSERT_EQ(v, r);
}

/**
 * 原子性写64位整型测试
 *
 * __mmx_writeq参数v必定不为NULL，由上层调用保证
 *
 */
TEST(AtomicInt64, __mmx_writeq) {

    uint64_t v , r;

    r = 0;
    __mmx_writeq(&v, r);
    ASSERT_EQ(v, r);

    r = 9999;
    __mmx_writeq(&v, r);
    ASSERT_EQ(v, r);

    r = 0xFFFFFFF;
    __mmx_writeq(&v, r);
    ASSERT_EQ(v, r);

    r = 0xFFFFFF;
    __mmx_writeq(&v, r);
    ASSERT_EQ(v, r);

}
