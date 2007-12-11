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

#ifndef GAUSSIANFIT_H
#define GAUSSIANFIT_H

#include "GaussianBeam.h"

#include <vector>

/**
* Find the waist radius and position for a given set of radii measurement of a Gaussian beam
* It fits the given data with a linear fit, and finds the only hyperbola
* that is tangent to the resulting line
*/
class Fit
{
public:
	Fit();

public:
	std::string name() const { return m_name; }
	int size() const { return m_positions.size(); }
	double position(unsigned int index) const { return m_positions[index]; }
	double radius(unsigned int index) const { return m_values[index]; }
	void setName(std::string name) { m_name = name; }
	void setData(unsigned int index, double position, double value);
	void addData(double position, double value);
	void clear();
	const Beam& beam(double wavelength) const;
	double rho2(double wavelength) const;

private:
	void fitBeam(double wavelength) const;

private:
	std::string m_name;
	std::vector<double> m_positions;
	std::vector<double> m_values;
	mutable bool m_dirty;
	mutable Beam m_beam;
	mutable double m_rho2;
	mutable double m_lastWavelength;
};

#endif
