#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <sys/stat.h>
#include <sys/time.h>

#include "edgetpu_c.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#include "bits.h"
#include "edgetpu.h"
#include "heap.h"
#include "movegen.h"
#include "position.h"

namespace {

std::vector<float> Dequantize(const TfLiteTensor &tensor) {
    const auto *data = reinterpret_cast<const int8_t *>(tensor.data.data);
    std::vector<float> result(tensor.bytes);
    for (int i = 0; i < tensor.bytes; ++i)
        result[i] = tensor.params.scale * (data[i] - tensor.params.zero_point);
    return result;
}

} // namespace

std::unique_ptr<tflite::Interpreter>
make_interpreter(const std::string model_file) {

    // Find TPU device.
    size_t num_devices;
    std::unique_ptr<edgetpu_device, decltype(&edgetpu_free_devices)> devices(
        edgetpu_list_devices(&num_devices), &edgetpu_free_devices);

    if (num_devices == 0) {
        std::cerr << "No connected TPU found" << std::endl;
        return nullptr;
    }
    const auto &device = devices.get()[0];

    // Load model.
    auto model = tflite::FlatBufferModel::BuildFromFile(model_file.c_str());
    if (!model) {
        std::cerr << "Cannot read model from " << model_file << std::endl;
        return nullptr;
    }

    // Create interpreter.
    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;
    if (tflite::InterpreterBuilder(*model, resolver)(&interpreter) !=
        kTfLiteOk) {
        std::cerr << "Cannot create interpreter" << std::endl;
        return nullptr;
    }

    auto *delegate =
        edgetpu_create_delegate(device.type, device.path, nullptr, 0);
    interpreter->ModifyGraphWithDelegate(delegate);

    // Allocate tensors.
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        std::cerr << "Cannot allocate interpreter tensors" << std::endl;
        return nullptr;
    }

    return interpreter;
}

EdgeTpuModel::EdgeTpuModel(const std::string model_file) {
    model_name = model_file;
    interpreter = make_interpreter(model_file);

    if (!interpreter)
        throw std::runtime_error("Could not initialize EdgeTPu.");

    mtime = get_modification_time();
}

time_t EdgeTpuModel::get_modification_time() {
    struct stat buf;
    if (stat(model_name.c_str(), &buf) == 0) {
        return buf.st_mtime;
    }
    return -1;
}

bool EdgeTpuModel::has_changed_on_disc() {
    return mtime != get_modification_time();
}

void EdgeTpuModel::test() {

    // Set interpreter input.
    const auto *input_tensor = interpreter->input_tensor(0);

    std::cout << "Input tensor height: " << input_tensor->dims->data[1]
              << std::endl;
    std::cout << "Input tensor width:  " << input_tensor->dims->data[2]
              << std::endl;
    std::cout << "Input tensor bpp:    " << input_tensor->dims->data[3]
              << std::endl;
    std::cout << "Input tensor zero point: " << input_tensor->params.zero_point
              << std::endl;
    std::cout << "Input tensor scale     : " << input_tensor->params.scale
              << std::endl;

    std::vector<int8_t> image(
        8 * 8 *
        19); // input_tensor->dims->data[1] * input_tensor->dims->data[2] *
             // input_tensor->dims->data[3]);

    // std::cout << image << std::endl;

    /*
      if (input_tensor->type != kTfLiteUInt8 ||           //
          input_tensor->dims->data[0] != 1 ||             //
          input_tensor->dims->data[1] != image_height ||  //
          input_tensor->dims->data[2] != image_width ||   //
          input_tensor->dims->data[3] != image_bpp) {
        std::cerr << "Input tensor shape does not match input image" <<
      std::endl; return 1;
      }
    */
    struct timeval begin, end;
    gettimeofday(&begin, 0);

    auto n = 100;

    for (int i = 0; i < n; i++) {
        std::transform(image.begin(), image.end(),
                       interpreter->typed_input_tensor<int8_t>(0),
                       [input_tensor](int8_t x) -> int8_t {
                           return x / input_tensor->params.scale +
                                  input_tensor->params.zero_point;
                       });

        // Run inference.
        if (interpreter->Invoke() != kTfLiteOk) {
            std::cerr << "Cannot invoke interpreter" << std::endl;
            return;
        }

        auto policy = Dequantize(*interpreter->output_tensor(0));
        auto value = Dequantize(*interpreter->output_tensor(1));
        // std::cout << value[0] << std::endl;

        auto wdl = Dequantize(*interpreter->output_tensor(2));
        // std::cout << wdl[0] << ", " << wdl[1] << ", " << wdl[2]<< std::endl;
    }

    gettimeofday(&end, 0);

    long seconds = end.tv_sec - begin.tv_sec;
    long microseconds = end.tv_usec - begin.tv_usec;
    float elapsed = seconds + microseconds * 1e-6;

    std::cout << "Inference took " << elapsed << "secs." << std::endl;

    std::cout << " = " << (n / elapsed) << " 1/s." << std::endl;
}

static void fill_plane_bb(std::vector<float> &data, int plane, uint64_t bb,
                          int eor) {
    while (bb) {
        int sq = poplsb(&bb);
        data[19 * (sq ^ eor) + plane] = 1.0f;
    }
}

static void fill_plane(std::vector<float> &data, int plane,
                       float fill_value = 1.0) {
    for (auto sq = 0; sq < 64; sq++) {
        data[19 * sq + plane] = fill_value;
    }
}

void EdgeTpuModel::predict(position_t pos) {
    std::vector<float> data(8 * 8 * 19);
    ChessRepr r;

    int plane = 0;

    bool me = pos->turn;
    bool you = !me;

    int eor = me ? 0 : 0x38;

    for (auto type = PAWN; type <= KING; type++) {
        fill_plane_bb(data, plane++, pos->by_type[type] & pos->by_color[me],
                      eor);
        fill_plane_bb(data, plane++, pos->by_type[type] & pos->by_color[you],
                      eor);
    }

    fill_plane_bb(data, plane++, pos->en_passant, eor);

    if (pos->can_castle[me][true])
        fill_plane(data, plane);
    plane++;
    if (pos->can_castle[me][false])
        fill_plane(data, plane);
    plane++;
    if (pos->can_castle[you][true])
        fill_plane(data, plane);
    plane++;
    if (pos->can_castle[you][false])
        fill_plane(data, plane);
    plane++;

    fill_plane(data, plane);
    plane++;

    fill_plane(data, plane, pos->irrev_count / 100.);
    plane++;

    assert(plane == 19);
    // std::cout << "Filled " << plane << " planes." << std::endl;

    // for (int i=0; i<data.size(); i++) {
    //     std::cout << i << ": " << data[i] << std::endl;
    // }

    // Set interpreter input.
    const auto *input_tensor = interpreter->input_tensor(0);

    std::transform(data.begin(), data.end(),
                   interpreter->typed_input_tensor<int8_t>(0),
                   [input_tensor](float x) -> int8_t {
                       return x / input_tensor->params.scale +
                              input_tensor->params.zero_point;
                   });

    // Run inference.
    if (interpreter->Invoke() != kTfLiteOk) {
        std::cerr << "Cannot invoke interpreter" << std::endl;
        return;
    }
}

float EdgeTpuModel::get_value() {
    auto value = Dequantize(*interpreter->output_tensor(1));
    return 0.5 * (value[0] + 1.0);
}

float EdgeTpuModel::get_logit(uint32_t move, int eor) {
    int pidx = repr.plane_index(move, eor);
    int index = 73 * (move_to(move) ^ eor) + pidx;

    auto logits = *interpreter->output_tensor(0);
    const auto *data = reinterpret_cast<const int8_t *>(logits.data.data);
    return logits.params.scale * (data[index] - logits.params.zero_point);
}

ChessRepr::ChessRepr() {
    int idx = 0;

    static int queen_dirs[] = {-1, 1, -8, 8, -7, 7, -9, 9};
    static int knight_dirs[] = {-15, 15, -17, 17, -10, 10, -6, 6};
    static int pawn_dirs[] = {7, 8, 9};
    static int pieces[] = {KNIGHT, BISHOP, ROOK};

    for (auto dir : queen_dirs) {
        for (auto i = 1; i < 8; i++) {
            auto delta = i * dir;
            queen_indexes[delta] = idx++;
        }
    }

    for (auto delta : knight_dirs) {
        knight_indexes[delta] = idx++;
    }

    for (auto delta : pawn_dirs) {
        for (auto piece : pieces) {
            underpromo_indexes[piece][delta] = idx++;
        }
    }

    assert(idx == 73);
}

bool ChessRepr::is_knight_move(int from_square, int to_square) {
    int file_dist = std::abs((from_square & 7) - (to_square & 7));
    int rank_dist = std::abs((from_square >> 3) - (to_square >> 3));

    return (file_dist == 1 && rank_dist == 2) ||
           (file_dist == 2 && rank_dist == 1);
}

int ChessRepr::plane_index(uint32_t move, int _xor) {
    auto delta = (move_to(move) ^ _xor) - (move_from(move) ^ _xor);
    if (move_promotion(move) && move_promotion(move) != QUEEN) {
        return underpromo_indexes[move_promotion(move)][delta];
    } else if (is_knight_move(move_from(move), move_to(move))) {
        return knight_indexes[delta];
    } else {
        return queen_indexes[delta];
    }
}
