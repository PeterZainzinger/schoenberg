#include "gtest/gtest.h"
#include "schoenberg.h"
#include <unistd.h>

TEST(Config, loadConfig) {
    auto config = schoenberg::read_config("test.yaml");
    EXPECT_EQ(2, config.layers.size());
    auto vimLayer = config.layers[0];

    EXPECT_EQ("F", vimLayer.prefix);

    auto state = schoenberg::build_state(config);
    EXPECT_EQ(2, state.layers.size());
}

TEST(Config, serialize) {
    auto target = "H";
    auto res = schoenberg::serialize_key(schoenberg::parse_key(target));
    EXPECT_EQ(target, res);
}

