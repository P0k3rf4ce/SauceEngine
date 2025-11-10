#ifndef MODEL_LOADER_HPP
#define MODEL_LOADER_HPP

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "modeling/Model.hpp"
#include "modeling/Mesh.hpp"
#include "modeling/Material.hpp"
#include "modeling/ModelProperties.hpp"
#include "utils/Shader.hpp"

namespace modeling {


class ModelLoader {
public:
    /**
     * @brief Load models from a 3D file (supports all Assimp formats)
     * @param filePath Path to the 3D model file
     * @param shader Shared pointer to the shader to use for all models
     * @return Vector of loaded models
     */
    static std::vector<std::shared_ptr<Model>> loadModels(
        const std::string& filePath, 
        std::shared_ptr<Shader> shader
    );

private:
    
    /**
     * @brief Process the entire scene and extract all models
     * @param scene Assimp scene object
     * @param shader Shader to assign to models
     * @return Vector of loaded models
     */
    static std::vector<std::shared_ptr<Model>> processScene(
        const aiScene* scene, 
        std::shared_ptr<Shader> shader
    );
    
    /**
     * @brief Recursively process scene nodes to build models
     * @param node Current scene node
     * @param scene Assimp scene object
     * @param models Output vector to add models to
     * @param materials Vector of loaded materials
     * @param shader Shader to assign to models
     */
    static void processNode(
        aiNode* node, 
        const aiScene* scene,
        std::vector<std::shared_ptr<Model>>& models,
        const std::vector<std::shared_ptr<Material>>& materials,
        std::shared_ptr<Shader> shader
    );

  
    /**
     * @brief Load mesh data from an Assimp mesh object
     * @param mesh Assimp mesh object
     * @param scene Assimp scene object (for additional context)
     * @return Shared pointer to the loaded Mesh object
     * 
     * IMPLEMENTATION NOTES:
     * - Extract vertices (position, normal, texture coordinates)
     * - Extract indices for triangulated faces
     * - Handle multiple texture coordinate sets if needed
     * - Validate mesh data before creating Mesh object
     */
    static std::shared_ptr<Mesh> loadMeshFromNode(aiMesh* mesh, const aiScene* scene, std::shared_ptr<Shader> shader);
    
    /**
     * @brief Process and validate mesh data
     * @param mesh Assimp mesh object
     * @param vertices Output vector for vertex data
     * @param indices Output vector for index data
     * @return True if processing successful, false otherwise
     * 
     * IMPLEMENTATION NOTES:
     * - Convert Assimp vertex format to engine Vertex struct
     * - Ensure proper winding order for faces
     * - Handle cases where normals or texture coordinates are missing
     */
    static bool processMesh(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

    // TODO: Implement these functions for material loading
    
    /**
     * @brief Load all materials from the scene
     * @param scene Assimp scene object
     * @return Vector of loaded materials
     * 
     * IMPLEMENTATION NOTES:
     * - Process all materials in scene->mMaterials
     * - Extract textures for each material type (diffuse, normal, metallic, etc.)
     * - Handle missing textures with default values
     * - Create Material objects using the existing Material::from_aiMaterial method or similar
     */
    static std::vector<std::shared_ptr<Material>> loadMaterials(const aiScene* scene);
    
    /**
     * @brief Process a single material from Assimp data
     * @param aiMat Assimp material object
     * @param scene Assimp scene object (for texture loading)
     * @return Shared pointer to the loaded Material object
     * 
     * IMPLEMENTATION NOTES:
     * - Extract material properties (colors, factors, etc.)
     * - Load and process embedded or referenced textures
     * - Handle different material models (PBR, Phong, etc.)
     */
    static std::shared_ptr<Material> processMaterial(aiMaterial* aiMat, const aiScene* scene);

    // TODO: Implement these functions for GLTF extension loading
    
    /**
     * @brief Load custom GLTF extensions and extras from the scene
     * @param scene Assimp scene object
     * @return Map of extension name to extension data
     * 
     * IMPLEMENTATION NOTES:
     * - Parse GLTF extras and extensions from scene metadata
     * - Handle custom properties that aren't part of standard GLTF
     * - Store extension data in a format that can be used by the engine
     * - Consider using JSON parsing for complex extension data
     */
    static std::unordered_map<std::string, PropertyValue> loadGLTFExtensions(const aiScene* scene);
    
    /**
     * @brief Process GLTF-specific node properties and extensions
     * @param node Assimp node object
     * @param scene Assimp scene object
     * @param extensions Map to store found extensions
     * 
     * IMPLEMENTATION NOTES:
     * - Extract node-level GLTF extensions
     * - Handle custom animation data
     * - Process LOD (Level of Detail) information
     * - Extract custom material variants or properties
     */
    static void processGLTFNode(
        aiNode* node, 
        const aiScene* scene, 
        std::unordered_map<std::string, PropertyValue>& extensions
    );
    
    /**
     * @brief Apply GLTF extensions to a model after loading
     * @param model Model to apply extensions to
     * @param extensions Map of extension data
     * 
     * IMPLEMENTATION NOTES:
     * - Apply custom properties to model
     * - Set up additional rendering parameters
     * - Configure animation or interaction data
     */
    static void applyGLTFExtensions(
        std::shared_ptr<Model> model, 
        const std::unordered_map<std::string, PropertyValue>& extensions
    );

    // Utils
    
    /**
     * @brief Get the directory path from a file path
     * @param filePath Full file path
     * @return Directory containing the file
     */
    static std::string getDirectoryPath(const std::string& filePath);
    
    /**
     * @brief Validate that a scene was loaded successfully
     * @param scene Assimp scene object
     * @return True if scene is valid, false otherwise
     */
    static bool validateScene(const aiScene* scene);
};

} // namespace modeling

#endif // MODEL_LOADER_HPP