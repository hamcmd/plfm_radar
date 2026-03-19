/*******************************************************************************
 * test_bug3_timed_sync_noop.c
 *
 * Bug #3: ADF4382A_TriggerTimedSync() (lines 282-303) is a no-op — it only
 * prints messages but performs NO hardware action (no register writes, no GPIO
 * pulses, no SPI transactions).
 *
 * Test strategy:
 *   1. Initialize manager with SYNC_METHOD_TIMED, manually fix the sync setup
 *      (call SetupTimedSync after init so it actually works).
 *   2. Reset spy log.
 *   3. Call ADF4382A_TriggerTimedSync().
 *   4. Verify it returns OK.
 *   5. Count all hardware-related spy records (GPIO writes, SPI writes,
 *      ADF4382 driver calls). Expect ZERO.
 *   6. Compare with ADF4382A_TriggerEZSync() which actually does 4 SPI calls
 *      (set_sw_sync true/false for TX and RX).
 ******************************************************************************/
#include "adf4382a_manager.h"
#include <assert.h>
#include <stdio.h>

/* Count all hardware-action spy records (everything except tick/delay reads) */
static int count_hardware_actions(void)
{
    int hw_count = 0;
    for (int i = 0; i < spy_count; i++) {
        const SpyRecord *r = spy_get(i);
        if (!r) continue;
        switch (r->type) {
            case SPY_GPIO_WRITE:
            case SPY_GPIO_TOGGLE:
            case SPY_ADF4382_SET_TIMED_SYNC:
            case SPY_ADF4382_SET_EZSYNC:
            case SPY_ADF4382_SET_SW_SYNC:
            case SPY_ADF4382_SPI_READ:
            case SPY_ADF4382_SET_OUT_POWER:
            case SPY_ADF4382_SET_EN_CHAN:
            case SPY_AD9523_SETUP:
            case SPY_AD9523_SYNC:
                hw_count++;
                break;
            default:
                break;
        }
    }
    return hw_count;
}

int main(void)
{
    ADF4382A_Manager mgr;
    int ret;

    printf("=== Bug #3: TriggerTimedSync is a no-op ===\n");

    /* Setup: init the manager, then manually fix sync (workaround Bug #1) */
    spy_reset();
    ret = ADF4382A_Manager_Init(&mgr, SYNC_METHOD_TIMED);
    assert(ret == ADF4382A_MANAGER_OK);

    /* Manually call SetupTimedSync now that initialized==true */
    ret = ADF4382A_SetupTimedSync(&mgr);
    assert(ret == ADF4382A_MANAGER_OK);

    /* ---- Test A: TriggerTimedSync produces zero hardware actions ---- */
    spy_reset();  /* Clear all prior spy records */
    ret = ADF4382A_TriggerTimedSync(&mgr);
    printf("  TriggerTimedSync returned: %d (expected 0=OK)\n", ret);
    assert(ret == ADF4382A_MANAGER_OK);

    int hw_actions = count_hardware_actions();
    printf("  Hardware action spy records: %d (expected 0)\n", hw_actions);
    assert(hw_actions == 0);
    printf("  PASS: TriggerTimedSync does absolutely nothing to hardware\n");

    /* ---- Test B: For comparison, TriggerEZSync DOES hardware actions ---- */
    /* Reconfigure to EZSYNC for comparison */
    mgr.sync_method = SYNC_METHOD_EZSYNC;
    spy_reset();
    ret = ADF4382A_TriggerEZSync(&mgr);
    printf("  TriggerEZSync returned: %d (expected 0=OK)\n", ret);
    assert(ret == ADF4382A_MANAGER_OK);

    int ezsync_sw_sync_count = spy_count_type(SPY_ADF4382_SET_SW_SYNC);
    printf("  SPY_ADF4382_SET_SW_SYNC records from EZSync: %d (expected 4)\n",
           ezsync_sw_sync_count);
    assert(ezsync_sw_sync_count == 4);  /* TX set, RX set, TX clear, RX clear */
    printf("  PASS: EZSync performs 4 SPI calls, TimedSync performs 0\n");

    /* Cleanup */
    mgr.sync_method = SYNC_METHOD_TIMED;  /* restore for deinit */
    ADF4382A_Manager_Deinit(&mgr);

    printf("=== Bug #3: ALL TESTS PASSED ===\n\n");
    return 0;
}
