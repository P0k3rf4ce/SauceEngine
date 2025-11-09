// Conversion functions between GLM and Eigen datatypes
// "Inspiration": https://gist.github.com/podgorskiy/04a3cb36a27159e296599183215a71b0
#pragma once
#include <Eigen/Geometry>
#include <glm/matrix.hpp>

template <typename T, int m, int n>
inline glm::mat<m, n, float, glm::packed_highp> E2GLM(const Eigen::Matrix<T, m, n> &em)
{
    glm::mat<m, n, float, glm::packed_highp> mat;
    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            mat[j][i] = em(i, j);
        }
    }
    return mat;
}

template <int m, int n>
inline Eigen::Matrix<float, m, n> GLM2E(const glm::mat<m, n, float, glm::precision::highp> &em)
{
    Eigen::Matrix<float, m, n> mat;
    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            mat(i, j) = em[j][i];
        }
    }
    return mat;
}

inline glm::mat<4, 4, float, glm::packed_highp> E2GLM(const Eigen::Affine3d &em)
{
    glm::mat<4, 4, float, glm::packed_highp> mat;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            mat[j][i] = em(i, j);
        }
    }
    return mat;
}

template <typename T, int m>
inline glm::vec<m, float, glm::packed_highp> E2GLM(const Eigen::Matrix<T, m, 1> &em)
{
    glm::vec<m, float, glm::packed_highp> v;
    for (int i = 0; i < m; ++i)
    {
        v[i] = em(i);
    }
    return v;
}