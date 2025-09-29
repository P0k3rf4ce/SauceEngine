#ifndef MODEL_LOADER_HPP
#define MODEL_LOADER_HPP

// #include "modeling/Mesh.hpp"
// #include "modeling/ModelProperties.hpp"

#include <string>
#include <vector>

namespace modeling {
class ModelLoader {
public:
    // Loads a model from the given file path
    static void load(const std::string& path);
    static const std::string& getDirectory(); //direcrtory getter method

    static std::size_t debug_mesh_count(); // for testing purposes, returns number of loaded meshes

private:

    static std::string directory;
};
}
#endif