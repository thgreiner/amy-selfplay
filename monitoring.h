#include <functional>
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

namespace monitoring {

class monitoring {
  public:
    std::function<void()> observe_node;
    std::function<void()> observe_terminal_node;
    std::function<void()> observe_game;
    std::function<void()> observe_position;
    std::function<void()> observe_checkmate;
    std::function<void()> observe_tbwinner;
    std::function<void(double)> observe_evaluation;
    std::function<void(int)> observe_depth;
    std::function<void(int)> observe_decision;

    static void initialize(std::string bind_address) {
        single_instance = new monitoring(bind_address);
    }
    static monitoring *instance() { return single_instance; }

  private:
    monitoring(std::string name) : exposer(name) { setup(); };
    void setup();
    prometheus::Exposer exposer;
    std::shared_ptr<prometheus::Registry> registry;

    static monitoring *single_instance;
};
} // namespace monitoring
