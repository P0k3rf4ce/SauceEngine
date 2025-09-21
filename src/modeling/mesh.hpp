#ifndef MESH_HPP
#define MESH_HPP

#include <string>
#include <vector>

#include <glm/glm.hpp>

using namespace std;

/*
 * Note: order of parameters matters for struct Vertex,
 * since it will be passed directly to openGL
 */
struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

struct Texture {
	unsigned int id;
	string type;
};

class Mesh {
    public:
		// mesh data
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;
		Mesh(vector<Vertex> vertices, vector<unsigned int> indices,
			vector<Texture> textures);
		// TODO once shader class
		// void Draw(Shader &shader);
	private:
		// render data
		unsigned int VAO, VBO, EBO;
		void setupMesh();
};

#endif