#include <string>
#include <utility>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <linux/input.h>

using namespace std;


class SchoenbergLayer {

public:
    string name;
    string prefix;

    std::map<int, int> keysCodes;
    std::map<int, int> modCodes;

    SchoenbergLayer(const string &name, const string &prefix,
                    const map<int, int> &keysCodes, const map<int, int> &modCodes) : name(name), prefix(prefix),
                                                                                     keysCodes(keysCodes),
                                                                                     modCodes(modCodes) {}
};

class SchoenbergConfig {
public:
    std::vector<SchoenbergLayer> layers;

    SchoenbergConfig(std::vector<SchoenbergLayer> layers) : layers(std::move(layers)) {}

};

class SchoenbergLayerState {

public:
    bool active;
    std::optional<std::map<int, int>> start_layer_state;

    bool used;
    bool written;

    std::map<int, int> mapping;
    std::map<int, int> mod;

    SchoenbergLayerState() {};


    SchoenbergLayerState(bool active, const optional<std::map<int, int>> &startLayerState, bool used, bool written,
                         const map<int, int> &mapping, const map<int, int> &mod) : active(active),
                                                                                   start_layer_state(startLayerState),
                                                                                   used(used), written(written),
                                                                                   mapping(mapping), mod(mod) {}
};


class SchoenbergState {

public:
    std::map<int, int> key_state;

    std::map<int, SchoenbergLayerState> layers;

    SchoenbergState(const map<int, int> keyState, const map<int, SchoenbergLayerState> &layers) : key_state(keyState),
                                                                                                  layers(layers) {}


};

class schoenberg {
public:

    static int parse_key(string keyCode);

    static string serialize_key(int code);

    static SchoenbergConfig read_config(const string &file);

    static SchoenbergState build_state(const SchoenbergConfig &config);

    static vector<input_event> process_event(SchoenbergState &state, input_event event, std::ostream &logs);

};
