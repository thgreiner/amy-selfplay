#ifdef __cplusplus
extern "C" {
#endif

extern const char *PERFT_POSITIONS[10];
extern const uint64_t expected_visits[][7];

void perft(int max_depth);
uint64_t test_move_gen_speed(heap_t heap, position_t pos, int depth);

#ifdef __cplusplus
}
#endif
