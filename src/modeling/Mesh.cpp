#include <glad/glad.h>

#include "modeling/Mesh.hpp"
#include "shared/Logger.hpp"

using namespace std;

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices)
{
	this->vertices = vertices;
	this->indices = indices;

	if (!this->validate()) {
		throw std::runtime_error("Bad mesh loaded");
	}

	setupMesh();
}

void Mesh::setupMesh()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
		&vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() *
		sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)0);

	// vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, Normal));

	// vertex texture coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, TexCoords));
	glBindVertexArray(0);
}

bool Mesh::validate() {
	auto nvert = this->vertices.size();

	if (nvert < 1) {
		LOG_ERROR("Bad mesh contains no vertices");
		return false;
	}
	if (this->indices.size() <= 1) {
		LOG_ERROR_F("Bad mesh has %d indices",this->indices.size());
		return false;
	}

	for (int i=0; i<this->indices.size(); i++) {
		if (this->indices[i] >= nvert) {
			LOG_ERROR_F("Bad mesh: indices[%d]=%d, exceeding %d vertices",i,this->indices[i],nvert);
			return false;
		}
	}

	return true;
}
