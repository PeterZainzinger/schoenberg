#include "schoenberg.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include "map"
#include <libevdev/libevdev.h>

vector<int> down_keys(SchoenbergState &state) {
    vector<int> res;
    for (auto e: state.key_state) {
        if (e.second > 0) {
            res.push_back(e.first);
        }
    }
    return res;
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

SchoenbergConfig schoenberg::read_config(const string &file) {
    YAML::Node config = YAML::LoadFile(file);
    YAML::Node layers = config["layers"];
    vector<SchoenbergLayer> outputLayers;

    for (auto layer:layers) {
        YAML::Node keys = layer["keys"];
        map<int, int> keysCodeOutput;
        map<int, int> mods;

        for (YAML::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            auto key = it->first.as<string>();

            if (it->second.IsScalar()) {
                auto keyInfo = it->second.as<string>();
                keysCodeOutput[parse_key(key)] = parse_key(keyInfo);
            } else {
                auto keyInfo = it->second["key"].as<string>();
                auto modInfo = it->second["mod"].as<string>();
                keysCodeOutput[parse_key(key)] = parse_key(keyInfo);
                mods[parse_key(key)] = parse_key(modInfo);
            }
        }
        outputLayers.emplace_back(
                layer["name"].as<std::string>(),
                layer["prefix"].as<std::string>(),
                keysCodeOutput, mods
        );
    }
    return SchoenbergConfig(outputLayers);
}

SchoenbergState schoenberg::build_state(const SchoenbergConfig &config) {
    std::map<int, SchoenbergLayerState> output_layers;
    for (const auto &layer: config.layers) {
        auto key = parse_key(layer.prefix);
        auto state = SchoenbergLayerState(false, optional<map<int, int>>(), false, false, layer.keysCodes,
                                          layer.modCodes);
        output_layers.insert({key, state});
    }
    return SchoenbergState(map<int, int>(), output_layers);
}

std::optional<pair<int, SchoenbergLayerState>> find_active_layer(SchoenbergState &state) {
    std::optional<pair<int, SchoenbergLayerState>> activeLayer;
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

void activate_layer(SchoenbergState &state, int code, std::ostream &logs) {
    logs << "activate_layer " << code << endl;
    state.layers[code].active = true;
    state.layers[code].used = false;
    state.layers[code].written = false;
}

void deactivate_layer(SchoenbergState &state, int code, std::ostream &logs) {
    logs << "deactivate_layer " << code << endl;
    state.layers[code].active = false;
}

void update_key_state(SchoenbergState &state, vector<input_event> events) {
    for (auto e: events) {
        state.key_state[e.code] = e.value;
    }
}

vector<input_event> schoenberg::process_event(SchoenbergState &state, input_event event, std::ostream &logs) {
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
    } else if (!activeLayer.has_value() && event.value == 1 && state.layers.count(event.code)) {
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
        if (activeLayer.value().second.mapping.count(event.code)) {
            auto target = activeLayer.value().second.mapping[event.code];
            auto has_mod = activeLayer.value().second.mod.count(event.code);
            if (has_mod && event.value == 1) {
                add_event(res, create_event(activeLayer.value().second.mod[event.code], 1), "add mod before", logs);
            }
            add_event(res, create_event(target, event.value), "mapped key", logs);
            if (has_mod && event.value == 0) {
                add_event(res, create_event(activeLayer.value().second.mod[event.code], 0), "add mod after", logs);
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


