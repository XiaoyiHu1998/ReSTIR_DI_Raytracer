#ifndef __MODEL_H__
#define __MODEL_H__
#include <vector>
#include <string>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/ext.hpp"

class Model {
private:
    std::vector<glm::vec3> verts;
    std::vector<glm::i32vec3> faces;
public:
    Model(const char *filename);

    int nverts() const;                          // number of vertices
    int nfaces() const;                          // number of triangles

    bool ray_triangle_intersect(const int &fi, const glm::vec3 &orig, const glm::vec3 &dir, float &tnear);
    bool intersect(const glm::vec3 &orig, const glm::vec3 &dir, float &tnear);

    const glm::vec3 &point(int i) const;                   // coordinates of the vertex i
    glm::vec3 &point(int i);                   // coordinates of the vertex i
    int vert(int fi, int li) const;              // index of the vertex for the triangle fi and local index li
    void get_bbox(glm::vec3 &min, glm::vec3 &max); // bounding box for all the vertices, including isolated ones
};

std::ostream& operator<<(std::ostream& out, Model &m);

#endif //__MODEL_H__

