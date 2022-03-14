#include <string>

#include "board.h"
#include "mate.h"
#include "movegen.h"

Board::Board() {
    std::shared_ptr<struct position> p = std::make_shared<struct position>();
    parse_epd(p.get(), INITIAL_POSITION_EPD);
    positions.push_back(p);

    heap = allocate_heap();
}

Board::Board(std::string &epd) {
    std::shared_ptr<struct position> p = std::make_shared<struct position>();
    parse_epd(p.get(), epd.c_str());
    positions.push_back(p);

    heap = allocate_heap();
}

Board::~Board() { free_heap(heap); }

void Board::generate_legal_moves(std::vector<uint32_t> &moves) const {
    position_t pos = positions.back().get();

    push_section(heap);
    generate_pseudolegal_moves(heap, pos);

    for (int i = heap->current_section->start; i < heap->current_section->end;
         i++) {

        struct position t;
        uint32_t move = heap->data[i];

        if (move & (CASTLE_KINGSIDE | CASTLE_QUEENSIDE)) {
            if (is_king_in_check(pos, pos->turn)) {
                continue;
            }
            if (!is_castle_legal(pos, move)) {
                continue;
            }
        }
        ::do_move(pos, &t, move);
        if (is_king_in_check(&t, !t.turn))
            continue;

        moves.push_back(move);
    }

    pop_section(heap);
}

void Board::do_move(uint32_t move) {
    position_t pos = positions.back().get();
    std::shared_ptr<struct position> next = std::make_shared<struct position>();

    ::do_move(pos, next.get(), move);

    positions.push_back(next);
}

void Board::undo_move() { positions.pop_back(); }

std::string Board::san(uint32_t move) const {
    position_t pos = positions.back().get();

    static char buffer[16];
    ::san(buffer, heap, pos, move);

    return std::string(buffer);
}

position_t Board::current_position() const { return positions.back().get(); }

bool Board::turn() const { return current_position()->turn; }

bool Board::is_in_check() const {
    return is_king_in_check(current_position(), turn());
}

bool Board::is_repeated(int count) const {
    return ::is_repeated(current_position(), count);
}

bool Board::is_insufficient_material() const {
    return ::is_insufficient_material(current_position());
}

bool Board::is_fifty_move_rule() const {
    return current_position()->irrev_count >= 100;
}

std::string Board::move_number_if_white() const {
    if (turn()) {
        return std::to_string(move_number()) + ". ";
    } else {
        return "";
    }
}

void Board::print() const { print_position(current_position()); }

std::string Board::variation_san(const std::vector<uint32_t> &variation) {
    std::string buffer;

    if (!turn()) {
        buffer += std::to_string(move_number()) + "... ";
    }

    for (auto move : variation) {
        buffer += move_number_if_white() + san(move) + " ";
        do_move(move);
    }

    for (auto move : variation)
        undo_move();

    return buffer;
}

int Board::move_number() const { return 1 + current_position()->ply / 2; }

bool Board::game_ended() const {
    if (is_insufficient_material())
        return true;
    if (is_repeated(3))
        return true;
    if (is_fifty_move_rule()) {
        return true;
    }

    std::vector<uint32_t> legal_moves;
    generate_legal_moves(legal_moves);
    return legal_moves.size() == 0;
}

std::string Board::outcome() const {
    if (is_insufficient_material() || is_repeated(3) || is_fifty_move_rule()) {
        return "1/2-1/2";
    }

    std::vector<uint32_t> legal_moves;
    generate_legal_moves(legal_moves);

    if (legal_moves.size() == 0) {
        if (is_in_check()) {
            return turn() ? "0-1" : "1-0";
        } else {
            return "1/2-1/2";
        }
    }

    return "*";
}

std::string Board::epd() const {
    char buffer[256];
    ::to_epd(current_position(), buffer);

    return buffer;
}

bool Board::parse_san(std::string &san, uint32_t &move) {
    return ::parse_san(san.c_str(), heap, current_position(), &move);
}

uint32_t Board::search_checkmate(int depth, uint64_t budget) {
    return ::mate_search(heap, current_position(), depth, budget);
}
