#include <stdbool.h>
#include <stdint.h>

#ifndef POSITION_H
#define POSITION_H

#ifdef __cplusplus
extern "C" {
#endif

extern const char *INITIAL_POSITION_EPD;

static const int PAWN = 0;
static const int KNIGHT = 1;
static const int BISHOP = 2;
static const int ROOK = 3;
static const int QUEEN = 4;
static const int KING = 5;

struct position {
    uint64_t by_color[2];
    uint64_t by_type[6];
    uint64_t en_passant;
    uint16_t ply;
    uint16_t irrev_count;
    bool turn;
    bool can_castle[2][2];
    struct position *prev;
};

typedef struct position *position_t;

static uint64_t all_pieces(position_t p) {
    return p->by_color[0] | p->by_color[1];
}

bool parse_epd(position_t, const char *);
void to_epd(position_t p, char *buffer);
bool is_square_attacked_by(position_t, int, bool);
bool is_king_in_check(position_t, bool);
void print_position(position_t);
bool is_insufficient_material(position_t);
bool is_repeated(position_t p, int count);

#ifdef __cplusplus
}
#endif

#endif
