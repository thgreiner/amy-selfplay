#include "position.h"
#include "attacks.h"
#include "bits.h"

#include <ctype.h>
#include <stdio.h>

// Initial position as EPD.
const char *INITIAL_POSITION_EPD =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

/**
 * Parse a position encoded in an EPD string to a position structure.
 */
bool parse_epd(position_t p, const char *epd) {
    p->by_color[1] = 0;
    p->by_color[0] = 0;
    p->by_type[PAWN] = 0;
    p->by_type[KNIGHT] = 0;
    p->by_type[BISHOP] = 0;
    p->by_type[ROOK] = 0;
    p->by_type[QUEEN] = 0;
    p->by_type[KING] = 0;
    p->en_passant = 0;

    int rk = 7;
    int fl = 0;

    const char *x = epd;
    for (; *x != ' '; x++) {
        switch (*x) {
        case 'P':
            p->by_color[1] |= set_mask(8 * rk + fl);
            p->by_type[PAWN] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'p':
            p->by_color[0] |= set_mask(8 * rk + fl);
            p->by_type[PAWN] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'N':
            p->by_color[1] |= set_mask(8 * rk + fl);
            p->by_type[KNIGHT] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'n':
            p->by_color[0] |= set_mask(8 * rk + fl);
            p->by_type[KNIGHT] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'B':
            p->by_color[1] |= set_mask(8 * rk + fl);
            p->by_type[BISHOP] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'b':
            p->by_color[0] |= set_mask(8 * rk + fl);
            p->by_type[BISHOP] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'R':
            p->by_color[1] |= set_mask(8 * rk + fl);
            p->by_type[ROOK] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'r':
            p->by_color[0] |= set_mask(8 * rk + fl);
            p->by_type[ROOK] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'Q':
            p->by_color[1] |= set_mask(8 * rk + fl);
            p->by_type[QUEEN] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'q':
            p->by_color[0] |= set_mask(8 * rk + fl);
            p->by_type[QUEEN] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'K':
            p->by_color[1] |= set_mask(8 * rk + fl);
            p->by_type[KING] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case 'k':
            p->by_color[0] |= set_mask(8 * rk + fl);
            p->by_type[KING] |= set_mask(8 * rk + fl);
            fl++;
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
            fl += *x - '0';
            break;
        case '/':
            rk -= 1;
            fl = 0;
            break;
        default:
            return false;
        }
    }
    x++;
    if (*x == 'w') {
        p->turn = true;
        p->ply = 0;
    } else if (*x == 'b') {
        p->turn = false;
        p->ply = 1;
    } else {
        return false;
    }

    if (*(++x) != ' ') {
        return false;
    }
    x++;

    p->can_castle[0][0] = p->can_castle[1][0] = p->can_castle[0][1] =
        p->can_castle[1][1] = false;

    for (; *x != ' '; x++) {
        switch (*x) {
        case 'K':
            p->can_castle[1][1] = true;
            break;
        case 'Q':
            p->can_castle[1][0] = true;
            break;
        case 'k':
            p->can_castle[0][1] = true;
            break;
        case 'q':
            p->can_castle[0][0] = true;
            break;
        case '-':
            break;
        default:
            return false;
        }
    }
    x++;

    if (*x == '-') {
        p->en_passant = 0;
    } else {
        if (*x < 'a' && *x > 'h')
            return false;
        int ep = *x - 'a';
        x++;
        if (*x < '1' && *x > '8')
            return false;
        ep |= (*x - '1') << 3;
        p->en_passant = set_mask(ep);
    }

    if (is_king_in_check(p, !p->turn)) {
        return false;
    }

    p->irrev_count = 0;
    p->prev = NULL;

    return true;
}

/**
 * Check if square _sq_ is attacked by side _turn_.
 */
bool is_square_attacked_by(restrict position_t p, int sq, bool turn) {
    const uint64_t attackers = p->by_color[turn];
    const uint64_t blockers = all_pieces(p);

    if (pawn_attacks(sq, !turn) & attackers & p->by_type[PAWN]) {
        // printf("P\n");
        return true;
    }
    if (knight_attacks(sq) & attackers & p->by_type[KNIGHT]) {
        // printf("N\n");
        return true;
    }
    if (BISHOP_ATTACKS(sq, blockers) & attackers &
        (p->by_type[BISHOP] | p->by_type[QUEEN])) {
        // printf("B/Q\n");
        return true;
    }
    if (ROOK_ATTACKS(sq, blockers) & attackers &
        (p->by_type[ROOK] | p->by_type[QUEEN])) {
        // printf("attacks by %d:\n", turn);
        // print_bitboard(ROOK_ATTACKS(sq, blockers) & attackers);
        // printf("R/Q\n");
        return true;
    }
    if (king_attacks(sq) & attackers & p->by_type[KING]) {
        // printf("K\n");
        return true;
    }

    return false;
}

bool is_king_in_check(position_t p, bool turn) {
    const uint64_t king = p->by_color[turn] & p->by_type[KING];
    const int sq = ctzll(king);

    return is_square_attacked_by(p, sq, !turn);
}

char piece_on(position_t p, int sq) {
    if (is_bit_set(p->by_type[PAWN], sq))
        return 'P';
    if (is_bit_set(p->by_type[KNIGHT], sq))
        return 'N';
    if (is_bit_set(p->by_type[BISHOP], sq))
        return 'B';
    if (is_bit_set(p->by_type[ROOK], sq))
        return 'R';
    if (is_bit_set(p->by_type[QUEEN], sq))
        return 'Q';
    if (is_bit_set(p->by_type[KING], sq))
        return 'K';

    return ' ';
}

bool is_white_on(position_t p, int sq) {
    return is_bit_set(p->by_color[1], sq);
}

bool is_black_on(position_t p, int sq) {
    return is_bit_set(p->by_color[0], sq);
}

bool is_enpassant_on(position_t p, int sq) {
    return is_bit_set(p->en_passant, sq);
}

void print_position(position_t p) {
    for (int row = 7; row >= 0; row--) {
        printf("+---+---+---+---+---+---+---+---+\n|");
        for (int col = 0; col < 8; col++) {
            int sq = col + row * 8;
            char piece = piece_on(p, sq);
            if (is_enpassant_on(p, sq))
                piece = '.';

            if (is_black_on(p, sq))
                printf("*%c*", piece);
            else if (is_white_on(p, sq) || is_enpassant_on(p, sq))
                printf(" %c ", piece);
            else
                printf("   ");
            printf("|");
        }
        if (row == 7 || row == 0) {
            printf(" Castle: ");
            if (p->can_castle[row == 0][1])
                printf("K");
            if (p->can_castle[row == 0][0])
                printf("Q");
        }
        printf("\n");
    }
    printf("+---+---+---+---+---+---+---+---+\n");
}

void to_epd(position_t p, char *buffer) {
    char *c = buffer;
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        if (rank < 7)
            *(c++) = '/';
        for (int file = 0; file < 8; file++) {
            int piece = piece_on(p, 8 * rank + file);
            if (piece != ' ') {
                if (empty != 0) {
                    *(c++) = '0' + empty;
                    empty = 0;
                }
                if (is_black_on(p, 8 * rank + file))
                    *(c++) = tolower(piece);
                else
                    *(c++) = piece;
            } else
                empty++;
        }
        if (empty != 0)
            *(c++) = '0' + empty;
    }

    *(c++) = ' ';
    *(c++) = (p->turn) ? 'w' : 'b';
    *(c++) = ' ';

    bool any_castling_rights = p->can_castle[0][0] | p->can_castle[0][1] |
                               p->can_castle[1][0] | p->can_castle[1][1];
    if (any_castling_rights) {
        if (p->can_castle[true][true])
            *(c++) = 'K';
        if (p->can_castle[true][false])
            *(c++) = 'Q';
        if (p->can_castle[false][true])
            *(c++) = 'k';
        if (p->can_castle[false][false])
            *(c++) = 'q';
    } else
        *(c++) = '-';

    *(c++) = ' ';

    if (p->en_passant) {
        int ep = ctzll(p->en_passant);
        *(c++) = 'a' + (ep & 7);
        *(c++) = '1' + (ep / 8);
    } else
        *(c++) = '-';

    *c = '\0';
}

const static uint64_t dark_squares = 0xaa55aa55aa55aa55ULL;
const static uint64_t light_squares = 0x55aa55aa55aa55aaULL;

bool is_insufficient_material(position_t p) {
    if (p->by_type[PAWN] || p->by_type[ROOK] || p->by_type[QUEEN]) {
        return false;
    }
    if (!p->by_type[KNIGHT]) {
        const int dark_squared_bishops =
            __builtin_popcountll(p->by_type[BISHOP] & dark_squares);
        const int light_squared_bishops =
            __builtin_popcountll(p->by_type[BISHOP] & light_squares);
        return dark_squared_bishops == 0 || light_squared_bishops == 0;
    } else {
        if (p->by_type[BISHOP])
            return false;
        const int knight_count = __builtin_popcountll(p->by_type[KNIGHT]);
        return knight_count > 0;
    }
}

bool is_repeated(position_t restrict p, int count) {
    int repetitions_found = 1;
    position_t q = p;

    while (p->prev) {
        q = q->prev;

        if (p->turn == q->turn && p->by_color[1] == q->by_color[1] &&
            p->by_color[0] == q->by_color[0] &&
            p->by_type[PAWN] == q->by_type[PAWN] &&
            p->by_type[KNIGHT] == q->by_type[KNIGHT] &&
            p->by_type[ROOK] == q->by_type[ROOK] &&
            p->by_type[QUEEN] == q->by_type[QUEEN] &&
            p->by_type[KING] == q->by_type[KING] &&
            p->can_castle[0][0] == q->can_castle[0][0] &&
            p->can_castle[0][1] == q->can_castle[0][1] &&
            p->can_castle[1][0] == q->can_castle[1][0] &&
            p->can_castle[1][1] == q->can_castle[1][1]) {
            repetitions_found += 1;
            if (repetitions_found == count) {
                return true;
            }
        }
        if (q->irrev_count == 0)
            break;
    }

    return false;
}
