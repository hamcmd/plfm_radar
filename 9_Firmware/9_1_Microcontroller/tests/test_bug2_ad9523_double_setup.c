/*******************************************************************************
 * test_bug2_ad9523_double_setup.c
 *
 * Bug #2: configure_ad9523() in main.cpp calls ad9523_setup() twice:
 *   - Line 1141: BEFORE AD9523_RESET_RELEASE() (chip still in reset)
 *   - Line 1159: AFTER reset release (the real configuration)
 *
 * We can't compile main.cpp directly, so we extract the bug pattern
 * and replay the exact sequence against our mocks to prove the double call.
 *
 * Test strategy:
 *   1. Replay the configure_ad9523() call sequence.
 *   2. Verify ad9523_setup() is called twice in the spy log.
 *   3. Verify the reset-release GPIO write (GPIOF, AD9523_RESET_Pin=SET)
 *      occurs BETWEEN the two setup calls.
 *   4. This proves the first call writes to a chip in reset.
 ******************************************************************************/
#include "stm32_hal_mock.h"
#include "ad_driver_mock.h"
#include <assert.h>
#include <stdio.h>

/* Pin defines from main.h shim */
#define AD9523_RESET_Pin        GPIO_PIN_6
#define AD9523_RESET_GPIO_Port  GPIOF
#define AD9523_REF_SEL_Pin      GPIO_PIN_4
#define AD9523_REF_SEL_GPIO_Port GPIOF

/* Macro from main.cpp */
#define AD9523_RESET_RELEASE() HAL_GPIO_WritePin(AD9523_RESET_GPIO_Port, AD9523_RESET_Pin, GPIO_PIN_SET)
#define AD9523_REF_SEL(x)     HAL_GPIO_WritePin(AD9523_REF_SEL_GPIO_Port, AD9523_REF_SEL_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/*
 * Extracted from main.cpp lines ~1130-1184.
 * This reproduces the exact call sequence with minimal setup.
 */
static int configure_ad9523_extracted(void)
{
    struct ad9523_dev *dev = NULL;
    struct ad9523_platform_data pdata;
    struct ad9523_init_param init_param;
    int32_t ret;

    /* Minimal pdata setup — details don't matter for this test */
    memset(&pdata, 0, sizeof(pdata));
    pdata.vcxo_freq = 100000000;
    pdata.num_channels = 0;
    pdata.channels = NULL;

    memset(&init_param, 0, sizeof(init_param));
    init_param.pdata = &pdata;

    /* Step 1: ad9523_init (fills defaults) */
    ad9523_init(&init_param);

    /* Step 2: FIRST ad9523_setup() — chip is still in reset!
     * This is the bug — line 1141 */
    ret = ad9523_setup(&dev, &init_param);

    /* Step 3: Release reset — line 1148 */
    AD9523_RESET_RELEASE();
    HAL_Delay(5);

    /* Step 4: Select REFB */
    AD9523_REF_SEL(true);

    /* Step 5: SECOND ad9523_setup() — post-reset, real config — line 1159 */
    ret = ad9523_setup(&dev, &init_param);
    if (ret != 0) return -1;

    /* Step 6: status + sync */
    ad9523_status(dev);
    ad9523_sync(dev);

    return 0;
}

int main(void)
{
    printf("=== Bug #2: AD9523 double setup call ===\n");

    spy_reset();
    int ret = configure_ad9523_extracted();
    assert(ret == 0);

    /* ---- Test A: ad9523_setup was called exactly twice ---- */
    int setup_count = spy_count_type(SPY_AD9523_SETUP);
    printf("  SPY_AD9523_SETUP records: %d (expected 2)\n", setup_count);
    assert(setup_count == 2);
    printf("  PASS: ad9523_setup() called twice\n");

    /* ---- Test B: Reset release GPIO write occurs BETWEEN the two setups ---- */
    int first_setup_idx  = spy_find_nth(SPY_AD9523_SETUP, 0);
    int second_setup_idx = spy_find_nth(SPY_AD9523_SETUP, 1);

    printf("  First setup at spy index %d, second at %d\n",
           first_setup_idx, second_setup_idx);

    /* Find the GPIO write for GPIOF, AD9523_RESET_Pin, SET between them */
    int reset_gpio_idx = -1;
    for (int i = first_setup_idx + 1; i < second_setup_idx; i++) {
        const SpyRecord *r = spy_get(i);
        if (r && r->type == SPY_GPIO_WRITE &&
            r->port == GPIOF &&
            r->pin == AD9523_RESET_Pin &&
            r->value == GPIO_PIN_SET) {
            reset_gpio_idx = i;
            break;
        }
    }

    printf("  Reset release GPIO write at spy index %d (expected between %d and %d)\n",
           reset_gpio_idx, first_setup_idx, second_setup_idx);
    assert(reset_gpio_idx > first_setup_idx);
    assert(reset_gpio_idx < second_setup_idx);
    printf("  PASS: First setup BEFORE reset release, second setup AFTER\n");
    printf("  This proves the first ad9523_setup() writes to a chip still in reset\n");

    printf("=== Bug #2: ALL TESTS PASSED ===\n\n");
    return 0;
}
