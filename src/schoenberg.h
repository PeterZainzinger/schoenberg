#include <string>
#include <utility>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <linux/input.h>

using namespace std;

namespace schoenberg {

    class KeyTarget {
    public:
        int key;
        int mod;

        KeyTarget() {};

        KeyTarget(int key, int mod) : key(key), mod(mod) {}
    };

    class LayerConfig {

    public:
        string prefix;

        std::map<int, KeyTarget> keys;


        LayerConfig(const string &prefix, const map<int, KeyTarget> &keys) : prefix(prefix), keys(keys) {}

    };

    class Config {
    public:
        std::vector<LayerConfig> layers;

        Config(std::vector<LayerConfig> layers) : layers(std::move(layers)) {}

    };

    class LayerState {

    public:
        bool active;

        bool used;
        bool written;

        std::map<int, KeyTarget> keys;

        LayerState() {};


        LayerState(bool active, bool used, bool written, const map<int, KeyTarget> &keys) : active(active), used(used),
                                                                                            written(written),
                                                                                            keys(keys) {

        }

    };


    class State {

    public:
        std::map<int, int> key_state;

        std::map<int, LayerState> layers;

        State(const map<int, int> keyState, const map<int, LayerState> &layers) : key_state(
                keyState),
                                                                                  layers(layers) {}


    };


    vector<input_event> process_for_layer(State &state, input_event event, std::ostream &logs);

    int parse_key(string keyCode);

    string serialize_key(int code);

    Config read_config(const string &file);

    State build_state(const Config &config);


};

