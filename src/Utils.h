/* This file is part of the GaussianBeam project
   Copyright (C) 2007-2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef UTILS_H
#define UTILS_H

#include <vector>

inline double sqr(double x)
{
	return x*x;
}

inline double sign(double x)
{
	return x < 0. ? -1. : 1.;
}

inline int intSign(double x)
{
	return x < 0. ? -1 : 1;
}

std::vector<double> operator+(const std::vector<double>& v, double r);
std::vector<double> operator-(const std::vector<double>& v, double r);
std::vector<double> operator*(const std::vector<double>& v, double r);
std::vector<double> operator/(const std::vector<double>& v, double r);
inline std::vector<double> operator+(double r, const std::vector<double>& v) { return v + r; }
inline std::vector<double> operator*(double r, const std::vector<double>& v) { return v * r; }
std::vector<double>& operator+=(std::vector<double>& v1, const std::vector<double>& v2);
std::vector<double>& operator-=(std::vector<double>& v1, const std::vector<double>& v2);
std::vector<double> operator+(const std::vector<double>& v1, const std::vector<double>& v2);
std::vector<double> operator-(const std::vector<double>& v1, const std::vector<double>& v2);
std::vector<double>& operator<<(std::vector<double>& v, double r);

namespace Utils
{
	double scalar(const std::vector<double>& v1, const std::vector<double>& v2);
	double norm(const std::vector<double>& v1);
	double distance(const std::vector<double>& v1, const std::vector<double>& v2);
}

#endif
