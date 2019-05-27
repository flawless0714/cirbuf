/*
 * TODOs:
 * 1. We should fix some functions inside `cirbuf.h` to having return value 
 *    themselves which indicating if the function is succeeded. E.g.
 *    `cirbuf_new`, if we trying to mmap a memory which is too large to map
 *    , it should instead return a false-like return value to indicate that
 *    the function has failed, rather than calling exit(-1) to exit the
 *    test directly. This is bad when running unit test.
 * 2. We should call perror from test suite, instead of calling from the API. That
 *    cause some performance penalty and output error log to stdout, which user
 *    may not happy to see this, because the failure is informed through the
 *    return value, user knew about it themself.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unit-test.h"
#include "cirbuf.h"

void TestCirbuf_set_size_with_init(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536u);
    CuAssertTrue(tc, 65536u == cirbuf_size(&cb));
}

void TestCirbuf_is_empty_after_init(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    CuAssertTrue(tc, cirbuf_is_empty(&cb));
}

void TestCirbuf_is_not_empty_after_offer(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    CuAssertTrue(tc, !cirbuf_is_empty(&cb));
}

void TestCirbuf_is_empty_after_poll_release(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    cirbuf_poll(&cb, 4);
    CuAssertTrue(tc, cirbuf_is_empty(&cb));
}

void TestCirbuf_spaceused_is_zero_after_poll_release(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    CuAssertTrue(tc, 4 == cirbuf_usedspace(&cb));
    cirbuf_poll(&cb, 4);
    CuAssertTrue(tc, !cirbuf_usedspace(&cb));
}

void TestCirbuf_cant_offer_if_not_enough_space(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    CuAssertTrue(tc, !cirbuf_offer(&cb, (unsigned char *) "1000", 1 << 17));
}

void TestCirbuf_cant_offer_if_buffer_will_be_completely_full(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    CuAssertTrue(tc, !cirbuf_offer(&cb, (unsigned char *) "1000", 1 << 16));
}

void TestCirbuf_offer_and_poll(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    CuAssertTrue(tc, !strncmp("abcd", (char *) cirbuf_poll(&cb, 4), 4));
}

void TestCirbuf_cant_poll_nonexistant(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    CuAssertTrue(tc, NULL == cirbuf_poll(&cb, 4));
}

void TestCirbuf_cant_poll_twice_when_released(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    cirbuf_offer(&cb, (unsigned char *) "1000", 4);
    cirbuf_poll(&cb, 4);
    cirbuf_poll(&cb, 4);
    CuAssertTrue(tc, NULL == cirbuf_poll(&cb, 4));
}

void TestCirbuf_independant_of_each_other(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    cirbuf_t cb2;
    cirbuf_new(&cb2, 65536);
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    cirbuf_offer(&cb2, (unsigned char *) "efgh", 4);
    CuAssertTrue(tc, !strncmp("abcd", (char *) cirbuf_poll(&cb, 4), 4));
    CuAssertTrue(tc, !strncmp("efgh", (char *) cirbuf_poll(&cb2, 4), 4));
}

void TestCirbuf_independant_of_each_other_with_no_polling(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536);
    cirbuf_t cb2;
    cirbuf_new(&cb2, 65536);
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    cirbuf_offer(&cb2, (unsigned char *) "efgh", 4);
    CuAssertTrue(tc, !strncmp("abcd", (char *) cirbuf_peek(&cb), 4));
    CuAssertTrue(tc, !strncmp("efgh", (char *) cirbuf_peek(&cb2), 4));
}
