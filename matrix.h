#ifndef __MATRIX_H__
#define __MATRIX_H__
#include <cmath>
#include <vector>
#include <cassert>
#include <iostream>
#include <geometry.h>

template <size_t DIM_ROWS, size_t DIM_COLS, typename T> struct matrix
{
	matrix() { for (size_t i = DIM_ROWS * DIM_COLS; i--; data_[i] = T()); }
	matrix(double** a)
	{
		for (size_t i = 0; i < DIM_ROWS; i++)
		{
			for (size_t j = 0; j < DIM_COLS; j++)
			{
				data_[i * DIM_ROWS + j] = a[i][j];
			}
		}
	}
	T& operator()(const size_t i, const size_t j) { assert(i < DIM_ROWS && j < DIM_COLS); return data_[i * DIM_ROWS + j]; }
	const T& operator()(const size_t i, const size_t j) { const assert(i < DIM_ROWS && j < DIM_COLS); return data_[i * DIM_ROWS + j]; }
	matrix<DIM_COLS, DIM_ROWS, T> transpose()
	{
		matrix<DIM_COLS, DIM_ROWS, T> ret = matrix<DIM_COLS, DIM_ROWS, T>();
		for (size_t i = 0; i < DIM_ROWS; i++)
		{
			for (size_t j = 0; j < DIM_COLS; j++)
			{
				ret(j, i) = data_[i * DIM_ROWS + j];
			}
		}
		return ret;
	}
private:
	T data_[DIM_ROWS * DIM_COLS];
};

typedef matrix<4, 4, float> Matrix4f;

template<typename T> matrix<4, 4, T> createTranslation(vec<3, T> translation)
{
	matrix<4, 4, T> ret = identity<4, T>(4);
	ret(0, 3) = translation[0];
	ret(1, 3) = translation[1];
	ret(2, 3) = translation[2];
	return ret;
}

template<typename T> matrix<4, 4, T> createScale(vec<3, T> scale)
{
	matrix<4, 4, T> ret = identity<4, T>(4);
	ret(0, 0) = scale[0];
	ret(1, 1) = scale[1];
	ret(2, 2) = scale[2];
	return ret;
}

template<typename T> matrix<4, 4, T> createRotation(float angle, vec<3, T> axis)
{
	matrix<4, 4, T> ret = identity<4, T>(4);
	float c = cos(angle);
	float s = sin(angle);
	float t = 1 - c;
	float x = axis.x, y = axis.y, z = axis.z;
	ret(0, 0) = t * x * x + c;
	ret(0, 1) = t * x * y - z * s;
	ret(0, 2) = t * x * z + y * s;
	ret(1, 0) = t * x * y + z * s;
	ret(1, 1) = t * y * y + c;
	ret(1, 2) = t * y * z - x * s;
	ret(2, 0) = t * x * z - y * s;
	ret(2, 1) = t * y * z + x * s;
	ret(2, 2) = t * z * z + c;
	return ret;
}

template<typename T> matrix<4, 4, T> createReflection(vec<3, T> normal)
{
	float a = normal.x, b = normal.y, c = normal.z;
	matrix<4, 4, T> ret = identity<4, T>(4);
	ret(0, 0) = 1 - 2 * a * a;
	ret(0, 1) = -2 * a * b;
	ret(0, 2) = -2 * a * c;
	ret(1, 0) = -2 * a * b;
	ret(1, 1) = 1 - 2 * b * b;
	ret(1, 2) = -2 * b * c;
	ret(2, 0) = -2 * a * c;
	ret(2, 1) = -2 * b * c;
	ret(2, 2) = 1 - 2 * c * c;
}

template<size_t DIM_ROWS, size_t DIM_COLS, typename T> vec<DIM_ROWS, T> operator*(const matrix<DIM_ROWS, DIM_COLS, T>& lhs, const vec<DIM_COLS, T>& rhs)
{
	vec<DIM_ROWS, T> ret = vec<DIM_ROWS, T>();
	for (size_t i = 0; i < DIM_ROWS; i++)
	{
		for (size_t j = 0; j < DIM_COLS; j++)
		{
			ret[i] += lhs(i, j) * rhs[j];
		}
	}
	return ret;
}

template<size_t DIM, typename T>matrix<DIM, DIM, T> identity(const size_t DIM)
{
	matrix<DIM, DIM, T> ret = matrix<DIM, DIM, T>();
	for (size_t i = 0; i < DIM; i++)
	{
		ret(i, i) = T(1);
	}
	return ret;
}

template<size_t DIM, size_t DIM_ROWS_1, size_t DIM_COLS_2, typename T>matrix<DIM_ROWS_1, DIM_COLS_2, T> operator*(const matrix<DIM_ROWS_1, DIM, T>& lhs, const matrix<DIM, DIM_COLS_2, T>& rhs)
{
	matrix<DIM_ROWS_1, DIM_COLS_2, T> ret = matrix<DIM_ROWS_1, DIM_COLS_2, T>();
	for (size_t i = 0; i < DIM_ROWS_1; i++)
	{
		for (size_t j = 0; j < DIM_COLS_2; j++)
		{
			for (size_t k = 0; k < DIM; k++)
			{
				ret(i, j) += lhs(i, k) * rhs(k, j);
			}
		}
	}
	return ret;
}

template<size_t DIM_ROWS, size_t DIM_COLS, typename T>matrix<DIM_ROWS, DIM_COLS, T> operator+(const matrix<DIM_ROWS, DIM_COLS, T>& lhs, const matrix<DIM_ROWS, DIM_COLS, T>& rhs)
{
	matrix<DIM_ROWS, DIM_COLS, T> ret = matrix<DIM_ROWS, DIM_COLS, T>();
	for (size_t i = 0; i < DIM_ROWS; i++)
	{
		for (size_t j = 0; j < DIM_COLS; j++)
		{
			ret(i, j) = lhs(i, j) + rhs(i, j);
		}
	}
	return ret;
}

template<size_t DIM_ROWS, size_t DIM_COLS, typename T>matrix<DIM_ROWS, DIM_COLS, T> operator-(const matrix<DIM_ROWS, DIM_COLS, T>& lhs, const matrix<DIM_ROWS, DIM_COLS, T>& rhs)
{
	matrix<DIM_ROWS, DIM_COLS, T> ret = matrix<DIM_ROWS, DIM_COLS, T>();
	for (size_t i = 0; i < DIM_ROWS; i++)
	{
		for (size_t j = 0; j < DIM_COLS; j++)
		{
			ret(i, j) = lhs(i, j) - rhs(i, j);
		}
	}
	return ret;
}

template<size_t DIM_ROWS, size_t DIM_COLS, typename T, typename U> matrix<DIM_ROWS, DIM_COLS, T> operator*(const matrix<DIM_ROWS, DIM_COLS, T>& lhs, const U& rhs)
{
	matrix<DIM_ROWS, DIM_COLS, T> ret;
	for (size_t i = 0; i < DIM_ROWS; i++)
	{
		for (size_t j = 0; j < DIM_COLS; j++)
		{
			ret(i, j) = lhs(i, j) * rhs;
		}
	}
	return ret;
}

template<size_t DIM_ROWS, size_t DIM_COLS, typename T> matrix<DIM_ROWS, DIM_COLS, T> operator-(const matrix<DIM_ROWS, DIM_COLS, T>& lhs)
{
	return lhs * T(-1);
}

template <size_t DIM_ROWS, size_t DIM_COLS, typename T> std::ostream& operator<<(std::ostream& out, const matrix<DIM_ROWS, DIM_COLS, T>& v)
{
	for (size_t i = 0; i < DIM_ROWS; i++)
	{
		for (size_t j = 0; j < DIM_COLS; j++)
		{
			out << v(i, j) << " ";
		}
		out << std::endl;
	}
	return out;
}
#endif //__GEOMETRY_H__

