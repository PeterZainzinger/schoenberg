#include "gtest/gtest.h"
#include "schoenberg.h"
#include <unistd.h>
#include <fstream>

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

SchoenbergState setup() {
    auto config = schoenberg::read_config("test.yaml");
    return schoenberg::build_state(config);
}


TYPE_EVENTS process_events(SchoenbergState &state, TYPE_EVENTS events) {

    TYPE_EVENTS res;

    for (auto eventInput :events) {
        input_event event = {
                .type = EV_KEY,
                .code = (__u16) schoenberg::parse_key(eventInput.second),
                eventInput.first,
        };
        auto item_res = schoenberg::process_event(state, event, cout);
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

pair<SchoenbergState, TYPE_EVENTS> test_run(TYPE_EVENTS input, TYPE_EVENTS output, bool assert = true) {
    auto state = setup();
    auto res = process_events(state, input);
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

TEST(Process, basic_one_key) {
    TYPE_EVENTS input;
    input.emplace_back(1, "S");
    input.emplace_back(0, "S");

    TYPE_EVENTS output;
    output.emplace_back(1, "S");
    output.emplace_back(0, "S");

    test_run(input, output);
}


TYPE_EVENTS empty_items;

TEST(Process, layer_still_goes_trough) {
    auto input = TYPE_EVENTS({
                                     {1, "F"},
                                     {0, "F"}
                             });
    auto output = TYPE_EVENTS({
                                      {1, "F"},
                                      {0, "F"}
                              });

    test_run(input, output);
}

TEST(Process, layer_activation) {

    TYPE_EVENTS output;
    auto r = test_run(TYPE_EVENTS({
                                          {1, "F"},
                                          {2, "F"}
                                  }), empty_items);

    EXPECT_EQ(true, r.first.layers.at(schoenberg::parse_key("F")).active);
    auto strokes = effective_keystrokes(r.second);
    EXPECT_EQ(0, strokes.size());

    r = test_run(TYPE_EVENTS({{0, "F"}}), empty_items, false);
    strokes = effective_keystrokes(r.second);
    EXPECT_EQ(0, strokes.size());
    EXPECT_EQ(false, r.first.layers.at(schoenberg::parse_key("F")).active);
}

TEST(Process, layer_activation_not_if_key_down) {

    TYPE_EVENTS output;
    auto r = test_run(TYPE_EVENTS({
                                          {1, "I"},
                                          {1, "F"},
                                  }), empty_items, false);

    EXPECT_EQ(false, r.first.layers.at(schoenberg::parse_key("F")).active);
}

TEST(Process, layer_deactivation) {
    TYPE_EVENTS input;
    input.emplace_back(1, "F");
    input.emplace_back(1, "H");
    input.emplace_back(0, "H");
    input.emplace_back(0, "F");

    auto res = test_run(input, empty_items, false);

    auto layer_state = res.first.layers[schoenberg::parse_key("F")];
    EXPECT_EQ(false, layer_state.active);

}

TEST(Process, with_basic_prefix) {
    TYPE_EVENTS input;
    input.emplace_back(1, "F");
    input.emplace_back(1, "H");
    input.emplace_back(0, "H");
    input.emplace_back(0, "F");

    TYPE_EVENTS output;
    output.emplace_back(1, "LEFT");
    output.emplace_back(0, "LEFT");

    auto res = test_run(input, output);
    cout << res.first.layers[schoenberg::parse_key("F")].active << endl;
}

TEST(Process, with_modifier) {
    TYPE_EVENTS input;
    input.emplace_back(1, "F");
    input.emplace_back(1, "U");
    input.emplace_back(0, "U");
    input.emplace_back(0, "F");

    TYPE_EVENTS output;
    output.emplace_back(1, "LEFTSHIFT");
    output.emplace_back(1, "LEFTBRACE");
    output.emplace_back(0, "LEFTBRACE");
    output.emplace_back(0, "LEFTSHIFT");

    test_run(input, output);
}


// typing "asdf" fast should result in "asdf" even tough
// f is a layer key
TEST(Process, asdf_test) {
    auto input = TYPE_EVENTS({{1, "A"},
                              {1, "S"},
                              {1, "D"},
                              {0, "A"},
                              {1, "F"},
                              {0, "S"},
                              {0, "D"},
                              {0, "F"}});
    auto expected = std::vector<string>({"A", "S", "D", "F"});
    auto state = setup();
    auto r = test_run(input, input, false);
    auto strokes = effective_keystrokes(r.second);
    assert_strokes(expected, strokes);
}


TEST(Process, prefix_and_other) {
    auto input = TYPE_EVENTS({
                                     {1, "F"},
                                     {1, "G"},
                                     {0, "F"},
                                     {0, "G"},
                             });
    auto expected = std::vector<string>({"F", "G"});
    auto state = setup();
    auto r = test_run(input, input, false);
    print_events(r.second);
    auto strokes = effective_keystrokes(r.second);
    print_strokes(strokes);
    assert_strokes(expected, strokes);
}


TEST(Process, repeater_bug) {
    auto input = TYPE_EVENTS({
                                     {1, "N"},
                                     {1, "A"},
                                     {1, "S"},
                                     {1, "D"},
                                     {0, "A"},
                                     {0, "S"},
                                     {0, "D"},
                                     {0, "N"},
                             });
    auto r = test_run(input, input, false);
    print_events(r.second);

}

TEST(Process, prefix_and_mapped_with_mod) {
    auto input = TYPE_EVENTS({
                                     {1, "F"},
                                     {1, "U"},
                                     {0, "F"},
                                     {0, "U"},
                             });
    auto state = setup();
    auto r = test_run(input, input, false);
    print_events(r.second);
    no_stroke_left_behind(r.second);
}
