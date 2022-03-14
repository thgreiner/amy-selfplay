#ifndef EDGETPU_H
#define EDGETPU_H

#include <map>
#include <string>

#include "tensorflow/lite/interpreter.h"

#include "position.h"

class ChessRepr {
  public:
    ChessRepr();
    int plane_index(uint32_t, int);

  private:
    bool is_knight_move(int from_square, int to_square);
    std::map<int, int> queen_indexes;
    std::map<int, int> knight_indexes;
    std::map<int, std::map<int, int>> underpromo_indexes;
};

class EdgeTpuModel {
  public:
    EdgeTpuModel(const std::string model_name);
    void predict(position_t);
    void test();
    bool has_changed_on_disc();

    float get_value();
    float get_logit(uint32_t move, int eor);

  private:
    std::string model_name;
    std::unique_ptr<tflite::Interpreter> interpreter;
    ChessRepr repr;
    time_t mtime;

    time_t get_modification_time();
};

#endif
