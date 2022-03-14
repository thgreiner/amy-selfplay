#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "attacks.h"
#include "bits.h"
#include "heap.h"
#include "movegen.h"

static const uint32_t MOVE_PAWN = PAWN << PIECE_OFFSET;
static const uint32_t MOVE_KNIGHT = KNIGHT << PIECE_OFFSET;
static const uint32_t MOVE_BISHOP = BISHOP << PIECE_OFFSET;
static const uint32_t MOVE_ROOK = ROOK << PIECE_OFFSET;
static const uint32_t MOVE_QUEEN = QUEEN << PIECE_OFFSET;
static const uint32_t MOVE_KING = KING << PIECE_OFFSET;
static const uint32_t MOVE_PIECE = 7u << PIECE_OFFSET;

static const uint32_t PROMO_QUEEN = QUEEN << PROMOTION_OFFSET;
static const uint32_t PROMO_ROOK = ROOK << PROMOTION_OFFSET;
static const uint32_t PROMO_BISHOP = BISHOP << PROMOTION_OFFSET;
static const uint32_t PROMO_KNIGHT = KNIGHT << PROMOTION_OFFSET;

static const uint32_t CAPTURE = 1u << 18;
static const uint32_t PAWN_DOUBLE_STEP = 1u << 19;
static const uint32_t EN_PASSANT = 1u << 20;

static const uint64_t castle_check_squares[2][2] = {
    {0x0e00000000000000uLL, 0x6000000000000000uLL},
    {0x000000000000000euLL, 0x0000000000000060uLL}};

static const int rook_move_disables_castling[2][2] = {{56, 63}, {0, 7}};

static const char piece_names[] = {'P', 'N', 'B', 'R', 'Q', 'K'};

void print_bitboard(uint64_t b) {
    for (int row = 7; row >= 0; row--) {
        for (int col = 0; col < 8; col++) {
            int addr = (row << 3) + col;
            printf((b & (1LL << addr)) ? "X " : ". ");
        }
        printf("\n");
    }
}

void print_2bitboards(uint64_t b, uint64_t c) {
    for (int row = 7; row >= 0; row--) {
        for (int col = 0; col < 8; col++) {
            int addr = (row << 3) + col;
            printf((b & (1LL << addr)) ? "X " : ". ");
        }
        printf("  ");
        for (int col = 0; col < 8; col++) {
            int addr = (row << 3) + col;
            printf((c & (1LL << addr)) ? "X " : ". ");
        }
        printf("\n");
    }
}

static int rank_of(int sq) { return sq >> 3; }

static int file_of(int sq) { return sq & 7; }

static char file_char(int sq) { return 'a' + file_of(sq); }

static char rank_char(int sq) { return '1' + rank_of(sq); }

static uint32_t make_move(int from, int to, int flags) {
    return from | (to * 64) | flags;
}

static void generate_moves(heap_t restrict heap, const restrict position_t p,
                           int sq, const uint64_t targets) {
    uint64_t captures = targets & p->by_color[!p->turn];
    while (captures) {
        int dst = poplsb(&captures);
        append_to_heap(heap, make_move(sq, dst, CAPTURE));
    }

    uint64_t non_captures = targets & ~all_pieces(p);
    while (non_captures) {
        int dst = poplsb(&non_captures);
        append_to_heap(heap, make_move(sq, dst, 0));
    }
}

static bool check_castling_rights(position_t restrict p, bool kingside) {
    return p->can_castle[p->turn][kingside] &&
           (castle_check_squares[p->turn][kingside] & all_pieces(p)) == 0;
}

void generate_pseudolegal_moves(heap_t restrict heap,
                                const restrict position_t p) {
    const uint64_t movers = p->by_color[p->turn];
    const uint64_t victims = p->by_color[!p->turn];

    uint64_t pawns = p->by_type[PAWN] & movers;
    while (pawns) {
        int sq = poplsb(&pawns);
        uint64_t targets = pawn_attacks(sq, p->turn);

        uint64_t non_promotions = targets & victims & ~RANK_18;
        while (non_promotions) {
            int dst = poplsb(&non_promotions);
            append_to_heap(heap, make_move(sq, dst, MOVE_PAWN | CAPTURE));
        }

        uint64_t ep_captures = targets & p->en_passant;
        if (ep_captures) {
            int dst = poplsb(&ep_captures);
            append_to_heap(heap, make_move(sq, dst, MOVE_PAWN | EN_PASSANT));
        }

        uint64_t promotions = targets & victims & RANK_18;
        while (promotions) {
            int dst = poplsb(&promotions);
            append_to_heap(
                heap, make_move(sq, dst, MOVE_PAWN | CAPTURE | PROMO_QUEEN));
            append_to_heap(
                heap, make_move(sq, dst, MOVE_PAWN | CAPTURE | PROMO_ROOK));
            append_to_heap(
                heap, make_move(sq, dst, MOVE_PAWN | CAPTURE | PROMO_BISHOP));
            append_to_heap(
                heap, make_move(sq, dst, MOVE_PAWN | CAPTURE | PROMO_KNIGHT));
        }
    }

    uint64_t pawn_pushes = p->by_type[PAWN] & movers;
    int delta;
    if (p->turn) {
        pawn_pushes = shift_up(pawn_pushes);
        delta = -8;
    } else {
        pawn_pushes = shift_down(pawn_pushes);
        delta = 8;
    }
    pawn_pushes &= ~all_pieces(p);

    uint64_t non_promotions = pawn_pushes & ~RANK_18;
    while (non_promotions) {
        int dst = poplsb(&non_promotions);
        append_to_heap(heap, make_move(dst + delta, dst, MOVE_PAWN));
    }

    uint64_t promotions = pawn_pushes & RANK_18;
    while (promotions) {
        int dst = poplsb(&promotions);
        append_to_heap(heap,
                       make_move(dst + delta, dst, MOVE_PAWN | PROMO_QUEEN));
        append_to_heap(heap,
                       make_move(dst + delta, dst, MOVE_PAWN | PROMO_ROOK));
        append_to_heap(heap,
                       make_move(dst + delta, dst, MOVE_PAWN | PROMO_BISHOP));
        append_to_heap(heap,
                       make_move(dst + delta, dst, MOVE_PAWN | PROMO_KNIGHT));
    }

    uint64_t pawn_double_steps;
    if (p->turn) {
        pawn_double_steps = shift_up(pawn_pushes & RANK_3);
    } else {
        pawn_double_steps = shift_down(pawn_pushes & RANK_6);
    }
    pawn_double_steps &= ~all_pieces(p);
    while (pawn_double_steps) {
        int dst = poplsb(&pawn_double_steps);
        append_to_heap(heap, make_move(dst + 2 * delta, dst,
                                       MOVE_PAWN | PAWN_DOUBLE_STEP));
    }

    uint64_t knights = p->by_type[KNIGHT] & movers;
    while (knights) {
        int sq = poplsb(&knights);
        uint64_t targets = knight_attacks(sq);
        generate_moves(heap, p, sq | MOVE_KNIGHT, targets);
    }

    uint64_t bishops = p->by_type[BISHOP] & movers;
    while (bishops) {
        int sq = poplsb(&bishops);
        uint64_t targets = BISHOP_ATTACKS(sq, all_pieces(p));
        generate_moves(heap, p, sq | MOVE_BISHOP, targets);
    }

    uint64_t rooks = p->by_type[ROOK] & movers;
    while (rooks) {
        int sq = poplsb(&rooks);
        uint64_t targets = ROOK_ATTACKS(sq, all_pieces(p));
        generate_moves(heap, p, sq | MOVE_ROOK, targets);
    }

    uint64_t queens = p->by_type[QUEEN] & movers;
    while (queens) {
        int sq = poplsb(&queens);
        uint64_t targets =
            ROOK_ATTACKS(sq, all_pieces(p)) | BISHOP_ATTACKS(sq, all_pieces(p));
        generate_moves(heap, p, sq | MOVE_QUEEN, targets);
    }

    uint64_t kings = p->by_type[KING] & movers;
    if (true) {
        int sq = poplsb(&kings);
        uint64_t targets = king_attacks(sq);
        generate_moves(heap, p, sq | MOVE_KING, targets);

        if (check_castling_rights(p, true))
            append_to_heap(heap,
                           make_move(sq, sq + 2, MOVE_KING | CASTLE_KINGSIDE));

        if (check_castling_rights(p, false))
            append_to_heap(heap,
                           make_move(sq, sq - 2, MOVE_KING | CASTLE_QUEENSIDE));
    }
}

void print_move(uint32_t move) {
    const int from = move_from(move);
    const int to = move_to(move);
    const int piece = move_piece(move);
    const int promotion = move_promotion(move);

    printf("%c%c%c%c%c%c", piece_names[piece], file_char(from), rank_char(from),
           (move & (CAPTURE | EN_PASSANT)) ? 'x' : '-', file_char(to),
           rank_char(to));

    if (promotion) {
        printf("=%c", piece_names[promotion]);
    }
    if (move & EN_PASSANT) {
        printf("e.p.");
    }
    // printf("\n");
}

void do_move(const position_t restrict source, position_t restrict destination,
             int move) {
    int from = move_from(move);
    int to = move_to(move);
    bool is_capture = move & CAPTURE;
    uint64_t capture_mask = set_mask(to) * is_capture;
    int promotion = move_promotion(move);
    int piece = promotion ? promotion : move_piece(move);

    // Update all masks
    destination->by_color[source->turn] =
        (source->by_color[source->turn] & clr_mask(from)) | set_mask(to);
    destination->by_color[!source->turn] =
        source->by_color[!source->turn] & ~capture_mask;

    for (int i = PAWN; i <= KING; i++) {
        destination->by_type[i] =
            source->by_type[i] & ~capture_mask & clr_mask(from);
    }
    destination->by_type[piece] = destination->by_type[piece] | set_mask(to);

    if (move & EN_PASSANT) {
        uint64_t ep_mask = source->turn ? shift_down(source->en_passant)
                                        : shift_up(source->en_passant);
        destination->by_type[PAWN] &= ~ep_mask;
        destination->by_color[!source->turn] &= ~ep_mask;
    }

    if (move & CASTLE_KINGSIDE) {
        int rook_to = from + 1;
        int rook_from = from + 3;
        destination->by_color[source->turn] =
            (destination->by_color[source->turn] & clr_mask(rook_from)) |
            set_mask(rook_to);
        destination->by_type[ROOK] =
            (destination->by_type[ROOK] & clr_mask(rook_from)) |
            set_mask(rook_to);
    }

    if (move & CASTLE_QUEENSIDE) {
        int rook_to = from - 1;
        int rook_from = from - 4;
        destination->by_color[source->turn] =
            (destination->by_color[source->turn] & clr_mask(rook_from)) |
            set_mask(rook_to);
        destination->by_type[ROOK] =
            (destination->by_type[ROOK] & clr_mask(rook_from)) |
            set_mask(rook_to);
    }

    // Set en passant if double pawn move
    destination->en_passant =
        (move & PAWN_DOUBLE_STEP) ? set_mask(from - 8 + 16 * source->turn) : 0;

    // Update castling rights
    destination->can_castle[0][0] = source->can_castle[0][0];
    destination->can_castle[0][1] = source->can_castle[0][1];
    destination->can_castle[1][0] = source->can_castle[1][0];
    destination->can_castle[1][1] = source->can_castle[1][1];

    if (piece == KING) {
        // moving the king disables castling for side to move
        destination->can_castle[source->turn][0] = false;
        destination->can_castle[source->turn][1] = false;
    } else if (piece == ROOK) {
        // moving a rook from a1/a8/h1/h8 disables castling
        for (int i = 1; i >= 0; i--)
            destination->can_castle[source->turn][i] &=
                (from != rook_move_disables_castling[source->turn][i]);
    }

    // capturing a rook disables castling
    if (is_capture) {
        for (int i = 1; i >= 0; i--)
            destination->can_castle[!source->turn][i] &=
                (to != rook_move_disables_castling[!source->turn][i]);
    }

    if (is_capture || move_piece(move) == PAWN) {
        destination->irrev_count = 0;
    } else {
        destination->irrev_count = source->irrev_count + 1;
    }

    destination->ply = source->ply + 1;
    destination->turn = !source->turn;
    destination->prev = source;

    assert(destination->by_type[PAWN] ==
           (destination->by_type[PAWN] & all_pieces(destination)));
    assert(destination->by_type[KNIGHT] ==
           (destination->by_type[KNIGHT] & all_pieces(destination)));
    assert(destination->by_type[BISHOP] ==
           (destination->by_type[BISHOP] & all_pieces(destination)));
    assert(destination->by_type[ROOK] ==
           (destination->by_type[ROOK] & all_pieces(destination)));
    assert(destination->by_type[QUEEN] ==
           (destination->by_type[QUEEN] & all_pieces(destination)));
    assert(destination->by_type[KING] ==
           (destination->by_type[KING] & all_pieces(destination)));
    assert((destination->by_type[PAWN] | destination->by_type[KNIGHT] |
            destination->by_type[BISHOP] | destination->by_type[ROOK] |
            destination->by_type[QUEEN] | destination->by_type[KING]) ==
           all_pieces(destination));

    assert((destination->by_color[1] & destination->by_color[0]) == 0);
    assert((destination->by_type[PAWN] & destination->by_type[KNIGHT]) == 0);
    assert((destination->by_type[PAWN] & destination->by_type[BISHOP]) == 0);
    assert((destination->by_type[PAWN] & destination->by_type[ROOK]) == 0);
    assert((destination->by_type[PAWN] & destination->by_type[QUEEN]) == 0);
    assert((destination->by_type[PAWN] & destination->by_type[KING]) == 0);
    assert((destination->by_type[KNIGHT] & destination->by_type[BISHOP]) == 0);
    assert((destination->by_type[KNIGHT] & destination->by_type[ROOK]) == 0);
    assert((destination->by_type[KNIGHT] & destination->by_type[QUEEN]) == 0);
    assert((destination->by_type[KNIGHT] & destination->by_type[KING]) == 0);
    assert((destination->by_type[BISHOP] & destination->by_type[ROOK]) == 0);
    assert((destination->by_type[BISHOP] & destination->by_type[QUEEN]) == 0);
    assert((destination->by_type[BISHOP] & destination->by_type[KING]) == 0);
    assert((destination->by_type[ROOK] & destination->by_type[KING]) == 0);
    assert((destination->by_type[ROOK] & destination->by_type[KING]) == 0);
    assert((destination->by_type[QUEEN] & destination->by_type[KING]) == 0);

    assert(__builtin_popcountll(destination->by_type[KING] &
                                destination->by_color[1]) == 1);
    assert(__builtin_popcountll(destination->by_type[KING] &
                                destination->by_color[0]) == 1);
}

bool is_castle_legal(position_t p, uint32_t move) {
    const int from = move_from(move);
    const int int_square = (move & CASTLE_KINGSIDE) ? from + 1 : from - 1;

    return !is_square_attacked_by(p, int_square, !p->turn);
}

/**
 * Check whether the current position is checkmate. This function should only be
 * called if the side to move is in check.
 *
 * @param heap a heap to generate (temporary moves)
 * @param pos the position
 * @return `true` if there are no legal moves for the side to move, `false`
 * otherwise
 */
bool is_checkmate(heap_t heap, position_t pos) {
    bool checkmate = true;
    struct position t;

    push_section(heap);
    generate_pseudolegal_moves(heap, pos);

    for (int i = heap->current_section->start; i < heap->current_section->end;
         i++) {
        uint32_t move = heap->data[i];

        if (move & (CASTLE_KINGSIDE | CASTLE_QUEENSIDE)) {
            continue;
        }
        do_move(pos, &t, move);
        if (is_king_in_check(&t, !t.turn))
            continue;

        checkmate = false;
        break;
    }
    pop_section(heap);
    return checkmate;
}

/**
 * Generate pseudolegal moves by _piece_ (N, B, R, Q) to square _to_.
 */
void generate_to(heap_t heap, position_t p, int to, int piece) {
    const uint64_t movers = p->by_color[p->turn];
    const uint64_t victims = p->by_color[!p->turn];

    uint64_t attackers = 0;

    if (piece == KNIGHT) {
        attackers = knight_attacks(to) & p->by_type[KNIGHT] & movers;
    } else {
        if (piece == BISHOP || piece == QUEEN) {
            attackers |=
                BISHOP_ATTACKS(to, all_pieces(p)) & movers & p->by_type[piece];
        }
        if (piece == ROOK || piece == QUEEN) {
            attackers |=
                ROOK_ATTACKS(to, all_pieces(p)) & movers & p->by_type[piece];
        }
    }

    uint32_t flags = is_bit_set(victims, to) * CAPTURE;
    flags |= piece << PIECE_OFFSET;

    while (attackers) {
        int from = poplsb(&attackers);
        append_to_heap(heap, make_move(from, to, flags));
    }
}

/**
 * Generate the standard algebraic notation for a move.
 *
 * @param buffer the output buffer
 * @param heap a heap to generate (temporary moves)
 * @param pos the position
 * @param move the move
 */
void san(char *buffer, heap_t heap, position_t pos, uint32_t move) {
    const int piece = move_piece(move);
    const int from = move_from(move);
    const int to = move_to(move);
    char *x = buffer;

    if (piece == PAWN) {
        if (move & (CAPTURE | EN_PASSANT)) {
            *(x++) = file_char(from);
            *(x++) = 'x';
        }
        *(x++) = file_char(to);
        *(x++) = rank_char(to);
        const int promotion = move_promotion(move);
        if (promotion) {
            *(x++) = '=';
            *(x++) = piece_names[promotion];
        }
    } else {
        if (move & CASTLE_KINGSIDE) {
            x = stpcpy(x, "O-O");
        } else if (move & CASTLE_QUEENSIDE) {
            x = stpcpy(x, "O-O-O");
        } else {
            *(x++) = piece_names[piece];

            push_section(heap);
            generate_to(heap, pos, to, piece);

            bool ambiguous = false;
            bool same_file = false;
            bool same_rank = false;

            for (int i = heap->current_section->start;
                 i < heap->current_section->end; i++) {
                uint32_t alt_move = heap->data[i];
                if (alt_move != move &&
                    move_piece(alt_move) == move_piece(move)) {
                    struct position tmp;
                    do_move(pos, &tmp, alt_move);
                    if (is_king_in_check(&tmp, !tmp.turn))
                        continue;

                    ambiguous = true;
                    if (file_of(move_from(alt_move)) ==
                        file_of(move_from(move)))
                        same_file = true;
                    if (rank_of(move_from(alt_move)) ==
                        rank_of(move_from(move)))
                        same_rank = true;
                }
            }
            pop_section(heap);

            if (ambiguous) {
                if (!same_file)
                    *(x++) = file_char(from);
                else {
                    if (!same_rank)
                        *(x++) = rank_char(from);
                    else {
                        *(x++) = file_char(from);
                        *(x++) = rank_char(from);
                    }
                }
            }

            if (move & CAPTURE) {
                *(x++) = 'x';
            }
            *(x++) = file_char(to);
            *(x++) = rank_char(to);
        }
    }

    struct position tmp;
    do_move(pos, &tmp, move);

    if (is_king_in_check(&tmp, tmp.turn)) {
        if (is_checkmate(heap, &tmp)) {
            *(x++) = '#';
        } else {
            *(x++) = '+';
        }
    }
    *x = 0;
}

int type_from_name(const char name) {
    for (int type = KNIGHT; type <= KING; type++) {
        if (piece_names[type] == name)
            return type;
    }
    return -1;
}

/**
 * Parse a string in standard algebraic notation to a move.
 *
 * @param san the string to parse
 * @param heap a heap to generate (temporary moves)
 * @param pos the position
 * @param move pointer to a move, will contain the parsed move
 * @return `true` if san could be parsed successfully, `false` otherwise
 */
bool parse_san(const char *san, heap_t heap, position_t pos, uint32_t *move) {
    if (0 == strncmp("O-O-O", san, 5)) {
        if (check_castling_rights(pos, false)) {
            int sq = 4 + (1 - pos->turn) * 56;
            *move = make_move(sq, sq - 2, MOVE_KING | CASTLE_QUEENSIDE);
            return true;
        } else
            return false;
    }
    if (0 == strncmp("O-O", san, 3)) {
        if (check_castling_rights(pos, true)) {
            int sq = 4 + (1 - pos->turn) * 56;
            *move = make_move(sq, sq + 2, MOVE_KING | CASTLE_KINGSIDE);
            return true;
        } else
            return false;
    }
    int to_file = -1, to_rank = -1, from_file = -1, from_rank = -1;
    int piece = 0;
    int promotion = 0;

    while (*san) {
        switch (*san) {
        case 'N':
        case 'B':
        case 'R':
        case 'Q':
        case 'K':
            if (piece == 0)
                piece = type_from_name(*san);
            else
                return false;
            break;
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
            from_file = to_file;
            to_file = *san - 'a';
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
            from_rank = to_rank;
            to_rank = *san - '1';
            break;
        case '=':
            san++;
            promotion = type_from_name(*san);
            if (promotion == -1)
                return false;
            break;
        case 'x':
        case '+':
        case '#':
            break;
        default:
            return false;
        }
        san++;
    }

    if (piece == 0)
        piece = PAWN;

    // printf("piece=%d, to_rank=%d, to_file=%d\n", piece, to_rank, to_file);

    push_section(heap);
    generate_pseudolegal_moves(heap, pos);

    uint32_t selected = 0;

    for (int i = heap->current_section->start; i < heap->current_section->end;
         i++) {
        struct position t;
        uint32_t candidate = heap->data[i];

        if (candidate & (CASTLE_KINGSIDE | CASTLE_QUEENSIDE)) {
            if (is_king_in_check(pos, pos->turn)) {
                continue;
            }
            if (!is_castle_legal(pos, candidate)) {
                continue;
            }
        }
        do_move(pos, &t, candidate);
        if (is_king_in_check(&t, !t.turn))
            continue;

        if (move_piece(candidate) != piece)
            continue;
        if (file_of(move_to(candidate)) != to_file)
            continue;
        if (rank_of(move_to(candidate)) != to_rank)
            continue;
        if (from_file != -1 && file_of(move_from(candidate)) != from_file)
            continue;
        if (from_rank != -1 && rank_of(move_from(candidate)) != from_rank)
            continue;
        if (promotion && move_promotion(candidate) != promotion)
            continue;

        if (selected != 0)
            return false; // ambiguous move
        selected = candidate;
    }

    pop_section(heap);

    if (selected) {
        *move = selected;
        return true;
    } else {
        return false;
    }
}
