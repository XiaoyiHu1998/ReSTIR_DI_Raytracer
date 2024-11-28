#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include "model.h"
#include "glm/gtx/string_cast.hpp"

// fills verts and faces arrays, supposes .obj file to have "f " entries without slashes
Model::Model(const char *filename) : verts(), faces() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) {
        std::cerr << "Failed to open " << filename << std::endl;
        return;
    }
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            glm::vec3 v;
            for (int i=0;i<3;i++) iss >> v[i];
            verts.push_back(v);
        } else if (!line.compare(0, 2, "f ")) {
            glm::i32vec3 f;
            int idx, cnt=0;
            iss >> trash;
            while (iss >> idx) {
                idx--; // in wavefront obj all indices start at 1, not zero
                f[cnt++] = idx;
            }
            if (3==cnt) faces.push_back(f);
        }
    }
    std::cerr << "# v# " << verts.size() << " f# "  << faces.size() << std::endl;

    glm::vec3 min, max;
    get_bbox(min, max);
}

// Moller and Trumbore
bool Model::ray_triangle_intersect(const int &fi, const glm::vec3 &orig, const glm::vec3 &dir, float &tnear) {
    glm::vec3 edge1 = point(vert(fi,1)) - point(vert(fi,0));
    glm::vec3 edge2 = point(vert(fi,2)) - point(vert(fi,0));
    glm::vec3 pvec = glm::cross(dir, edge2);
    float det = glm::dot(edge1, pvec);
    if (det<1e-5) return false;

    glm::vec3 tvec = orig - point(vert(fi,0));
    float u = glm::dot(tvec, pvec);
    if (u < 0 || u > det) return false;

    glm::vec3 qvec = glm::cross(tvec, edge1);
    float v = glm::dot(dir, qvec);
    if (v < 0 || u + v > det) return false;

    tnear = glm::dot(edge2, qvec) * (1./det);
    return tnear>1e-5;
}


int Model::nverts() const {
    return (int)verts.size();
}

int Model::nfaces() const {
    return (int)faces.size();
}

void Model::get_bbox(glm::vec3 &min, glm::vec3 &max) {
    min = max = verts[0];
    for (int i=1; i<(int)verts.size(); ++i) {
        for (int j=0; j<3; j++) {
            min[j] = std::min(min[j], verts[i][j]);
            max[j] = std::max(max[j], verts[i][j]);
        }
    }
    std::cerr << "bbox: [" << glm::to_string(min) << " : " << glm::to_string(max) << "]" << std::endl;
}

const glm::vec3 &Model::point(int i) const {
    assert(i>=0 && i<nverts());
    return verts[i];
}

glm::vec3 &Model::point(int i) {
    assert(i>=0 && i<nverts());
    return verts[i];
}

int Model::vert(int fi, int li) const {
    assert(fi>=0 && fi<nfaces() && li>=0 && li<3);
    return faces[fi][li];
}

std::ostream& operator<<(std::ostream& out, Model &m) {
    for (int i=0; i<m.nverts(); i++) {
        out << "v " << glm::to_string(m.point(i)) << std::endl;
    }
    for (int i=0; i<m.nfaces(); i++) {
        out << "f ";
        for (int k=0; k<3; k++) {
            out << (m.vert(i,k)+1) << " ";
        }
        out << std::endl;
    }
    return out;
}

