#include "modeling/ModelLoader.hpp"
#include "utils/Logger.hpp"
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
        return processScene(scene, shader, filePath);
    }

    std::vector<std::shared_ptr<Model>> ModelLoader::processScene(
        const aiScene* scene,
        std::shared_ptr<Shader> shader,
        const std::string& filePath
    ) {
        LOG_DEBUG("Processing scene...");

        std::vector<std::shared_ptr<Model>> models;

        // Get the directory containing the model file for texture loading
        std::string modelDir = getDirectoryPath(filePath);
        LOG_DEBUG_F("Model directory: %s", modelDir.c_str());

        // Create texture cache for this scene
        TextureCache textureCache;

        // Load all materials first
        std::vector<std::shared_ptr<Material>> materials = loadMaterials(scene, textureCache, modelDir);
        LOG_INFO_F("Loaded %d materials", static_cast<int>(materials.size()));

        // Load GLTF extensions
        std::unordered_map<std::string, PropertyValue> gltfExtensions = loadGLTFExtensions(scene);
        LOG_INFO_F("Loaded %d GLTF extensions", static_cast<int>(gltfExtensions.size()));

        // Map to store node metadata for each model
        std::unordered_map<std::shared_ptr<Model>, std::unordered_map<std::string, PropertyValue>> nodeData;

        // Process the root node recursively
        if (scene->mRootNode) {
            processNode(scene->mRootNode, scene, models, materials, shader, nodeData);
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
        std::shared_ptr<Shader> shader,
        std::unordered_map<std::shared_ptr<Model>, std::unordered_map<std::string, PropertyValue>>& nodeData
    ) {
        LOG_DEBUG_F("Processing node: %s (meshes: %d, children: %d)",
                    node->mName.C_Str(), node->mNumMeshes, node->mNumChildren);

        // Store node metadata
        std::unordered_map<std::string, PropertyValue> metadata;

        // Store node name
        metadata["node_name"] = std::string(node->mName.C_Str());

        // Store node transformation matrix components
        aiVector3D position, scaling;
        aiQuaternion rotation;
        node->mTransformation.Decompose(scaling, rotation, position);

        metadata["pos_x"] = static_cast<double>(position.x);
        metadata["pos_y"] = static_cast<double>(position.y);
        metadata["pos_z"] = static_cast<double>(position.z);

        metadata["rot_x"] = static_cast<double>(rotation.x);
        metadata["rot_y"] = static_cast<double>(rotation.y);
        metadata["rot_z"] = static_cast<double>(rotation.z);
        metadata["rot_w"] = static_cast<double>(rotation.w);

        metadata["scale_x"] = static_cast<double>(scaling.x);
        metadata["scale_y"] = static_cast<double>(scaling.y);
        metadata["scale_z"] = static_cast<double>(scaling.z);

        // Process GLTF node-specific extensions
        std::unordered_map<std::string, PropertyValue> nodeExtensions;
        processGLTFNode(node, scene, nodeExtensions);

        // Merge extensions into metadata
        for (const auto& [key, value] : nodeExtensions) {
            metadata[key] = value;
        }

        // Process all meshes in this node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            unsigned int meshIndex = node->mMeshes[i];
            aiMesh* assimpMesh = scene->mMeshes[meshIndex];

            // Load mesh data
            std::shared_ptr<Mesh> mesh = loadMeshFromNode(assimpMesh, scene, shader);

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

                // Set metadata on the model
                model->setMetadata(metadata);

                // Store metadata for this model (for Scene integration later)
                nodeData[model] = metadata;

                // Apply node-specific GLTF extensions
                applyGLTFExtensions(model, nodeExtensions);

                models.push_back(model);

                LOG_DEBUG_F("Created model from node '%s', mesh '%s'",
                           node->mName.C_Str(), assimpMesh->mName.C_Str());
            } else {
                LOG_WARN_F("Failed to load mesh: %s", assimpMesh->mName.C_Str());
            }
        }

        // Recursively process child nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, models, materials, shader, nodeData);
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

    std::shared_ptr<Mesh> ModelLoader::loadMeshFromNode(aiMesh* mesh, const aiScene* scene, std::shared_ptr<Shader> shader) {
        LOG_DEBUG_F("Loading mesh: %s", mesh->mName.C_Str());

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Call the mesh processing function
        if (!processMesh(mesh, vertices, indices)) {
            LOG_ERROR_F("Failed to process mesh: %s", mesh->mName.C_Str());
            return nullptr;
        }

        // Create and return the Mesh object
        try {
            bool setupGL = (shader != nullptr);
            return std::make_shared<Mesh>(vertices, indices, setupGL);
        } catch (const std::exception& e) {
            LOG_ERROR_F("Failed to create Mesh object: %s", e.what());
            return nullptr;
        }
    }

    bool ModelLoader::processMesh(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
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
    Vertex v; /* vertices */
    unsigned int idx; /* indices */
    unsigned int nvertices = mesh->mNumVertices;
    unsigned int nfaces = mesh->mNumFaces;

		if (nvertices < 1) {
			LOG_ERROR("Mesh has no vertices, fail");
			return false;
		}
		if (!(mesh->HasFaces())) {
			/*
			 * apparently some assimp-valid models can have no
			 * faces via "special scene flags" but I'm
			 * assuming this is out of scope
			 */
			LOG_ERROR("Mesh has no indices, fail");
			return false;
		}

		/* fetch vertex/normal/UV data */
		vertices.reserve(nvertices); // allocate space early, quick optimization
		for (unsigned int i=0; i<nvertices; i++) {
			v.Position.x=mesh->mVertices[i].x;
			v.Position.y=mesh->mVertices[i].y;
			v.Position.z=mesh->mVertices[i].z;

			/* fetch normals, or generate */
			if (mesh->HasNormals()) {
				/*
				 * if there are normals, "The array is mNumVertices in size",
				 * so this is safe without checking the size of mNormals.
				 * Similar for mTextureCoords
				 * ref: https://documentation.help/assimp/structai_mesh.html
				 */
				v.Normal.x=mesh->mNormals[i].x;
				v.Normal.y=mesh->mNormals[i].y;
				v.Normal.z=mesh->mNormals[i].z;
			}
			else {
				/*
				 * we use the assimp flag aiProcess_GenSmoothNormals,
				 * so assimp should always generate normals for us.
				 * Therefore we should consider it an error if
				 * a mesh gets to this point with no normals.
				 */
				LOG_ERROR("Mesh loader: mesh has no normal vectors, did assimp fail to generate them?");
				return false;
			}

			/* fetch UV coords, or default to (0,0) */
			if (mesh->HasTextureCoords(0)) {
				v.TexCoords.x=mesh->mTextureCoords[0][i].x;
				v.TexCoords.y=mesh->mTextureCoords[0][i].y;
			}
			else {
				v.TexCoords.x=0;
				v.TexCoords.y=0;
			}

			vertices.push_back(v);
			LOG_DEBUG_F("vertex {}: <{},{},{}>, UV=({}, {}), normal <{},{},{}>",i,
					v.Position.x,v.Position.y,v.Position.z,
					v.TexCoords.x,v.TexCoords.y,
					v.Normal.x,v.Normal.y,v.Normal.z
			);
		}

        /* fetch faces/indices */
        indices.reserve(nfaces * 3);
		for (unsigned int i=0; i<nfaces; i++) {
			auto f=mesh->mFaces[i];

			/* skip points/lines, only load triangles */
			if (f.mNumIndices!=3) {
				LOG_INFO_F("Mesh loader: faces[{}] is not a triangle, skipping",i);
				continue;
			}

            /* flatten indices of all faces to 1d vector of indices */
            for (unsigned int j=0; j<f.mNumIndices; j++) {
                idx = f.mIndices[j];
				/* check for indices out of bounds - we cannot use vertex 4 of a triangle */
                if (idx>=nvertices) {
                    LOG_ERROR_F("Mesh loader: indices[{}] of faces[{}]={}, but there are only {} vertices", j, i, idx, nvertices);
					return false;
				}
                indices.push_back(idx);
			}
		}

        /* if for some reason the model has faces but they are all points/lines, fail to load */
        if (indices.empty()) {
            LOG_ERROR("Mesh loader: mesh did not contain any triangles");
            return false;
        }

        LOG_DEBUG_F("Mesh loaded: %u vertices, %zu indices", nvertices, indices.size());

        return true;
    }

    namespace {
        std::shared_ptr<Material> make_default_material(const std::string& name_hint, TextureCache& cache) {
            auto defaultTex = cache.getDefaultTexture();
            return std::make_shared<Material>(
                name_hint.empty() ? std::string("material") : name_hint,
                defaultTex, defaultTex, defaultTex, defaultTex, defaultTex, defaultTex
            );
        }

        std::shared_ptr<Texture> loadTextureFromMaterial(
            aiMaterial* aiMat,
            const aiScene* scene,
            aiTextureType type,
            TextureCache& cache,
            const std::string& modelDir
        ) {
            if (!aiMat || aiMat->GetTextureCount(type) == 0) {
                return cache.getDefaultTexture();
            }

            aiString path;
            if (aiMat->GetTexture(type, 0, &path) != AI_SUCCESS) {
                return cache.getDefaultTexture();
            }

            // Check if it's an embedded texture
            if (path.length > 0 && path.C_Str()[0] == '*') {
                int idx = std::atoi(path.C_Str() + 1);
                if (scene && idx >= 0 && static_cast<unsigned>(idx) < scene->mNumTextures) {
                    const aiTexture* embeddedTex = scene->mTextures[idx];
                    return cache.getEmbeddedTexture(embeddedTex);
                }
                LOG_WARN_F("Invalid embedded texture index: %d", idx);
                return cache.getDefaultTexture();
            }

            // External texture file - resolve path relative to model directory
            std::filesystem::path texturePath = std::filesystem::path(modelDir) / path.C_Str();
            std::string texturePathStr = texturePath.string();

            if (!std::filesystem::exists(texturePath)) {
                LOG_WARN_F("Texture file not found: %s", texturePathStr.c_str());
                return cache.getDefaultTexture();
            }

            return cache.getTexture(texturePathStr);
        }
    }
    
    std::vector<std::shared_ptr<Material>> ModelLoader::loadMaterials(
        const aiScene* scene,
        TextureCache& cache,
        const std::string& modelDir
    ) {
        std::vector<std::shared_ptr<Material>> materials;
        if (!scene || scene->mNumMaterials == 0) {
            materials.push_back(make_default_material("default", cache));
            return materials;
        }

        materials.reserve(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            auto material = processMaterial(scene->mMaterials[i], scene, cache, modelDir);
            if (!material) {
                LOG_WARN_F("Failed to process material %d, using fallback", i);
                material = make_default_material("fallback", cache);
            }
            materials.push_back(std::move(material));
        }
        return materials;
    }

    std::shared_ptr<Material> ModelLoader::processMaterial(
        aiMaterial* aiMat,
        const aiScene* scene,
        TextureCache& cache,
        const std::string& modelDir
    ) {
        if (!aiMat) {
            LOG_WARN("Null aiMaterial, using default material");
            return make_default_material("material", cache);
        }

        // Get material name
        aiString ainame;
        std::string name = "material";
        if (aiMat->Get(AI_MATKEY_NAME, ainame) == AI_SUCCESS && ainame.length > 0) {
            name = ainame.C_Str();
        }

        LOG_DEBUG_F("Processing material: %s", name.c_str());

        // Load textures for PBR workflow
        auto base   = loadTextureFromMaterial(aiMat, scene, aiTextureType_DIFFUSE, cache, modelDir);
        auto normal = loadTextureFromMaterial(aiMat, scene, aiTextureType_NORMALS, cache, modelDir);
        auto metal  = loadTextureFromMaterial(aiMat, scene, aiTextureType_METALNESS, cache, modelDir);
        auto rough  = loadTextureFromMaterial(aiMat, scene, aiTextureType_DIFFUSE_ROUGHNESS, cache, modelDir);
        auto ao     = loadTextureFromMaterial(aiMat, scene, aiTextureType_AMBIENT_OCCLUSION, cache, modelDir);
        auto albedo = base; // Albedo is typically the same as base color

        return std::make_shared<Material>(
            name, base, normal, albedo, metal, rough, ao
        );
    }

    std::unordered_map<std::string, PropertyValue> ModelLoader::loadGLTFExtensions(const aiScene* scene) {
        LOG_DEBUG("Loading GLTF extensions from scene");
        
        std::unordered_map<std::string, PropertyValue> extensions;
        
        if (!scene->mMetaData) {
            LOG_DEBUG("No metadata found in scene");
            return extensions;
        }
        
        for (unsigned int i = 0; i < scene->mMetaData->mNumProperties; i++) {
            const aiString& key = scene->mMetaData->mKeys[i];
            const aiMetadataEntry& entry = scene->mMetaData->mValues[i];
            
            std::string keyStr = key.C_Str();
            
            if (keyStr.find("gltf") != std::string::npos || 
                keyStr.find("KHR_") != std::string::npos ||
                keyStr.find("EXT_") != std::string::npos) {
                
                PropertyValue value;
                switch (entry.mType) {
                    case AI_BOOL:
                        value = *static_cast<bool*>(entry.mData);
                        break;
                    case AI_INT32:
                        value = *static_cast<int32_t*>(entry.mData);
                        break;
                    case AI_UINT64:
                        value = static_cast<int>(*static_cast<uint64_t*>(entry.mData));
                        break;
                    case AI_FLOAT:
                        value = *static_cast<float*>(entry.mData);
                        break;
                    case AI_DOUBLE:
                        value = *static_cast<double*>(entry.mData);
                        break;
                    case AI_AISTRING:
                        value = std::string(static_cast<aiString*>(entry.mData)->C_Str());
                        break;
                    default:
                        LOG_WARN_F("Unknown metadata type for key: %s", keyStr.c_str());
                        continue;
                }
                
                extensions[keyStr] = value;
            }
        }
        
        return extensions;
    }

    void ModelLoader::processGLTFNode(
        aiNode* node, 
        const aiScene* scene, 
        std::unordered_map<std::string, PropertyValue>& extensions
    ) {
        LOG_DEBUG_F("Processing GLTF node: %s", node->mName.C_Str());
        
        std::string nodeName = node->mName.C_Str();
        
        if (node->mMetaData) {
            for (unsigned int i = 0; i < node->mMetaData->mNumProperties; i++) {
                const aiString& key = node->mMetaData->mKeys[i];
                const aiMetadataEntry& entry = node->mMetaData->mValues[i];
                
                std::string keyStr = key.C_Str();
                std::string fullKey = nodeName + "." + keyStr;
                
                PropertyValue value;
                switch (entry.mType) {
                    case AI_BOOL:
                        value = *static_cast<bool*>(entry.mData);
                        break;
                    case AI_INT32:
                        value = *static_cast<int32_t*>(entry.mData);
                        break;
                    case AI_UINT64:
                        value = static_cast<int>(*static_cast<uint64_t*>(entry.mData));
                        break;
                    case AI_FLOAT:
                        value = *static_cast<float*>(entry.mData);
                        break;
                    case AI_DOUBLE:
                        value = *static_cast<double*>(entry.mData);
                        break;
                    case AI_AISTRING:
                        value = std::string(static_cast<aiString*>(entry.mData)->C_Str());
                        break;
                    case AI_AIVECTOR3D: {
                        aiVector3D* vec = static_cast<aiVector3D*>(entry.mData);
                        value = std::to_string(vec->x) + "," + 
                                std::to_string(vec->y) + "," + 
                                std::to_string(vec->z);
                        break;
                    }
                    default:
                        continue;
                }
                
                extensions[fullKey] = value;
            }
        }
        
        aiMatrix4x4 transform = node->mTransformation;
        if (!transform.IsIdentity()) {
            std::string transformKey = nodeName + ".transform";
            std::string transformValue;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    if (i != 0 || j != 0) transformValue += ",";
                    transformValue += std::to_string(transform[i][j]);
                }
            }
            extensions[transformKey] = transformValue;
        }
    }

    void ModelLoader::applyGLTFExtensions(
        std::shared_ptr<Model> model, 
        const std::unordered_map<std::string, PropertyValue>& extensions
    ) {
        if (extensions.empty()) {
            return;
        }
        
        LOG_DEBUG_F("Applying %d GLTF extensions to model", static_cast<int>(extensions.size()));
        
        for (const auto& [extensionName, extensionData] : extensions) {
            if (extensionName.find("KHR_materials_unlit") != std::string::npos) {
                LOG_INFO("Model uses unlit material");
            } else if (extensionName.find("KHR_materials_pbrSpecularGlossiness") != std::string::npos) {
                LOG_INFO("Model uses PBR specular-glossiness workflow");
            } else if (extensionName.find("KHR_lights_punctual") != std::string::npos) {
                LOG_INFO("Model contains punctual lights");
            } else if (extensionName.find("KHR_draco_mesh_compression") != std::string::npos) {
                LOG_INFO("Model uses Draco compression");
            } else if (extensionName.find("transform") != std::string::npos) {
                LOG_DEBUG_F("Transform data: %s", extensionName.c_str());
            } else if (extensionName.find("LOD") != std::string::npos || 
                       extensionName.find("lod") != std::string::npos) {
                LOG_INFO_F("LOD information: %s", extensionName.c_str());
            }
        }
    }

}
