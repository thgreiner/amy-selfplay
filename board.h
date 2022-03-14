#ifndef BOARD_H
#define BOARD_H

#include <memory>
#include <string>
#include <vector>

#include "heap.h"
#include "position.h"

class Board {
  public:
    Board();
    Board(std::string &);
    void generate_legal_moves(std::vector<uint32_t> &moves) const;
    void do_move(uint32_t move);
    void undo_move();
    std::string san(uint32_t) const;
    std::string variation_san(const std::vector<uint32_t> &variation);
    std::string outcome() const;

    ~Board();

    position_t current_position() const;
    bool turn() const;
    int move_number() const;

    bool is_in_check() const;
    bool is_repeated(int count) const;
    bool is_insufficient_material() const;
    bool is_fifty_move_rule() const;

    bool game_ended() const;

    std::string move_number_if_white() const;
    std::string epd() const;

    void print() const;

    bool parse_san(std::string &, uint32_t &);

    uint32_t search_checkmate(int depth, uint64_t budget);

  private:
    std::vector<std::shared_ptr<struct position>> positions;
    heap_t heap;
};

#endif
