#include "heap.h"
#include "movegen.h"
#include "position.h"

#include <math.h>
#include <sys/time.h>

const char *PERFT_POSITIONS[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -",
    "8/3k4/6N1/8/2N3N1/3K4/8/8 w - -",
    "8/3k3b/6N1/8/2N3N1/3K4/8/8 w - - ",
    "5k2/8/2R2K2/8/8/8/8/8 w - -",
    "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq d6"};

const uint64_t expected_visits[][7] = {
    {1, 20, 400, 8902, 197281, 4865609, 119060324},
    {1, 48, 2039, 97862, 4085603, 193690690, 8031647685ULL},
    {1, 14, 191, 2812, 43238, 674624, 11030083},
    {1, 6, 264, 9467, 422333, 15833292, 706045033},
    {1, 44, 1486, 62379, 2103487, 89941194, 3048196529},
    {1, 46, 2079, 89890, 3894594, 164075551, 6923051137},
    {1, 27, 164, 4185, 19132, 471178, 2496867},
    {1, 21, 161, 3288, 25206, 528832, 5131014},
    {1, 16, 40, 771, 2876, 57307, 219980},
    {1, 31, 704, 21542, 519896, 16284091, 416503988}};

#ifdef ENABLE_STATS
static uint64_t captures;
static uint64_t en_passant;
static uint64_t promotions;
static uint64_t castles;
static uint64_t checks;
static int print_depth;
#endif

static uint64_t recurse(restrict heap_t heap, const restrict position_t pos,
                        int depth) {
    struct position t;

    uint64_t visits = 0;

    if (depth <= 0) {
        return 1;
    }

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

#ifdef ENABLE_STATS
        if (depth == 1) {
            if (move & EN_PASSANT)
                en_passant++;
            if (move & CAPTURE)
                captures++;
            if (move_promotion(move))
                promotions++;
            if (move & (CASTLE_KINGSIDE | CASTLE_QUEENSIDE))
                castles++;
            if (is_king_in_check(&t, t.turn))
                checks++;
        }
#endif

        uint64_t v = recurse(heap, &t, depth - 1);

        visits += v;

#ifdef ENABLE_STATS
        if (depth == print_depth) {
            print_move(move);
            printf("  %llu\n", v);
        }
#endif
    }
    pop_section(heap);

    return visits;
}

uint64_t test_move_gen_speed(heap_t heap, position_t pos, int depth) {

#ifdef ENABLE_STATS
    captures = 0;
    en_passant = 0;
    promotions = 0;
    castles = 0;
    checks = 0;
    print_depth = depth;
#endif

    uint64_t result = recurse(heap, pos, depth);

#ifdef ENABLE_STATS
    printf("Captures: %llu\n", captures + en_passant);
    printf("E.p.: %llu\n", en_passant);
    printf("Promotions: %llu\n", promotions);
    printf("Castles: %llu\n", castles);
    printf("Checks: %llu\n", checks);
#endif

    return result;
}

void perft(int max_depth) {
    heap_t heap = allocate_heap();

    for (int i = 0; i < sizeof(PERFT_POSITIONS) / sizeof(char *); i++) {
        const char *epd = PERFT_POSITIONS[i];
        struct position pos;

        bool success = parse_epd(&pos, epd);

        if (!success) {
            printf("Illegal EPD!\n");
            continue;
        }

        print_position(&pos);

        printf("Depth       Nodes  NPS\n");
        for (int depth = 0; depth < max_depth; depth++) {
            struct timeval begin, end;
            gettimeofday(&begin, 0);

            uint64_t visits = test_move_gen_speed(heap, &pos, depth);

            gettimeofday(&end, 0);

            long seconds = end.tv_sec - begin.tv_sec;
            long microseconds = end.tv_usec - begin.tv_usec;
            float elapsed = seconds + microseconds * 1e-6;
            float nps = visits / elapsed;

            const char *unit = "";
            if (!isinf(nps) && nps >= 1e6) {
                nps *= 1e-6;
                unit = "M";
            } else if (!isinf(nps) && nps >= 1e3) {
                nps *= 1e-3;
                unit = "k";
            }

            printf("%d %15llu  %.3f%s\n", depth + 1, visits, nps, unit);

            if (visits != expected_visits[i][depth]) {
                printf("!!!\n");
            }
        }
    }

    free_heap(heap);
}
