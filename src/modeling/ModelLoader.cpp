#include "modeling/ModelProperties.hpp"
#include "modeling/ModelLoader.hpp"
#include "modeling/Mesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <iostream> 
#include <string>

using namespace std;

// Container to store loaded meshes
static std::vector<Mesh> loaded_meshes;


namespace modeling {

std::string ModelLoader::directory;

// ---- internal helpers (not in header file) ----
static void processNode(aiNode* node, const aiScene* scene);
static Mesh processMesh(aiMesh* mesh, const aiScene* scene);
static std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);
static unsigned int TextureFromFile(const char* path, const string& directory);

const std::string& ModelLoader::getDirectory() { 
    return directory; 
}

std::size_t ModelLoader::debug_mesh_count() {
    return loaded_meshes.size();
}

// to match ModelProperties.hpp, file-scope storage for the path (since ModelProperties only stores it temporarily, not as member)
// static std::string s_gltfPath;

// ModelProperties::ModelProperties(std::string gltfFilename) {
//     s_gltfPath = std::move(gltfFilename);
// }


// -- Main function to load a model from file --
void ModelLoader::load(const std::string& path)
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(
        path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals
    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);

}

// Recursive helper node traversal function as in the textbook
static void processNode(aiNode* node, const aiScene* scene)
{
    // process all the node’s meshes
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        
        loaded_meshes.push_back(processMesh(mesh, scene));
    }

    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

static Mesh processMesh(aiMesh *mesh, const aiScene *scene)
{
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        // process vertex positions, normals and texture coordinates
        glm::vec3 vector;

        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;
        if (mesh->mTextureCoords[0]) {
         // from textbook
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else{
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }
    // process indices
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
        }

    // process material
    if(mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(),
        specularMaps.end());
    }

    return Mesh(vertices, indices, textures);
}


static std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, const std::string& typeName) {
    vector<Texture> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++){
        aiString str;
        mat->GetTexture(type, i, &str);
        Texture texture;
        texture.id = TextureFromFile(str.C_Str(), ModelLoader::getDirectory());
        texture.type = typeName;
        // I think we can add more fields here later like path
        textures.push_back(texture);
    }
    return textures;
}

// Placeholder implementation for texture loading
static unsigned int TextureFromFile(const char* /*path*/, const string& /*directory*/) {
    return 0;
}
}
