#include "bits.h"
#include <iostream>

#define NEW
#define XX 128

#define C_PIECES 3

#define SqFindKing(psq) (psq[C_PIECES * (x_pieceKing - 1)])
#define SqFindOne(psq, p) (psq[C_PIECES * (p - 1)])
#define SqFindFirst(psq, p) (psq[C_PIECES * (p - 1)])
#define SqFindSecond(psq, p) (psq[C_PIECES * (p - 1) + 1])
#define SqFindThird(psq, p) (psq[C_PIECES * (p - 1) + 2])

typedef int square;
typedef unsigned int INDEX;

#include "position.h"
#include "tbindex.cpp"

#define EGTB_CACHE_SIZE 32 * 1024 * 1024

static int EGTBMenCount;

void initEGTB(char *tbpath) {
    TB_CRC_CHECK = 0;
    EGTBMenCount = IInitializeTb(tbpath);
    if (EGTBMenCount != 0) {
        void *egtb_cache = malloc(EGTB_CACHE_SIZE);
        std::cout << "Found " << EGTBMenCount << "-men endgame table bases."
                  << std::endl;
        FTbSetCacheSize(egtb_cache, EGTB_CACHE_SIZE);
    }
}

void initializeCounters(int *pieceCounter, int *squares, int type,
                        uint64_t mask) {
    int count = 0;
    while (mask) {
        int index = poplsb(&mask);
        squares[type * C_PIECES + count] = index;
        count++;
    }
    *pieceCounter = count;
}

int probeEGTB(position_t p, int *score) {
    int pcCount[10];
    int wSquares[16], bSquares[16];
    int iTB;
    int color;
    int invert;
    int *wp, *bp;
    int ep;
    INDEX index;
    int value;
    int result;

    if (__builtin_popcountll(all_pieces(p)) > EGTBMenCount)
        return 0;

    initializeCounters(pcCount, wSquares, 0, p->by_color[1] & p->by_type[PAWN]);
    initializeCounters(pcCount + 1, wSquares, 1,
                       p->by_color[1] & p->by_type[KNIGHT]);
    initializeCounters(pcCount + 2, wSquares, 2,
                       p->by_color[1] & p->by_type[BISHOP]);
    initializeCounters(pcCount + 3, wSquares, 3,
                       p->by_color[1] & p->by_type[ROOK]);
    initializeCounters(pcCount + 4, wSquares, 4,
                       p->by_color[1] & p->by_type[QUEEN]);
    initializeCounters(pcCount + 5, bSquares, 0,
                       p->by_color[0] & p->by_type[PAWN]);
    initializeCounters(pcCount + 6, bSquares, 1,
                       p->by_color[0] & p->by_type[KNIGHT]);
    initializeCounters(pcCount + 7, bSquares, 2,
                       p->by_color[0] & p->by_type[BISHOP]);
    initializeCounters(pcCount + 8, bSquares, 3,
                       p->by_color[0] & p->by_type[ROOK]);
    initializeCounters(pcCount + 9, bSquares, 4,
                       p->by_color[0] & p->by_type[QUEEN]);

    do {
        iTB = IDescFindFromCounters(pcCount);
        if (iTB == 0) {
            result = 0;
            break;
        }

        wSquares[15] = ctzll(p->by_color[1] & p->by_type[KING]);
        bSquares[15] = ctzll(p->by_color[0] & p->by_type[KING]);

        if (iTB > 0) {
            color = p->turn ? 0 : 1;
            invert = 0;
            wp = wSquares;
            bp = bSquares;
        } else {
            color = p->turn ? 1 : 0;
            invert = 1;
            wp = bSquares;
            bp = wSquares;
            iTB = -iTB;
        }

        if (!FRegisteredFun(iTB, color)) {
            result = 0;
            break;
        }

        ep = p->en_passant ? ctzll(p->en_passant) : XX;
        index = PfnIndCalcFun(iTB, color)(wp, bp, ep, invert);
        value = L_TbtProbeTable(iTB, color, index);
        if (value == bev_broken) {
            result = 0;
            break;
        }

        if (value > 0) {
            int distance = 32767 - value;
            value = (INF - (2 * distance + 1));
        } else if (value < 0) {
            int distance = 32766 + value;
            value = -INF + 2 * distance;
        }

        *score = value;

        result = 1;
    } while (0);

    return result;
}

#include "board.h"

uint32_t tb_move(Board &board) {
    uint32_t best_move = 0;
    int best_score = -INF;
    int result, score = 0;

    std::vector<uint32_t> moves;
    board.generate_legal_moves(moves);

    for (auto m : moves) {
        board.do_move(m);
        result = probeEGTB(board.current_position(), &score);
        board.undo_move();

        if (result && (-score) > best_score) {
            best_score = -score;
            best_move = m;
        }
    }

    return best_move;
}

uint32_t tb_winner(Board &board) {
    uint32_t best_move = 0;
    int best_score = -INF;
    int result, score = 0;

    std::vector<uint32_t> moves;
    board.generate_legal_moves(moves);

    for (auto m : moves) {
        board.do_move(m);
        result = probeEGTB(board.current_position(), &score);
        board.undo_move();

        if (result && (-score) > best_score) {
            best_score = -score;
            best_move = m;
        }
    }

    return best_score > 0 ? best_move : 0;
}

void testEGTB() {
    initEGTB("TB");

    std::string p = "4k3/4pp2/8/8/8/8/5P2/4K3 w - -";
    Board board(p);

    int score = 0;
    int result;

    while (!board.game_ended()) {
        uint32_t move = tb_move(board);

        if (move == 0)
            break;

        board.print();
        std::cout << board.san(move) << std::endl;
        board.do_move(move);
    }
}
