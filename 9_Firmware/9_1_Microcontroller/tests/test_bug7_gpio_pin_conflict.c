/*******************************************************************************
 * test_bug7_gpio_pin_conflict.c
 *
 * Bug #7: adf4382a_manager.h defines GPIOG pins 0-9 for ADF4382 signals,
 * but main.h (CubeMX-generated) assigns:
 *   - GPIOG pins 0-5 → PA enables + clock enables
 *   - GPIOG pins 6-15 → ADF4382 signals (DIFFERENT pin assignments)
 *
 * The .c file uses the manager.h pin mapping, meaning it will toggle PA
 * enables and clock enables instead of the intended ADF4382 pins.
 *
 * Example conflicts:
 *   manager.h: TX_CE_Pin = GPIO_PIN_0       main.h: EN_P_5V0_PA1_Pin = GPIO_PIN_0
 *   manager.h: TX_CS_Pin = GPIO_PIN_1       main.h: EN_P_5V0_PA2_Pin = GPIO_PIN_1
 *   manager.h: TX_DELADJ_Pin = GPIO_PIN_2   main.h: EN_P_5V0_PA3_Pin = GPIO_PIN_2
 *   manager.h: TX_DELSTR_Pin = GPIO_PIN_3   main.h: EN_P_5V5_PA_Pin  = GPIO_PIN_3
 *   manager.h: TX_LKDET_Pin = GPIO_PIN_4    main.h: EN_P_1V8_CLOCK_Pin = GPIO_PIN_4
 *   manager.h: RX_CE_Pin = GPIO_PIN_5       main.h: EN_P_3V3_CLOCK_Pin = GPIO_PIN_5
 *
 * And for the actual ADF4382 pins:
 *   main.h: ADF4382_TX_CE_Pin = GPIO_PIN_15 vs manager.h: TX_CE_Pin = GPIO_PIN_0
 *   main.h: ADF4382_TX_CS_Pin = GPIO_PIN_14 vs manager.h: TX_CS_Pin = GPIO_PIN_1
 *
 * Test strategy:
 *   Use compile-time assertions to verify the pin conflicts exist.
 *   Then use runtime checks to demonstrate the specific collisions.
 ******************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

/* We need to manually define the pins from BOTH headers since the shim
 * main.h already has the CubeMX definitions, and including adf4382a_manager.h
 * would re-define them (which is exactly the production bug). */

/* ---- Pin definitions from adf4382a_manager.h ---- */
#define MGR_TX_CE_Pin        ((uint16_t)0x0001)  /* GPIO_PIN_0 */
#define MGR_TX_CS_Pin        ((uint16_t)0x0002)  /* GPIO_PIN_1 */
#define MGR_TX_DELADJ_Pin    ((uint16_t)0x0004)  /* GPIO_PIN_2 */
#define MGR_TX_DELSTR_Pin    ((uint16_t)0x0008)  /* GPIO_PIN_3 */
#define MGR_TX_LKDET_Pin     ((uint16_t)0x0010)  /* GPIO_PIN_4 */
#define MGR_RX_CE_Pin        ((uint16_t)0x0020)  /* GPIO_PIN_5 */
#define MGR_RX_CS_Pin        ((uint16_t)0x0040)  /* GPIO_PIN_6 */
#define MGR_RX_DELADJ_Pin    ((uint16_t)0x0080)  /* GPIO_PIN_7 */
#define MGR_RX_DELSTR_Pin    ((uint16_t)0x0100)  /* GPIO_PIN_8 */
#define MGR_RX_LKDET_Pin     ((uint16_t)0x0200)  /* GPIO_PIN_9 */

/* ---- Pin definitions from main.h (CubeMX) ---- */
#define MAIN_EN_P_5V0_PA1_Pin     ((uint16_t)0x0001)  /* GPIO_PIN_0 — GPIOG */
#define MAIN_EN_P_5V0_PA2_Pin     ((uint16_t)0x0002)  /* GPIO_PIN_1 — GPIOG */
#define MAIN_EN_P_5V0_PA3_Pin     ((uint16_t)0x0004)  /* GPIO_PIN_2 — GPIOG */
#define MAIN_EN_P_5V5_PA_Pin      ((uint16_t)0x0008)  /* GPIO_PIN_3 — GPIOG */
#define MAIN_EN_P_1V8_CLOCK_Pin   ((uint16_t)0x0010)  /* GPIO_PIN_4 — GPIOG */
#define MAIN_EN_P_3V3_CLOCK_Pin   ((uint16_t)0x0020)  /* GPIO_PIN_5 — GPIOG */

/* Correct ADF4382 pins from main.h */
#define MAIN_ADF4382_TX_CE_Pin    ((uint16_t)0x8000)  /* GPIO_PIN_15 */
#define MAIN_ADF4382_TX_CS_Pin    ((uint16_t)0x4000)  /* GPIO_PIN_14 */
#define MAIN_ADF4382_TX_DELADJ_Pin ((uint16_t)0x2000) /* GPIO_PIN_13 */
#define MAIN_ADF4382_TX_DELSTR_Pin ((uint16_t)0x1000) /* GPIO_PIN_12 */
#define MAIN_ADF4382_TX_LKDET_Pin ((uint16_t)0x0800)  /* GPIO_PIN_11 */
#define MAIN_ADF4382_RX_CE_Pin    ((uint16_t)0x0200)  /* GPIO_PIN_9 */
#define MAIN_ADF4382_RX_CS_Pin    ((uint16_t)0x0400)  /* GPIO_PIN_10 */
#define MAIN_ADF4382_RX_DELADJ_Pin ((uint16_t)0x0080) /* GPIO_PIN_7 */
#define MAIN_ADF4382_RX_DELSTR_Pin ((uint16_t)0x0100) /* GPIO_PIN_8 */
#define MAIN_ADF4382_RX_LKDET_Pin ((uint16_t)0x0040)  /* GPIO_PIN_6 */

int main(void)
{
    int conflicts = 0;

    printf("=== Bug #7: GPIO pin mapping conflict ===\n");
    printf("\n  Checking manager.h pins vs CubeMX main.h pins (all GPIOG):\n\n");

    /* ---- Conflict checks: manager.h ADF4382 pins == main.h power enables ---- */

#define CHECK_CONFLICT(mgr_name, mgr_val, main_name, main_val) do {        \
    printf("  %-20s = 0x%04X  vs  %-25s = 0x%04X", #mgr_name, mgr_val,    \
           #main_name, main_val);                                            \
    if ((mgr_val) == (main_val)) {                                           \
        printf("  ** CONFLICT **\n");                                        \
        conflicts++;                                                         \
    } else {                                                                 \
        printf("  (ok)\n");                                                  \
    }                                                                        \
} while(0)

    printf("  --- manager.h TX pins collide with PA/clock enables ---\n");
    CHECK_CONFLICT(MGR_TX_CE,     MGR_TX_CE_Pin,     MAIN_EN_P_5V0_PA1, MAIN_EN_P_5V0_PA1_Pin);
    CHECK_CONFLICT(MGR_TX_CS,     MGR_TX_CS_Pin,     MAIN_EN_P_5V0_PA2, MAIN_EN_P_5V0_PA2_Pin);
    CHECK_CONFLICT(MGR_TX_DELADJ, MGR_TX_DELADJ_Pin, MAIN_EN_P_5V0_PA3, MAIN_EN_P_5V0_PA3_Pin);
    CHECK_CONFLICT(MGR_TX_DELSTR, MGR_TX_DELSTR_Pin, MAIN_EN_P_5V5_PA,  MAIN_EN_P_5V5_PA_Pin);
    CHECK_CONFLICT(MGR_TX_LKDET,  MGR_TX_LKDET_Pin,  MAIN_EN_P_1V8_CLK, MAIN_EN_P_1V8_CLOCK_Pin);
    CHECK_CONFLICT(MGR_RX_CE,     MGR_RX_CE_Pin,     MAIN_EN_P_3V3_CLK, MAIN_EN_P_3V3_CLOCK_Pin);

    printf("\n  --- manager.h TX pins vs correct CubeMX ADF4382 TX pins ---\n");
    CHECK_CONFLICT(MGR_TX_CE,     MGR_TX_CE_Pin,     MAIN_ADF_TX_CE,    MAIN_ADF4382_TX_CE_Pin);
    CHECK_CONFLICT(MGR_TX_CS,     MGR_TX_CS_Pin,     MAIN_ADF_TX_CS,    MAIN_ADF4382_TX_CS_Pin);
    CHECK_CONFLICT(MGR_TX_DELADJ, MGR_TX_DELADJ_Pin, MAIN_ADF_TX_DADJ,  MAIN_ADF4382_TX_DELADJ_Pin);
    CHECK_CONFLICT(MGR_TX_DELSTR, MGR_TX_DELSTR_Pin, MAIN_ADF_TX_DSTR,  MAIN_ADF4382_TX_DELSTR_Pin);
    CHECK_CONFLICT(MGR_TX_LKDET,  MGR_TX_LKDET_Pin,  MAIN_ADF_TX_LKDT,  MAIN_ADF4382_TX_LKDET_Pin);

    printf("\n  Total pin conflicts found: %d\n", conflicts);

    /* We expect 6 conflicts (the PA/clock enable collisions) and
     * 5 mismatches (manager.h pins != correct CubeMX ADF4382 pins) */
    assert(conflicts >= 6);
    printf("  PASS: At least 6 pin conflicts confirmed\n");

    /* ---- Verify specific critical conflict: TX_CE writes to PA1 enable ---- */
    printf("\n  Critical safety issue:\n");
    printf("  When adf4382a_manager.c writes TX_CE_Pin (0x%04X) on GPIOG,\n",
           MGR_TX_CE_Pin);
    printf("  it actually toggles EN_P_5V0_PA1 (0x%04X) — the PA1 5V power enable!\n",
           MAIN_EN_P_5V0_PA1_Pin);
    assert(MGR_TX_CE_Pin == MAIN_EN_P_5V0_PA1_Pin);
    printf("  PASS: Confirmed TX_CE_Pin == EN_P_5V0_PA1_Pin (0x%04X)\n",
           MGR_TX_CE_Pin);

    printf("=== Bug #7: ALL TESTS PASSED ===\n\n");
    return 0;
}
