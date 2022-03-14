#include <stdbool.h>

#include "bits.h"

extern const uint64_t knight_attack_data[];
extern const uint64_t pawn_attack_data[2][64];
extern const uint64_t king_attack_data[];

uint64_t knight_attacks_gen(int sq) {
    uint64_t x = 1LL << sq;

    uint64_t t1 = shift_left(shift_left(x)) | shift_right(shift_right(x));
    uint64_t t2 = shift_up(shift_up(x)) | shift_down(shift_down(x));

    return shift_up(t1) | shift_down(t1) | shift_left(t2) | shift_right(t2);
}

uint64_t knight_attacks(int sq) { return knight_attack_data[sq]; }

uint64_t king_attacks_gen(int sq) {
    uint64_t x = 1LL << sq;
    uint64_t t1 = shift_left(x) | shift_right(x);

    return t1 | shift_up(t1 | x) | shift_down(t1 | x);
}

uint64_t king_attacks(int sq) { return king_attack_data[sq]; }

uint64_t pawn_attacks_gen(int sq, bool turn) {
    uint64_t x = 1LL << sq;
    uint64_t t1 = shift_left(x) | shift_right(x);

    return turn ? shift_up(t1) : shift_down(t1);
}

uint64_t pawn_attacks(int sq, bool turn) { return pawn_attack_data[turn][sq]; }

uint64_t rook_attack_mask(int sq, uint64_t blockers) {
    uint64_t x = 0;

    uint64_t m = shift_left(set_mask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = shift_left(m);
    }

    m = shift_right(set_mask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = shift_right(m);
    }

    m = shift_up(set_mask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = shift_up(m);
    }

    m = shift_down(set_mask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = shift_down(m);
    }
    return x;
}

uint64_t bishop_attack_mask(int sq, uint64_t blockers) {
    uint64_t x = 0;

    uint64_t m = shift_up_left(set_mask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = shift_up_left(m);
    }

    m = shift_up_right(set_mask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = shift_up_right(m);
    }

    m = shift_down_left(set_mask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = shift_down_left(m);
    }

    m = shift_down_right(set_mask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = shift_down_right(m);
    }
    return x;
}

uint64_t rook_attack_mask_flood_fill(int sq, uint64_t blockers) {
    const uint64_t rook = 1ULL << sq;
    const uint64_t empty = ~blockers;
    uint64_t attacks;
    uint64_t result;

    uint64_t flood = rook;
    attacks = flood;
    flood = shift_up(flood) & empty;
    attacks |= flood;
    flood = shift_up(flood) & empty;
    attacks |= flood;
    flood = shift_up(flood) & empty;
    attacks |= flood;
    flood = shift_up(flood) & empty;
    attacks |= flood;
    flood = shift_up(flood) & empty;
    attacks |= flood;
    flood = shift_up(flood) & empty;
    attacks |= flood;

    result = shift_up(attacks);

    flood = rook;
    attacks = flood;
    flood = shift_down(flood) & empty;
    attacks |= flood;
    flood = shift_down(flood) & empty;
    attacks |= flood;
    flood = shift_down(flood) & empty;
    attacks |= flood;
    flood = shift_down(flood) & empty;
    attacks |= flood;
    flood = shift_down(flood) & empty;
    attacks |= flood;
    flood = shift_down(flood) & empty;
    attacks |= flood;

    result |= shift_down(attacks);

    flood = rook;
    attacks = flood;
    const uint64_t empty_no_h = empty & NOT_H_FILE;

    flood = shift_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_left_n(flood) & empty_no_h;
    attacks |= flood;

    result |= shift_left(attacks);

    flood = rook;
    attacks = flood;
    const uint64_t empty_no_a = empty & NOT_A_FILE;

    flood = shift_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_right_n(flood) & empty_no_a;
    attacks |= flood;

    result |= shift_right(attacks);
    return result;
}

uint64_t bishop_attack_mask_flood_fill(int sq, uint64_t blockers) {
    const uint64_t bishop = 1ULL << sq;
    const uint64_t empty = ~blockers;
    uint64_t attacks;
    uint64_t result;

    uint64_t flood = bishop;
    attacks = flood;
    const uint64_t empty_no_h = empty & NOT_H_FILE;
    flood = shift_up_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_up_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_up_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_up_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_up_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_up_left_n(flood) & empty_no_h;
    attacks |= flood;

    result = shift_up_left(attacks);

    flood = bishop;
    attacks = flood;
    flood = shift_down_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_down_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_down_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_down_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_down_left_n(flood) & empty_no_h;
    attacks |= flood;
    flood = shift_down_left_n(flood) & empty_no_h;
    attacks |= flood;

    result |= shift_down_left(attacks);

    flood = bishop;
    attacks = flood;
    const uint64_t empty_no_a = empty & NOT_A_FILE;
    flood = shift_up_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_up_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_up_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_up_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_up_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_up_right_n(flood) & empty_no_a;
    attacks |= flood;

    result |= shift_up_right(attacks);

    flood = bishop;
    attacks = flood;
    flood = shift_down_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_down_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_down_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_down_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_down_right_n(flood) & empty_no_a;
    attacks |= flood;
    flood = shift_down_right_n(flood) & empty_no_a;
    attacks |= flood;

    result |= shift_down_right(attacks);
    return result;
}

const uint64_t knight_attack_data[] = {
    0x20400ULL,
    0x50800ULL,
    0xa1100ULL,
    0x142200ULL,
    0x284400ULL,
    0x508800ULL,
    0xa01000ULL,
    0x402000ULL,
    0x2040004ULL,
    0x5080008ULL,
    0xa110011ULL,
    0x14220022ULL,
    0x28440044ULL,
    0x50880088ULL,
    0xa0100010ULL,
    0x40200020ULL,
    0x204000402ULL,
    0x508000805ULL,
    0xa1100110aULL,
    0x1422002214ULL,
    0x2844004428ULL,
    0x5088008850ULL,
    0xa0100010a0ULL,
    0x4020002040ULL,
    0x20400040200ULL,
    0x50800080500ULL,
    0xa1100110a00ULL,
    0x142200221400ULL,
    0x284400442800ULL,
    0x508800885000ULL,
    0xa0100010a000ULL,
    0x402000204000ULL,
    0x2040004020000ULL,
    0x5080008050000ULL,
    0xa1100110a0000ULL,
    0x14220022140000ULL,
    0x28440044280000ULL,
    0x50880088500000ULL,
    0xa0100010a00000ULL,
    0x40200020400000ULL,
    0x204000402000000ULL,
    0x508000805000000ULL,
    0xa1100110a000000ULL,
    0x1422002214000000ULL,
    0x2844004428000000ULL,
    0x5088008850000000ULL,
    0xa0100010a0000000ULL,
    0x4020002040000000ULL,
    0x400040200000000ULL,
    0x800080500000000ULL,
    0x1100110a00000000ULL,
    0x2200221400000000ULL,
    0x4400442800000000ULL,
    0x8800885000000000ULL,
    0x100010a000000000ULL,
    0x2000204000000000ULL,
    0x4020000000000ULL,
    0x8050000000000ULL,
    0x110a0000000000ULL,
    0x22140000000000ULL,
    0x44280000000000ULL,
    0x88500000000000ULL,
    0x10a00000000000ULL,
    0x20400000000000ULL,
};

const uint64_t pawn_attack_data[2][64] = {{0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x2ULL,
                                           0x5ULL,
                                           0xaULL,
                                           0x14ULL,
                                           0x28ULL,
                                           0x50ULL,
                                           0xa0ULL,
                                           0x40ULL,
                                           0x200ULL,
                                           0x500ULL,
                                           0xa00ULL,
                                           0x1400ULL,
                                           0x2800ULL,
                                           0x5000ULL,
                                           0xa000ULL,
                                           0x4000ULL,
                                           0x20000ULL,
                                           0x50000ULL,
                                           0xa0000ULL,
                                           0x140000ULL,
                                           0x280000ULL,
                                           0x500000ULL,
                                           0xa00000ULL,
                                           0x400000ULL,
                                           0x2000000ULL,
                                           0x5000000ULL,
                                           0xa000000ULL,
                                           0x14000000ULL,
                                           0x28000000ULL,
                                           0x50000000ULL,
                                           0xa0000000ULL,
                                           0x40000000ULL,
                                           0x200000000ULL,
                                           0x500000000ULL,
                                           0xa00000000ULL,
                                           0x1400000000ULL,
                                           0x2800000000ULL,
                                           0x5000000000ULL,
                                           0xa000000000ULL,
                                           0x4000000000ULL,
                                           0x20000000000ULL,
                                           0x50000000000ULL,
                                           0xa0000000000ULL,
                                           0x140000000000ULL,
                                           0x280000000000ULL,
                                           0x500000000000ULL,
                                           0xa00000000000ULL,
                                           0x400000000000ULL,
                                           0x2000000000000ULL,
                                           0x5000000000000ULL,
                                           0xa000000000000ULL,
                                           0x14000000000000ULL,
                                           0x28000000000000ULL,
                                           0x50000000000000ULL,
                                           0xa0000000000000ULL,
                                           0x40000000000000ULL},
                                          {0x200ULL,
                                           0x500ULL,
                                           0xa00ULL,
                                           0x1400ULL,
                                           0x2800ULL,
                                           0x5000ULL,
                                           0xa000ULL,
                                           0x4000ULL,
                                           0x20000ULL,
                                           0x50000ULL,
                                           0xa0000ULL,
                                           0x140000ULL,
                                           0x280000ULL,
                                           0x500000ULL,
                                           0xa00000ULL,
                                           0x400000ULL,
                                           0x2000000ULL,
                                           0x5000000ULL,
                                           0xa000000ULL,
                                           0x14000000ULL,
                                           0x28000000ULL,
                                           0x50000000ULL,
                                           0xa0000000ULL,
                                           0x40000000ULL,
                                           0x200000000ULL,
                                           0x500000000ULL,
                                           0xa00000000ULL,
                                           0x1400000000ULL,
                                           0x2800000000ULL,
                                           0x5000000000ULL,
                                           0xa000000000ULL,
                                           0x4000000000ULL,
                                           0x20000000000ULL,
                                           0x50000000000ULL,
                                           0xa0000000000ULL,
                                           0x140000000000ULL,
                                           0x280000000000ULL,
                                           0x500000000000ULL,
                                           0xa00000000000ULL,
                                           0x400000000000ULL,
                                           0x2000000000000ULL,
                                           0x5000000000000ULL,
                                           0xa000000000000ULL,
                                           0x14000000000000ULL,
                                           0x28000000000000ULL,
                                           0x50000000000000ULL,
                                           0xa0000000000000ULL,
                                           0x40000000000000ULL,
                                           0x200000000000000ULL,
                                           0x500000000000000ULL,
                                           0xa00000000000000ULL,
                                           0x1400000000000000ULL,
                                           0x2800000000000000ULL,
                                           0x5000000000000000ULL,
                                           0xa000000000000000ULL,
                                           0x4000000000000000ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL,
                                           0x0ULL}};

const uint64_t king_attack_data[] = {
    0x302ULL,
    0x705ULL,
    0xe0aULL,
    0x1c14ULL,
    0x3828ULL,
    0x7050ULL,
    0xe0a0ULL,
    0xc040ULL,
    0x30203ULL,
    0x70507ULL,
    0xe0a0eULL,
    0x1c141cULL,
    0x382838ULL,
    0x705070ULL,
    0xe0a0e0ULL,
    0xc040c0ULL,
    0x3020300ULL,
    0x7050700ULL,
    0xe0a0e00ULL,
    0x1c141c00ULL,
    0x38283800ULL,
    0x70507000ULL,
    0xe0a0e000ULL,
    0xc040c000ULL,
    0x302030000ULL,
    0x705070000ULL,
    0xe0a0e0000ULL,
    0x1c141c0000ULL,
    0x3828380000ULL,
    0x7050700000ULL,
    0xe0a0e00000ULL,
    0xc040c00000ULL,
    0x30203000000ULL,
    0x70507000000ULL,
    0xe0a0e000000ULL,
    0x1c141c000000ULL,
    0x382838000000ULL,
    0x705070000000ULL,
    0xe0a0e0000000ULL,
    0xc040c0000000ULL,
    0x3020300000000ULL,
    0x7050700000000ULL,
    0xe0a0e00000000ULL,
    0x1c141c00000000ULL,
    0x38283800000000ULL,
    0x70507000000000ULL,
    0xe0a0e000000000ULL,
    0xc040c000000000ULL,
    0x302030000000000ULL,
    0x705070000000000ULL,
    0xe0a0e0000000000ULL,
    0x1c141c0000000000ULL,
    0x3828380000000000ULL,
    0x7050700000000000ULL,
    0xe0a0e00000000000ULL,
    0xc040c00000000000ULL,
    0x203000000000000ULL,
    0x507000000000000ULL,
    0xa0e000000000000ULL,
    0x141c000000000000ULL,
    0x2838000000000000ULL,
    0x5070000000000000ULL,
    0xa0e0000000000000ULL,
    0x40c0000000000000ULL,
};
