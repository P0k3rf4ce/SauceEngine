#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "modeling/Manager.hpp"
#include "modeling/ModelLoader.hpp"


class AssetManagerTest : public ::testing::Test {
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_F(AssetManagerTest, Load) {
    AssetManager manager;
    EXPECT_EQ(manager.get_scenes().size(), 0); // no file loaded
    
    // load file
    manager.load_file("test/assets/unitcube.gltf");
    EXPECT_EQ(manager.get_scenes().size(), 1); // file loaded
    EXPECT_FALSE(manager.get_scenes()[0].is_marked_unloaded); // marked loaded
}

TEST_F(AssetManagerTest, Unload) {
    AssetManager manager;

    // load file
    manager.load_file("test/assets/unitcube.gltf");
    manager.mark_unloadable("test/assets/unitcube.gltf");
    auto &scene = manager.get_scenes()[0];

    // file still exist but not loaded in memory
    EXPECT_EQ(manager.get_scenes().size(), 1); 
    EXPECT_TRUE(scene.is_marked_unloaded);
    for (auto &model: scene.contents) {
        EXPECT_TRUE(holds_alternative<MaybeUnloadedModel>(model));
        EXPECT_TRUE(get<MaybeUnloadedModel>(model).expired());
    }
}


TEST_F(AssetManagerTest, Reload) {
    AssetManager manager;

    // reload file
    manager.load_file("test/assets/unitcube.gltf");
    manager.mark_unloadable("test/assets/unitcube.gltf");
    manager.load_file("test/assets/unitcube.gltf");
    auto &scene = manager.get_scenes()[0];

    // file is reloaded into memory  
    EXPECT_EQ(manager.get_scenes().size(), 1); 
    EXPECT_FALSE(scene.is_marked_unloaded);
    for (auto &model: scene.contents) {
        EXPECT_TRUE(holds_alternative<LoadedModel>(model));
    }
}