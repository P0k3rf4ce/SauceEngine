#include "modeling/ModelLoader.hpp"
#include "shared/Logger.hpp"
#include <filesystem>
#include <iostream>
// commented out to avoid compile errors
// [    (bool ModelLoader::processMesh(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)).
// (std::shared_ptr<Material> ModelLoader::processMaterial(aiMaterial* aiMat, const aiScene* scene))]

namespace modeling {


        /**
         * Main loading function
         * Loads model(s) from <filePath> using <shader>
         */
    std::vector<std::shared_ptr<Model>> ModelLoader::loadModels(
        const std::string& filePath, 
        std::shared_ptr<Shader> shader
    ) {
        LOG_INFO_F("Loading models from file: %s", filePath.c_str());
        
        // Create Assimp importer
        Assimp::Importer importer;
        
        // Configure import settings for optimal loading
        unsigned int importFlags = 
            aiProcess_Triangulate |           // Convert all faces to triangles
            aiProcess_FlipUVs |              // Flip texture coordinates (OpenGL convention)
            aiProcess_GenSmoothNormals |     // Generate smooth normals if missing
            aiProcess_CalcTangentSpace |     // Calculate tangent space for normal mapping
            aiProcess_JoinIdenticalVertices | // Remove duplicate vertices
            aiProcess_OptimizeMeshes |       // Optimize mesh data
            aiProcess_ValidateDataStructure | // Validate the imported data
            aiProcess_ImproveCacheLocality;  // Use Cache
        
        // Load the scene
        const aiScene* scene = importer.ReadFile(filePath, importFlags);
        
        // Validate the loaded scene
        if (!validateScene(scene)) {
            LOG_ERROR_F("Failed to load model from file: %s", filePath.c_str());
            LOG_ERROR_F("Assimp error: %s", importer.GetErrorString());
            return {};
        }
        
        LOG_INFO_F("Successfully loaded scene with %d meshes, %d materials", 
                scene->mNumMeshes, scene->mNumMaterials);
        
        // Process the scene and return models
        return processScene(scene, shader);
    }

    std::vector<std::shared_ptr<Model>> ModelLoader::processScene(
        const aiScene* scene, 
        std::shared_ptr<Shader> shader
    ) {
        LOG_DEBUG("Processing scene...");
        
        std::vector<std::shared_ptr<Model>> models;
        
        // Load all materials first
        std::vector<std::shared_ptr<Material>> materials = loadMaterials(scene);
        LOG_INFO_F("Loaded %d materials", static_cast<int>(materials.size()));
        
        // Load GLTF extensions
        std::unordered_map<std::string, std::string> gltfExtensions = loadGLTFExtensions(scene);
        LOG_INFO_F("Loaded %d GLTF extensions", static_cast<int>(gltfExtensions.size()));
        
        // Process the root node recursively
        if (scene->mRootNode) {
            processNode(scene->mRootNode, scene, models, materials, shader);
        }
        
        // Apply GLTF extensions to all models
        for (auto& model : models) {
            applyGLTFExtensions(model, gltfExtensions);
        }
        
        LOG_INFO_F("Successfully processed scene into %d models", static_cast<int>(models.size()));
        return models;
    }

    void ModelLoader::processNode(
        aiNode* node, 
        const aiScene* scene,
        std::vector<std::shared_ptr<Model>>& models,
        const std::vector<std::shared_ptr<Material>>& materials,
        std::shared_ptr<Shader> shader
    ) {
        LOG_DEBUG_F("Processing node: %s (meshes: %d, children: %d)", 
                    node->mName.C_Str(), node->mNumMeshes, node->mNumChildren);
        
        // Process GLTF node-specific data
        std::unordered_map<std::string, std::string> nodeExtensions;
        processGLTFNode(node, scene, nodeExtensions);
        
        // Process all meshes in this node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            unsigned int meshIndex = node->mMeshes[i];
            aiMesh* assimpMesh = scene->mMeshes[meshIndex];
            
            // Load mesh data 
            std::shared_ptr<Mesh> mesh = loadMeshFromNode(assimpMesh, scene);
            
            if (mesh) {
                // Get the material for this mesh
                std::shared_ptr<Material> material = nullptr;
                if (assimpMesh->mMaterialIndex < materials.size()) {
                    material = materials[assimpMesh->mMaterialIndex];
                }
                
                // Create a new model for this mesh
                std::vector<std::shared_ptr<Mesh>> meshes = { mesh };
                std::vector<std::shared_ptr<Material>> modelMaterials = { material };
                
                auto model = std::make_shared<Model>(meshes, modelMaterials, shader);
                
                // Apply node-specific GLTF extensions
                applyGLTFExtensions(model, nodeExtensions);
                
                models.push_back(model);
                
                LOG_DEBUG_F("Created model from mesh: %s", assimpMesh->mName.C_Str());
            } else {
                LOG_WARN_F("Failed to load mesh: %s", assimpMesh->mName.C_Str());
            }
        }
        
        // Recursively process child nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, models, materials, shader);
        }
    }

    // Utility functions
    std::string ModelLoader::getDirectoryPath(const std::string& filePath) {
        std::filesystem::path path(filePath);
        return path.parent_path().string();
    }

    bool ModelLoader::validateScene(const aiScene* scene) {
        if (!scene) {
            LOG_ERROR("Scene is null");
            return false;
        }
        
        if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            LOG_ERROR("Scene data is incomplete");
            return false;
        }
        
        if (!scene->mRootNode) {
            LOG_ERROR("Scene has no root node");
            return false;
        }
        
        if (scene->mNumMeshes == 0) {
            LOG_WARN("Scene contains no meshes");
            // This might be valid for some files, so don't return false
        }
        
        return true;
    }

    // TODO: Mesh loader 

    std::shared_ptr<Mesh> ModelLoader::loadMeshFromNode(aiMesh* mesh, const aiScene* scene) {
        // TODO: Implement mesh loading
        LOG_DEBUG_F("TODO: Implement loadMeshFromNode for mesh: %s", mesh->mName.C_Str());
        
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        // Call the mesh processing function
        if (!processMesh(mesh, vertices, indices)) {
            LOG_ERROR_F("Failed to process mesh: %s", mesh->mName.C_Str());
            return nullptr;
        }
        
        // Create and return the Mesh object
        try {
            return std::make_shared<Mesh>(vertices, indices);
        } catch (const std::exception& e) {
            LOG_ERROR_F("Failed to create Mesh object: %s", e.what());
            return nullptr;
        }
    }

    bool ModelLoader::processMesh(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
        // TODO: Implement mesh processing
        LOG_DEBUG("TODO: Implement processMesh");
        
        /*
        * 
        * 1. Extract vertices:
        *    - Loop through mesh->mVertices for positions
        *    - Loop through mesh->mNormals for normals (if available)
        *    - Loop through mesh->mTextureCoords[0] for UV coordinates (if available)
        *    - Create Vertex struct for each vertex
        * 
        * 2. Extract indices:
        *    - Loop through mesh->mFaces
        *    - Each face should be triangulated (3 indices per face)
        *    - Add indices to the indices vector
        * 
        * 3. Handle missing data:
        *    - If normals are missing, you might want to generate them
        *    - If UVs are missing, set to (0,0)
        * 
        * 4. Validate data:
        *    - Ensure indices are within bounds
        *    - Ensure we have at least some vertices
        * 
        */
        
        // Placeholder implementation - replace with actual implementation
        LOG_WARN("Using placeholder mesh processing - implement proper mesh loading!");
        
        // Create a simple triangle as placeholder
        vertices = {
            {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
        };
        indices = {0, 1, 2};
        
        return true;
    }

    namespace {
        Texture& default_texture() {
            static Texture* tex = []{
                auto p = std::unique_ptr<const uint8_t>(new uint8_t(255));
                return new Texture(std::move(p), 1, 1, 4, 0);
            }();
            return *tex;
        }

        std::shared_ptr<Material> make_default_material(const std::string& name_hint) {
            Texture& t = default_texture();
            return std::shared_ptr<Material>(new Material(
                name_hint.empty() ? std::string("material") : name_hint,
                t, t, t, t, t, t
            ));
        }

        Texture* loadTextureFromMaterial(aiMaterial* aiMat, const aiScene* scene, aiTextureType type, Texture& defaultTex) {
            if (!aiMat){ 
                return &defaultTex;
            }
            if (aiMat->GetTextureCount(type) == 0){
                return &defaultTex;
            }
            aiString path;
            if (aiMat->GetTexture(type, 0, &path) != AI_SUCCESS){
                return &defaultTex;
            }
            
            if (path.length > 0 && path.C_Str()[0] == '*') {
                int idx = std::atoi(path.C_Str() + 1);
                if (scene && idx >= 0 && static_cast<unsigned>(idx) < scene->mNumTextures) {
                    const aiTexture* emb = scene->mTextures[idx];
                    return &defaultTex;
                }
                return &defaultTex;
            }
            
            return &defaultTex;
        }
    }
    
    std::vector<std::shared_ptr<Material>> ModelLoader::loadMaterials(const aiScene* scene) {
        std::vector<std::shared_ptr<Material>> materials;
        if (!scene || scene->mNumMaterials == 0) {
            materials.push_back(make_default_material("default"));
            return materials;
        }

        materials.reserve(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            auto material = processMaterial(scene->mMaterials[i], scene);
            if (!material) {
                LOG_WARN_F("Failed to process material %d, using fallback", i);
                material = make_default_material("fallback");
            }
            materials.push_back(std::move(material));
        }
        return materials;
    }

    std::shared_ptr<Material> ModelLoader::processMaterial(aiMaterial* aiMat, const aiScene* scene) {
        if (!aiMat) {
            LOG_WARN("Null aiMaterial, using default material");
            return make_default_material("material");
        }

        aiString ainame; std::string name = "material";
        if (aiMat->Get(AI_MATKEY_NAME, ainame) == AI_SUCCESS && ainame.length > 0){ 
            name = ainame.C_Str();
        }

        auto& def_tex = default_texture();

        Texture* base   = loadTextureFromMaterial(aiMat, scene, aiTextureType_DIFFUSE, def_tex);
        Texture* normal = loadTextureFromMaterial(aiMat, scene, aiTextureType_NORMALS, def_tex);
        Texture* metal  = loadTextureFromMaterial(aiMat, scene, aiTextureType_METALNESS, def_tex);
        Texture* rough  = loadTextureFromMaterial(aiMat, scene, aiTextureType_DIFFUSE_ROUGHNESS, def_tex);
        Texture* ao     = loadTextureFromMaterial(aiMat, scene, aiTextureType_AMBIENT_OCCLUSION, def_tex);
        Texture* albedo = base;

        return std::shared_ptr<Material>(new Material(
            name, *base, *normal, *albedo, *metal, *rough, *ao
        ));

    }

    // todo: Tag Extraction
    std::unordered_map<std::string, std::string> ModelLoader::loadGLTFExtensions(const aiScene* scene) {

        LOG_DEBUG("TODO: Implement loadGLTFExtensions");
        
        /*
        * 
        * 1. Check if this is a GLTF file:
        *    - Look at scene metadata
        *    - Check for GLTF-specific properties
        * 
        * 2. Parse GLTF extensions from scene metadata:
        *    - Extensions are usually stored in scene->mMetaData
        *    - Look for keys that start with "gltf." or similar
        * 
        * 3. Handle common GLTF extensions:
        *    - KHR_materials_pbrSpecularGlossiness
        *    - KHR_materials_unlit
        *    - KHR_draco_mesh_compression
        *    - Custom extensions specific to your use case
        * 
        * 4. Parse JSON data if extensions contain complex data

    
        */
        
        std::unordered_map<std::string, std::string> extensions;
        
        // Placeholder implementation - replace with actual implementation
        LOG_WARN("Using placeholder GLTF extension loading - implement proper GLTF extension parsing!");
        
        return extensions;
    }

    void ModelLoader::processGLTFNode(
        aiNode* node, 
        const aiScene* scene, 
        std::unordered_map<std::string, std::string>& extensions
    ) {
        LOG_DEBUG_F("TODO: Implement processGLTFNode for node: %s", node->mName.C_Str());
        
        /*
        * 
        * 1. Check node metadata for GLTF-specific properties:
        *    - Custom properties
        *    - LOD information
        *    - Animation data
        * 
        * 2. Parse node transformation matrix:
        *    - node->mTransformation contains the local transform
        *    - Convert to your engine's matrix format if needed
        * 
        * 3. Look for GLTF node extensions:
        *    - Custom node properties
        *    - Lights (KHR_lights_punctual)
        *    - Physics properties
        * 
        * 4. Store found extensions in the extensions map
        * 

        */
        // Placeholder implementation - replace with actual implementation
        LOG_WARN("Using placeholder GLTF node processing - implement proper GLTF node parsing!");
    }

    void ModelLoader::applyGLTFExtensions(
        std::shared_ptr<Model> model, 
        const std::unordered_map<std::string, std::string>& extensions
    ) {

        LOG_DEBUG_F("TODO: Implement applyGLTFExtensions with %d extensions", static_cast<int>(extensions.size()));
        
        /*
        * 
        * 1. Iterate through all extensions
        * 2. For each extension, determine what it affects:
        *    - Material properties
        *    - Rendering settings
        *    - Animation data
        *    - Custom behaviors
        * 
        * 3. Apply extensions to the model:
        *    - Modify model properties
        *    - Set up additional rendering parameters
        *    - Configure animation systems
        * 
        * 4. Handle unknown extensions gracefully
        * 
        
        */
        
        // Placeholder implementation - replace with actual implementation
        if (!extensions.empty()) {
            LOG_WARN("Using placeholder GLTF extension application - implement proper extension handling!");
        }
    }

}