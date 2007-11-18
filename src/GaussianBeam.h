/* This file is part of the Gaussian Beam project
   Copyright (C) 2007 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>

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

#ifndef GAUSSIANBEAM_H
#define GAUSSIANBEAM_H

#include <complex>


/// @todo some of these are defined by the STL library

inline double sqr(double x)
{
	return x*x;
}
/*
inline double min(double x, double y)
{
	return y < x ? y : x;
}

inline double max(double x, double y)
{
	return y > x ? y : x;
}
*/
inline double sign(double x)
{
	return x < 0. ? -1. : 1.;
}

struct Approximation
{
	double minZ, maxZ;
	double resolution;

//	int currentInterval;
//	double currentValue;
};

class Beam
{
public:
	Beam();
	Beam(double waist, double waistPosition, double wavelength);
	Beam(const std::complex<double>& q, double z, double wavelength);

public:
	double waistPosition() const { return m_waistPosition; }
	void setWaistPosition(double waistPosition) { m_waistPosition = waistPosition; }
	double waist() const { return m_waist; }
	void setWaist(double waist) { m_waist = waist; }
	double divergence() const;
	void setDivergence(double divergence);
	double rayleigh() const;
	void setRayleigh(double rayleigh);
	double wavelength() const { return m_wavelength; }
	void setWavelength(double wavelength) { m_wavelength = wavelength; }

	bool isValid() const { return m_valid; }
	double radius(double z) const;
	double radiusDerivative(double z) const;
	double radiusSecondDerivative(double z) const;
	double curvature(double z) const;
	double gouyPhase(double z) const;
	std::complex<double> q(double z) const;
	inline double zred(double z) const { return (z - waistPosition())/rayleigh(); }

	double approxNextPosition(double currentPosition, Approximation& approximation) const;

private:
	double m_waist;
	double m_waistPosition;
	double m_wavelength;
	bool m_valid;
};

#endif
