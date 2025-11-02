#include <gtest/gtest.h>

#include "launcher/optionParser.hpp"

TEST(OptionParserTest, ShouldHandleNoOptions) {
    const int argc = 1;
    const char *argv[argc] = {"./exec"};
    AppOptions ops{argc, argv};

    EXPECT_EQ(ops.scr_width, ops.DEFAULT_SCR_WIDTH);
    EXPECT_EQ(ops.scr_height, ops.DEFAULT_SCR_HEIGHT);
    EXPECT_FALSE(ops.help);
    EXPECT_FLOAT_EQ(ops.tickrate, ops.DEFAULT_TICKRATE);
}

TEST(OptionParserTest, ShouldHandleHelp) {
    const int argc = 2;
    const char *argv[argc] = {"./exec", "--help"};
    AppOptions ops{argc, argv};

    EXPECT_TRUE(ops.help);
}

TEST(OptionParserTest, ShouldHandleWidth) {
    const int argc1 = 2;
    const char *argv1[argc1] = {"./exec", "--width=10"};
    AppOptions ops1{argc1, argv1};

    EXPECT_EQ(ops1.scr_width, 10);

    const int argc2 = 3;
    const char *argv2[argc2] = {"./exec", "-w", "10"};
    AppOptions ops2{argc2, argv2};

    EXPECT_EQ(ops2.scr_width, 10);

    const int argc3 = 3;
    const char *argv3[argc3] = {"./exec", "--width", "10"};
    AppOptions ops3{argc3, argv3};

    EXPECT_EQ(ops3.scr_width, 10);
}

TEST(OptionParserTest, ShouldHandleHeight) {
    const int argc1 = 2;
    const char *argv1[argc1] = {"./exec", "--height=10"};
    AppOptions ops1{argc1, argv1};

    EXPECT_EQ(ops1.scr_height, 10);

    const int argc2 = 3;
    const char *argv2[argc2] = {"./exec", "-h", "10"};
    AppOptions ops2{argc2, argv2};

    EXPECT_EQ(ops2.scr_height, 10);

    const int argc3 = 3;
    const char *argv3[argc3] = {"./exec", "--height", "10"};
    AppOptions ops3{argc3, argv3};

    EXPECT_EQ(ops3.scr_height, 10);
}

TEST(OptionParserTest, ShouldHandleTickrate) {
    const int argc1 = 2;
    const char *argv1[argc1] = {"./exec", "--tickrate=256.0"};
    AppOptions ops1{argc1, argv1};

    EXPECT_FLOAT_EQ(ops1.tickrate, 256.0);

    const int argc2 = 3;
    const char *argv2[argc2] = {"./exec", "-t", "256.0"};
    AppOptions ops2{argc2, argv2};

    EXPECT_FLOAT_EQ(ops2.tickrate, 256.0);

    const int argc3 = 3;
    const char *argv3[argc3] = {"./exec", "--tickrate", "256.0"};
    AppOptions ops3{argc3, argv3};

    EXPECT_FLOAT_EQ(ops3.tickrate, 256.0);

    const int argc4 = 2;
    const char *argv4[argc4] = {"./exec", "--tickrate=256"};
    AppOptions ops4{argc4, argv4};

    EXPECT_FLOAT_EQ(ops4.tickrate, 256.0);
}

TEST(OptionParserTest, ShouldHandleInputFile) {
    const int argc1 = 2;
    const char *argv1[argc1] = {"./exec", "--input-file=file.obj"};
    AppOptions ops1{argc1, argv1};

    EXPECT_STREQ(ops1.scene_file.c_str(), "file.obj");

    const int argc2 = 3;
    const char *argv2[argc2] = {"./exec", "-f", "file.obj"};
    AppOptions ops2{argc2, argv2};

    EXPECT_STREQ(ops2.scene_file.c_str(), "file.obj");

    const int argc3 = 3;
    const char *argv3[argc3] = {"./exec", "--input-file", "file.obj"};
    AppOptions ops3{argc3, argv3};

    EXPECT_STREQ(ops3.scene_file.c_str(), "file.obj");

    const int argc4 = 2;
    const char *argv4[argc4] = {"./exec", "file.obj"};
    AppOptions ops4{argc4, argv4};

    EXPECT_STREQ(ops4.scene_file.c_str(), "file.obj");
}
