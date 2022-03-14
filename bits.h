#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static uint64_t set_mask(int sq) { return 1ULL << sq; }
static uint64_t clr_mask(int sq) { return ~set_mask(sq); }

static bool is_bit_set(uint64_t x, int sq) { return (x & set_mask(sq)) != 0LL; }

// Masking out the h-file
static const uint64_t NOT_A_FILE = 0xfefefefefefefefeuLL;
// Masking out the a-file
static const uint64_t NOT_H_FILE = 0x7f7f7f7f7f7f7f7fuLL;
// Masking the 1st and 8th rank
static const uint64_t RANK_18 = 0xff000000000000ffuLL;
// Mask for the 3rd rank
static const uint64_t RANK_3 = 0x0000000000ff0000uLL;
// Mask for the 6th rank
static const uint64_t RANK_6 = 0x0000ff0000000000uLL;

static uint64_t shift_up(uint64_t x) { return x << 8; }
static uint64_t shift_down(uint64_t x) { return x >> 8; }
static uint64_t shift_left(uint64_t x) { return (x & NOT_A_FILE) >> 1; }
static uint64_t shift_right(uint64_t x) { return (x & NOT_H_FILE) << 1; }
static uint64_t shift_up_left(uint64_t x) { return (x & NOT_A_FILE) << 7; }
static uint64_t shift_up_right(uint64_t x) { return (x & NOT_H_FILE) << 9; }
static uint64_t shift_down_left(uint64_t x) { return (x & NOT_A_FILE) >> 9; }
static uint64_t shift_down_right(uint64_t x) { return (x & NOT_H_FILE) >> 7; }

// Same as above, no mask applied
static uint64_t shift_left_n(uint64_t x) { return x >> 1; }
static uint64_t shift_right_n(uint64_t x) { return x << 1; }
static uint64_t shift_up_left_n(uint64_t x) { return x << 7; }
static uint64_t shift_up_right_n(uint64_t x) { return x << 9; }
static uint64_t shift_down_left_n(uint64_t x) { return x >> 9; }
static uint64_t shift_down_right_n(uint64_t x) { return x >> 7; }

#ifdef __arm__

static int ctzll(uint64_t x) {
    uint32_t y = (uint32_t)x;
    if (y)
        return __builtin_ctz(y);
    else
        return __builtin_ctz(x >> 32) + 32;
}
#else
#define ctzll __builtin_ctzll
#endif

static int poplsb(uint64_t *x) {
    int lsb = ctzll(*x);
    *x &= *x - 1;
    return lsb;
}

void print_bitboard(uint64_t);
void print_2bitboards(uint64_t, uint64_t);

#ifdef __cplusplus
}
#endif
