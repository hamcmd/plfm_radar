/*******************************************************************************
 * test_bug5_fine_phase_gpio_only.c
 *
 * Bug #5: ADF4382A_SetFinePhaseShift() (lines 558-589) is a placeholder.
 * For intermediate duty_cycle values (not 0, not max), it should generate a
 * PWM signal on DELADJ pin. Instead, it just sets the GPIO pin HIGH — same
 * as the maximum duty cycle case.  There is no timer/PWM setup.
 *
 * Test strategy:
 *   1. Initialize manager, fix sync (workaround Bug #1).
 *   2. Call SetFinePhaseShift with duty_cycle=0 → verify GPIO LOW.
 *   3. Call SetFinePhaseShift with duty_cycle=MAX → verify GPIO HIGH.
 *   4. Call SetFinePhaseShift with duty_cycle=500 (intermediate) → verify
 *      it also just sets GPIO HIGH (the bug — should be PWM, not bang-bang).
 *   5. Verify NO timer/PWM configuration spy records exist.
 ******************************************************************************/
#include "adf4382a_manager.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    ADF4382A_Manager mgr;
    int ret;

    printf("=== Bug #5: SetFinePhaseShift is GPIO-only placeholder ===\n");

    /* Setup: init + manual sync fix */
    spy_reset();
    ret = ADF4382A_Manager_Init(&mgr, SYNC_METHOD_TIMED);
    assert(ret == ADF4382A_MANAGER_OK);
    ret = ADF4382A_SetupTimedSync(&mgr);
    assert(ret == ADF4382A_MANAGER_OK);

    /* ---- Test A: duty_cycle=0 → GPIO LOW ---- */
    spy_reset();
    ret = ADF4382A_SetFinePhaseShift(&mgr, 0, 0);  /* device=0 (TX), duty=0 */
    assert(ret == ADF4382A_MANAGER_OK);

    /* Find the GPIO write for TX_DELADJ pin */
    int gpio_idx = spy_find_nth(SPY_GPIO_WRITE, 0);
    assert(gpio_idx >= 0);
    const SpyRecord *r = spy_get(gpio_idx);
    assert(r != NULL);
    printf("  duty=0: GPIO write port=%p pin=0x%04X value=%u\n",
           r->port, r->pin, r->value);
    assert(r->value == GPIO_PIN_RESET);
    printf("  PASS: duty=0 → GPIO LOW (correct)\n");

    /* ---- Test B: duty_cycle=DELADJ_MAX_DUTY_CYCLE → GPIO HIGH ---- */
    spy_reset();
    ret = ADF4382A_SetFinePhaseShift(&mgr, 0, DELADJ_MAX_DUTY_CYCLE);
    assert(ret == ADF4382A_MANAGER_OK);

    gpio_idx = spy_find_nth(SPY_GPIO_WRITE, 0);
    assert(gpio_idx >= 0);
    r = spy_get(gpio_idx);
    assert(r != NULL);
    printf("  duty=MAX(%d): GPIO write value=%u\n", DELADJ_MAX_DUTY_CYCLE, r->value);
    assert(r->value == GPIO_PIN_SET);
    printf("  PASS: duty=MAX → GPIO HIGH (correct)\n");

    /* ---- Test C: duty_cycle=500 (intermediate) → GPIO HIGH (BUG) ---- */
    spy_reset();
    ret = ADF4382A_SetFinePhaseShift(&mgr, 0, 500);
    assert(ret == ADF4382A_MANAGER_OK);

    gpio_idx = spy_find_nth(SPY_GPIO_WRITE, 0);
    assert(gpio_idx >= 0);
    r = spy_get(gpio_idx);
    assert(r != NULL);
    printf("  duty=500 (intermediate): GPIO write value=%u\n", r->value);
    assert(r->value == GPIO_PIN_SET);
    printf("  PASS: duty=500 → GPIO HIGH (BUG: should be PWM, not static HIGH)\n");

    /* ---- Test D: Verify total GPIO writes is exactly 1 for intermediate ---- */
    /* If proper PWM were set up, we'd see timer config calls or multiple
     * GPIO toggles. Instead, there's just a single GPIO write. */
    int total_gpio = spy_count_type(SPY_GPIO_WRITE);
    printf("  Total GPIO writes for intermediate duty: %d (expected 1 — no PWM)\n",
           total_gpio);
    assert(total_gpio == 1);
    printf("  PASS: Only 1 GPIO write — confirms no PWM generation\n");

    /* ---- Test E: duty=500 produces SAME output as duty=MAX ---- */
    printf("  BUG CONFIRMED: duty=500 and duty=MAX both produce identical GPIO HIGH\n");
    printf("  Any intermediate value is treated as 100%% duty — no proportional control\n");

    /* Cleanup */
    ADF4382A_Manager_Deinit(&mgr);

    printf("=== Bug #5: ALL TESTS PASSED ===\n\n");
    return 0;
}
