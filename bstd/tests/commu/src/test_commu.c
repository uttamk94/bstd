#include <zephyr/ztest.h>

ZTEST(test_commu, test_case1)
{
    zassert_true(true, "This test case should always pass");
}

ZTEST_SUITE(test_commu, NULL, NULL, NULL, NULL, NULL);