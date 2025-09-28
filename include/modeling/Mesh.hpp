#ifndef MESH_HPP
#define MESH_HPP

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "shared/Shader.hpp"

using namespace std;

/*
 * Note: order of parameters matters for struct Vertex,
 * since it will be passed raw to openGL
 */
struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

class Mesh {
    public:
		// mesh data
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		Mesh(vector<Vertex> vertices, vector<unsigned int> indices);
		void Draw(Shader &shader);
	private:
		// render data
		unsigned int VAO, VBO, EBO;
		void setupMesh();

		/*
		 * helper function, ensure vertices/indices/textures are aligned:
		 * - no indices outside the range of vertices
		 * - at least 1 vertex, at least 2 indices
		 */
		bool validate();

		/*
		 * getters
		 */
		// TODO implementation of these functions is pending a Model class, that will store the model matrix
		glm::vec3 getPos(); // <x,y,z> position
		glm::vec3 getScale(); // scale in x,y,z axes
};

#endif
