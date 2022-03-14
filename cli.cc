#include <iostream>
#include <string>

#include "edgetpu.h"
#include "mcts.h"

void cli(std::string model_name, const int sims) {

    std::shared_ptr<EdgeTpuModel> model =
        std::make_shared<EdgeTpuModel>(model_name);

    MCTS mcts(model);
    mcts.use_exploration_noise(false);
    mcts.use_forced_playouts(true);

    Board b;

    std::string line;

    while (!b.game_ended()) {
        uint32_t move;

        std::cout << "> ";
        if (!std::getline(std::cin, line))
            break;

        if (b.parse_san(line, move)) {
            b.do_move(move);

            const std::shared_ptr<Node> root = mcts.mcts(b, sims);

            move = select_most_visited_move(root);

            std::cout << std::endl;
            std::cout << std::endl;
            std::cout << "  >> " << b.san(move) << " << " << std::endl;

            b.do_move(move);
            b.print();
        } else if ("go" == line) {
            const std::shared_ptr<Node> root = mcts.mcts(b, sims);

            move = select_most_visited_move(root);

            std::cout << std::endl;
            std::cout << std::endl;
            std::cout << "  >> " << b.san(move) << " << " << std::endl;

            b.do_move(move);
            b.print();
        } else if ("undo" == line) {
            b.undo_move();
        } else if ("d" == line) {
            b.print();
        } else {
            std::cout << "???" << std::endl;
        }
    }
}
