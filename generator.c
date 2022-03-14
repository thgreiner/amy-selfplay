#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static uint64_t rook_blocker_mask(int sq) {
    int rank = sq >> 3;
    int file = sq & 7;
    uint64_t x = 0;

    for (int rk = 1; rk < rank; rk++) {
        x |= 1LL << (rk * 8 + file);
    }
    for (int rk = 6; rk > rank; rk--) {
        x |= 1LL << (rk * 8 + file);
    }
    for (int fl = 1; fl < file; fl++) {
        x |= 1LL << (rank * 8 + fl);
    }
    for (int fl = 6; fl > file; fl--) {
        x |= 1LL << (rank * 8 + fl);
    }

    return x;
}

static uint64_t bishop_blocker_mask(int sq) {
    int rank = sq >> 3;
    int file = sq & 7;
    uint64_t x = 0;

    for (int rk = rank - 1, fl = file - 1; rk > 0 && fl > 0; rk--, fl--) {
        x |= 1LL << (rk * 8 + fl);
    }
    for (int rk = rank + 1, fl = file - 1; rk < 7 && fl > 0; rk++, fl--) {
        x |= 1LL << (rk * 8 + fl);
    }
    for (int rk = rank - 1, fl = file + 1; rk > 0 && fl < 7; rk--, fl++) {
        x |= 1LL << (rk * 8 + fl);
    }
    for (int rk = rank + 1, fl = file + 1; rk < 7 && fl < 7; rk++, fl++) {
        x |= 1LL << (rk * 8 + fl);
    }
    return x;
}

int main(int argc, char *argv[]) {

    printf("const static uint64_t rook_blocker_mask[] = {\n");
    for (int i = 0; i < 64; i++) {
        printf("  0x%llx,\n", rook_blocker_mask(i));
    }
    printf("};\n");
    printf("const static uint64_t bishop_blocker_mask[] = {\n");
    for (int i = 0; i < 64; i++) {
        printf("  0x%llx,\n", bishop_blocker_mask(i));
    }
    printf("};\n");

    return 0;
}
