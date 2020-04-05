#include "gtest/gtest.h"
#include "schoenberg.h"
#include <unistd.h>
#include <fstream>

using namespace schoenberg;

typedef vector<pair<__u16, string>> TYPE_EVENTS;

vector<string> effective_keystrokes(TYPE_EVENTS event) {
    auto res = vector<string>();
    for (auto e:event) {
        if (e.first == 1) {
            res.push_back(e.second);
        }
    }
    return res;
}

State setup_test() {
    auto config = schoenberg::read_config("test.yaml");
    return schoenberg::build_state(config);
}


TYPE_EVENTS process_layer_events(State &state, TYPE_EVENTS events) {
    TYPE_EVENTS res;
    for (auto eventInput :events) {
        input_event event = {
                .type = EV_KEY,
                .code = (__u16) schoenberg::parse_key(eventInput.second),
                eventInput.first,
        };
        auto item_res = schoenberg::process_for_layer(state, event, cout);
        for (auto e:item_res) {
            res.push_back({e.value, schoenberg::serialize_key(e.code)});
        }
    }
    return res;
}

void print_strokes(vector<string> strokes) {
    for (const auto s:strokes) {
        cout << "=> " << s << endl;
    }
}

void print_events(TYPE_EVENTS events) {
    for (const auto s:events) {
        cout << "key: " << s.second << " value: " << s.first << endl;
    }
}

void events_equals(TYPE_EVENTS expected, TYPE_EVENTS actual) {
    EXPECT_EQ(expected.size(), actual.size());
    if (expected.size() == actual.size()) {
        for (int i = 0; i < expected.size(); i++) {
            auto expected_item = expected[i];
            auto actual_item = actual[i];
            EXPECT_EQ(expected_item.first, actual_item.first);
            EXPECT_EQ(expected_item.second, actual_item.second);
        }
    } else {
        cout << "expected: " << endl;
        print_events(expected);

        cout << "actual: " << endl;
        print_events(actual);

    }
}

pair<State, TYPE_EVENTS> test_run_layer(TYPE_EVENTS input, TYPE_EVENTS output, bool assert = true) {
    auto state = setup_test();
    auto res = process_layer_events(state, input);
    if (assert) {
        events_equals(output, res);
    }
    return {state, res};
};

void assert_strokes(vector<string> expected, vector<string> actual) {
    EXPECT_EQ(expected.size(), actual.size());
    if (expected.size() == actual.size()) {
        for (auto i = 0; i < expected.size(); i++) {
            auto expected_item = expected[i];
            auto actual_item = actual[i];
            EXPECT_EQ(expected_item, actual_item);
        }
    } else {
        for (auto i = 0; i < min(actual.size(), expected.size()); i++) {
            auto expected_item = expected[i];
            auto actual_item = actual[i];
            cout << expected_item << " <=> " << actual_item << endl;
        }
    }
}

void no_stroke_left_behind(TYPE_EVENTS events) {
    auto key_state = std::map<string, int>();
    for (auto e: events) {
        key_state[e.second] = e.first;
    }

    for (auto entry: key_state) {
        EXPECT_EQ(0, entry.second);
        if (0 != entry.second) {
            cout << "key: " << entry.first << " is left behind" << endl;
        }
    }
}
