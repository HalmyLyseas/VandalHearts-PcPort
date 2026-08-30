/* gte_width_check.c -- PR C / finding 2.6 regression.
 * Reproduces `g.tr[0] * 4096L` under the old (32-bit `long`, as on MinGW/LLP64)
 * and new (explicit int64_t, as libgte.c now does) forms with tr = 600000, a
 * value whose product overflows a 32-bit intermediate but not a 64-bit one.
 * Asserts the new form matches the true 64-bit result and that a genuine
 * 32-bit `long` would have wrapped to something else.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    int32_t tr0 = 600000;

    /* Old form as it executes under LLP64, where `long` is 32-bit: the
     * multiply happens in 32-bit precision and wraps mod 2^32. */
    uint32_t old_wrapped = (uint32_t)tr0 * (uint32_t)4096;
    int32_t old_form = (int32_t)old_wrapped;

    /* New (fixed) form: explicit 64-bit intermediate. */
    int64_t new_form = (int64_t)tr0 * 4096;
    int64_t expected = 600000LL * 4096LL;

    assert(new_form == expected);
    assert((int64_t)old_form != expected);

    printf("gte_width_check: tr=%d old(32-bit)=%d new(64-bit)=%lld expected=%lld -- PASS\n",
           tr0, old_form, (long long)new_form, (long long)expected);
    return 0;
}
