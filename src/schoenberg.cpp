#include "schoenberg.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include "map"
#include <libevdev/libevdev.h>

using namespace schoenberg;

vector<int> down_keys(State &state) {
    vector<int> res;
    for (auto e: state.key_state) {
        if (e.second > 0) {
            res.push_back(e.first);
        }
    }
    return res;
}

bool has_down_key(State &state) {
    return !down_keys(state).empty();
}

int schoenberg::parse_key(string keyCode) {
    std::stringstream key;
    key << "KEY_" << keyCode;
    auto res = libevdev_event_code_from_name(EV_KEY, key.str().c_str());
    return res;
}

string schoenberg::serialize_key(int code) {
    auto res = libevdev_event_code_get_name(EV_KEY, code);
    if (res != NULL) {
        auto resStr = string(res);
        return resStr.substr(4);
    }
    return NULL;
}

KeyTarget parse_key_target(YAML::Node node) {
    if (node.IsScalar()) {
        auto keyInfo = node.as<string>();
        return KeyTarget(parse_key(keyInfo), -1);
    } else {
        auto keyInfo = node["key"].as<string>();

        auto modKey = -1;
        if (node["mod"]) {
            auto modInfo = node["mod"].as<string>();
            modKey = parse_key(modInfo);
        }
        return KeyTarget(parse_key(keyInfo), modKey);
    }
}

Config schoenberg::read_config(const string &file) {
    YAML::Node config = YAML::LoadFile(file);
    YAML::Node layers = config["layers"];
    YAML::Node mapping = config["mapping"];
    vector<LayerConfig> outputLayers;

    map<int, KeyTarget> keys;

    for (auto layer:layers) {
        YAML::Node keys = layer["keys"];
        map<int, KeyTarget> keys_output;

        for (YAML::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            keys_output[parse_key(it->first.as<string>())] = parse_key_target(it->second);
        }
        outputLayers.emplace_back(
                layer["prefix"].as<std::string>(),
                keys_output
        );
    }
    for (YAML::const_iterator it = mapping.begin(); it != mapping.end(); ++it) {
        keys[parse_key(it->first.as<string>())] = parse_key_target(it->second);
    }
    return Config(outputLayers, keys);
}

State schoenberg::build_state(const Config &config) {
    std::map<int, LayerState> output_layers;
    for (const auto &layer: config.layers) {
        auto key = parse_key(layer.prefix);
        auto state = LayerState(false, false, false, layer.keys);
        output_layers.insert({key, state});
    }
    return State(map<int, int>(), output_layers);
}

std::optional<pair<int, LayerState>> find_active_layer(State &state) {
    std::optional<pair<int, LayerState>> activeLayer;
    for (auto entry: state.layers) {
        if (entry.second.active) {
            activeLayer = entry;
            break;
        }
    }
    return activeLayer;
}


input_event create_event(__u16 code, int value) {
    return input_event{.type = EV_KEY, .code = code, .value = value};
}

vector<input_event> create_release_events(vector<int> keys) {
    vector<input_event> res;
    for (auto k: keys) {
        res.push_back(create_event(k, 0));
    }
    return res;

}

void add_event(vector<input_event> &events, input_event event, string message, std::ostream &logs) {
    logs << "add event " << event.code << " value: " << event.value << " message: " << message << endl;
    events.push_back(event);
}

void activate_layer(State &state, int code, std::ostream &logs) {
    logs << "activate_layer " << code << endl;
    state.layers[code].active = true;
    state.layers[code].used = false;
    state.layers[code].written = false;
}

void deactivate_layer(State &state, int code, std::ostream &logs) {
    logs << "deactivate_layer " << code << endl;
    state.layers[code].active = false;
}

void update_key_state(State &state, vector<input_event> events) {
    for (auto e: events) {
        state.key_state[e.code] = e.value;
    }
}

vector<input_event> schoenberg::process_for_layer(State &state, input_event event, std::ostream &logs) {
    vector<input_event> res;

    auto activeLayer = find_active_layer(state);

    // handle special cases
    if (activeLayer.has_value() && event.value == 0 && activeLayer.value().first == event.code) {
        deactivate_layer(state, event.code, logs);
        if (!activeLayer.value().second.used && !activeLayer.value().second.written) {
            add_event(res, create_event(event.code, 1), "not used prefix key down", logs);
            add_event(res, create_event(event.code, 0), "not used prefix key up", logs);
        }

        // if there are still keys that are not release, release them now
        auto downs = down_keys(state);
        auto down_release = create_release_events(downs);
        for (auto release: down_release) {
            add_event(res, release, "release down key", logs);
        }

        update_key_state(state, res);
        return res;
    } else if (!has_down_key(state) && !activeLayer.has_value() && event.value == 1 && state.layers.count(event.code)) {
        activate_layer(state, event.code, logs);
        update_key_state(state, res);
        return res;
    } else if (event.value == 2 && state.layers.count(event.code)) {
        state.layers[event.code].used = true;
        // ignore holding a layer key
        update_key_state(state, res);
        return res;
    }

    activeLayer = find_active_layer(state);

    if (activeLayer.has_value()) {
        // check if key is mapped
        if (activeLayer.value().second.keys.count(event.code)) {
            auto target = activeLayer.value().second.keys[event.code];
            if (target.mod > 0 && event.value == 1) {
                add_event(res, create_event(target.mod, 1), "add mod before", logs);
            }
            add_event(res, create_event(target.key, event.value), "mapped key", logs);
            if (target.mod > 0 && event.value == 0) {
                add_event(res, create_event(target.mod, 0), "add mod after", logs);
            }
        } else {
            // if an layer is active but key is not mapped still write it throw
            if (!state.layers[activeLayer.value().first].used && event.value == 1) {
                add_event(res, create_event(activeLayer.value().first, 1), "written now down", logs);
                add_event(res, create_event(activeLayer.value().first, 0), "written now up", logs);
                state.layers[activeLayer.value().first].written = true;
            }
            add_event(res, create_event(event.code, event.value), "in layer, but not mapped", logs);
        }
        // if there is an active value with an down or hold event mark it as used
        if (event.value == 1) {
            state.layers[activeLayer.value().first].used = true;
        }
    } else {
        add_event(res, event, "default case", logs);
    }
    update_key_state(state, res);
    return res;
}

vector<input_event> schoenberg::process_mapping(Config &config, input_event event, std::ostream &logs) {
    auto res = vector<input_event>();
    if (config.keys.count(event.code)) {
        auto target = config.keys[event.code];
        if (target.mod > 0 && event.value == 1) {
            add_event(res, create_event(target.mod, 1), "mapping: add mod before", logs);
        }
        add_event(res, create_event(target.key, event.value), "mapping: mapped key", logs);
        if (target.mod > 0 && event.value == 0) {
            add_event(res, create_event(target.mod, 0), "mapping: add mod after", logs);
        }
    } else {
        res.push_back(event);
    }
    return res;
}



