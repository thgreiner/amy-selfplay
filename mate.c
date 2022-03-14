#include "heap.h"
#include "movegen.h"
#include "position.h"

#include <math.h>
#include <sys/time.h>

static const int INF = 10000;

static unsigned int nodes;

static int negascout(restrict heap_t heap, const restrict position_t pos,
                     int depth, int ply, int alpha, int beta) {

    int value = -INF;
    struct position t;

    if (depth <= 0) {
        return 0;
    }

    nodes++;

    push_section(heap);
    generate_pseudolegal_moves(heap, pos);

    for (int i = heap->current_section->start; i < heap->current_section->end;
         i++) {
        uint32_t move = heap->data[i];

        if (move & (CASTLE_KINGSIDE | CASTLE_QUEENSIDE)) {
            if (is_king_in_check(pos, pos->turn)) {
                continue;
            }
            if (!is_castle_legal(pos, move)) {
                continue;
            }
        }
        do_move(pos, &t, move);
        if (is_king_in_check(&t, !t.turn))
            continue;

        int v = -negascout(heap, &t, depth - 1, ply + 1, -beta, -alpha);

        if (v > value) {
            value = v;
            if (v >= beta) {
                break;
            }
            if (v > alpha) {
                alpha = v;
            }
        }
    }
    pop_section(heap);

    if (value == -INF) {
        if (is_king_in_check(pos, pos->turn)) {
            value = -INF + ply;
        } else {
            value = 0;
        }
    }

    return value;
}

uint32_t mate_search(restrict heap_t heap, const restrict position_t pos,
                     int max_depth, unsigned int budget) {

    struct timeval begin, end;
    gettimeofday(&begin, 0);

    uint32_t best_move = 0;
    int value;

    nodes = 0;

    struct position t;

    push_section(heap);
    generate_pseudolegal_moves(heap, pos);

    for (int depth = 1; depth < max_depth; depth += 2) {
        value = -INF;

        int alpha = INF - 100;
        int beta = INF;

        for (int i = heap->current_section->start;
             i < heap->current_section->end; i++) {
            uint32_t move = heap->data[i];

            if (move & (CASTLE_KINGSIDE | CASTLE_QUEENSIDE)) {
                if (is_king_in_check(pos, pos->turn)) {
                    continue;
                }
                if (!is_castle_legal(pos, move)) {
                    continue;
                }
            }
            do_move(pos, &t, move);
            if (is_king_in_check(&t, !t.turn))
                continue;

            int v = -negascout(heap, &t, depth, 1, -beta, -alpha);

            if (v > value) {
                value = v;
                if (v > alpha) {
                    alpha = v;
                }

                best_move = move;
            }
        }

        if (value > (INF - 400))
            break;

        if (budget != 0 && nodes > budget)
            break;
    }
    pop_section(heap);

    gettimeofday(&end, 0);
    long seconds = end.tv_sec - begin.tv_sec;
    long microseconds = end.tv_usec - begin.tv_usec;
    float elapsed = seconds + microseconds * 1e-6;

    printf("Mate search visited %u nodes in %.2fs.\n", nodes, elapsed);

    if (value < (INF - 400)) {
        best_move = 0;
    }

    return best_move;
}

const char *MATE_POSITIONS[] = {
    "8/8/8/8/8/R3K3/8/4k3 w - -", "8/8/8/8/5K2/8/R7/6k1 w - -",
    "r1bq2rk/pp3pbp/2p1p1pQ/7P/3P4/2PB1N2/PP3PPR/2KR4 w - -",
    "r2rb1k1/pp1q1p1p/2n1p1p1/2bp4/5P2/PP1BPR1Q/1BPN2PP/R5K1 w - -",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"};

void test_mate_search() {

    heap_t heap = allocate_heap();

    for (int i = 0; i < sizeof(MATE_POSITIONS) / sizeof(char *); i++) {
        const char *epd = MATE_POSITIONS[i];
        struct position pos;

        bool success = parse_epd(&pos, epd);

        print_position(&pos);
        mate_search(heap, &pos, 8, 50000);
    }
}
