#include <stdbool.h>

#include "heap.h"
#include "position.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CASTLE_KINGSIDE (1u << 21)
#define CASTLE_QUEENSIDE (1u << 22)

void generate_pseudolegal_moves(heap_t heap, const position_t p);
void do_move(position_t source, position_t destination, int move);
bool is_castle_legal(position_t p, uint32_t move);
void san(char *buffer, heap_t heap, position_t pos, uint32_t move);
bool is_checkmate(heap_t heap, position_t pos);
bool parse_san(const char *san, heap_t heap, position_t pos, uint32_t *move);

static const int PIECE_OFFSET = 12;
static const int PROMOTION_OFFSET = 15;

static int move_from(uint32_t move) { return move & 63; }

static int move_to(uint32_t move) { return (move >> 6) & 63; }

static int move_piece(uint32_t move) { return (move >> PIECE_OFFSET) & 7; }

static int move_promotion(uint32_t move) {
    return (move >> PROMOTION_OFFSET) & 7;
}

#ifdef __cplusplus
}
#endif
