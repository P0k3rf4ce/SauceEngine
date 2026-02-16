#include <physics/Vertex.hpp>

#include <glm/glm.hpp>

namespace physics {

struct Constraint {

  virtual void solve(
      std::vector<Vertex>& vertices, 
      std::vector<glm::vec3>& lagrangeMultipliers, 
      float deltatime
  ) const = 0;

  float compliance;

};

}
