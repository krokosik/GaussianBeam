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
	Beam(double waist, double waistPosition, double wavelength, double index, double M2);
	Beam(const std::complex<double>& q, double z, double wavelength, double index, double M2);

public:
	bool isValid() const { return m_valid; }

	// get/set properties
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
	double index() const { return m_index; }
	void setIndex(double index) { m_index = index; }
	double M2() const { return m_M2; }
	void setM2(double M2) { m_M2 = M2; }

	// Position dependent properties
	double radius(double z) const;
	double radiusDerivative(double z) const;
	double radiusSecondDerivative(double z) const;
	double curvature(double z) const;
	double gouyPhase(double z) const;
	std::complex<double> q(double z) const;

	/**
	* Compute the intensity overlap between beams @p beam1 and @p beam2 at position @p z
	* This overlap does not depend on @p z if both beams have the same wavelength,
	* hence the default value for z
	*/
	static double overlap(const Beam& beam1, const Beam& beam2, double z = 0.);
	double approxNextPosition(double currentPosition, Approximation& approximation) const;

private:
	inline double zred(double z) const { return (z - waistPosition())/rayleigh(); }

private:
	double m_waist;
	double m_waistPosition;
	double m_wavelength;
	double m_index;
	double m_M2;
	bool m_valid;
};

#endif
