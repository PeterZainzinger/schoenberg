#include "test_utils.h"

TYPE_EVENTS empty_items;

TEST(Mapping, caps_to_esc) {
    auto input = TYPE_EVENTS({
                                     {1, "CAPSLOCK"},
                             });
    auto output = TYPE_EVENTS({
                                      {1, "ESC"},
                              });
    test_run(input, output);
}

TEST(Layer, basic_one_key) {
    TYPE_EVENTS input;
    input.emplace_back(1, "S");
    input.emplace_back(0, "S");

    TYPE_EVENTS output;
    output.emplace_back(1, "S");
    output.emplace_back(0, "S");

    test_run(input, output);
}

TEST(Layer, layer_still_goes_trough) {
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

TEST(Layer, layer_activation) {

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

TEST(Layer, layer_activation_not_if_key_down) {

    TYPE_EVENTS output;
    auto r = test_run(TYPE_EVENTS({
                                          {1, "I"},
                                          {1, "F"},
                                  }), empty_items, false);

    EXPECT_EQ(false, r.first.layers.at(schoenberg::parse_key("F")).active);
}

TEST(Layer, layer_deactivation) {
    TYPE_EVENTS input;
    input.emplace_back(1, "F");
    input.emplace_back(1, "H");
    input.emplace_back(0, "H");
    input.emplace_back(0, "F");

    auto res = test_run(input, empty_items, false);

    auto layer_state = res.first.layers[schoenberg::parse_key("F")];
    EXPECT_EQ(false, layer_state.active);

}

TEST(Layer, with_basic_prefix) {
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

TEST(Layer, with_modifier) {
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
TEST(Layer, asdf_test) {
    auto input = TYPE_EVENTS({{1, "A"},
                              {1, "S"},
                              {1, "D"},
                              {0, "A"},
                              {1, "F"},
                              {0, "S"},
                              {0, "D"},
                              {0, "F"}});
    auto expected = std::vector<string>({"A", "S", "D", "F"});
    auto state = setup_test();
    auto r = test_run(input, input, false);
    auto strokes = effective_keystrokes(r.second);
    assert_strokes(expected, strokes);
}


TEST(Layer, prefix_and_other) {
    auto input = TYPE_EVENTS({
                                     {1, "F"},
                                     {1, "G"},
                                     {0, "F"},
                                     {0, "G"},
                             });
    auto expected = std::vector<string>({"F", "G"});
    auto state = setup_test();
    auto r = test_run(input, input, false);
    print_events(r.second);
    auto strokes = effective_keystrokes(r.second);
    print_strokes(strokes);
    assert_strokes(expected, strokes);
}


TEST(Layer, repeater_bug) {
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

TEST(Layer, prefix_and_mapped_with_mod) {
    auto input = TYPE_EVENTS({
                                     {1, "F"},
                                     {1, "U"},
                                     {0, "F"},
                                     {0, "U"},
                             });
    auto state = setup_test();
    auto r = test_run(input, input, false);
    print_events(r.second);
    no_stroke_left_behind(r.second);
}


