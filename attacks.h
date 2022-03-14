#include <stdbool.h>
#include <stdint.h>

uint64_t pawn_attacks(int sq, bool turn);
uint64_t knight_attacks(int);
uint64_t king_attacks(int);
uint64_t rook_attack_mask(int sq, uint64_t blockers);
uint64_t rook_attack_mask_flood_fill(int sq, uint64_t blockers);
uint64_t bishop_attack_mask(int sq, uint64_t blockers);
uint64_t bishop_attack_mask_flood_fill(int sq, uint64_t blockers);

#ifdef USE_MAGIC
#include "magic.h"
#define BISHOP_ATTACKS bishop_attacks
#define ROOK_ATTACKS rook_attacks
#else
#define BISHOP_ATTACKS bishop_attack_mask_flood_fill
#define ROOK_ATTACKS rook_attack_mask_flood_fill
#endif
