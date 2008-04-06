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

enum FitDataType {Radius_e2 = 0, Diameter_e2, standardDeviation, FWHM, HWHM};

/**
* Find the waist radius and position for a given set of radii measurement of a Gaussian beam
* It fits the given data with a linear fit, and finds the only hyperbola
* that is tangent to the resulting line
*/
class Fit
{
public:
	Fit(std::string name = "Fit");

public:
	int size() const { return m_positions.size(); }
	int nonZeroSize() const;
	std::string name() const { return m_name; }
	void setName(std::string name) { m_name = name; }
	FitDataType dataType() const { return m_dataType; }
	void setDataType(FitDataType dataType) { m_dataType = dataType; }
	unsigned int color() const { return m_color; }
	void setColor(unsigned int color) { m_color = color; }
	double position(unsigned int index) const { return m_positions[index]; }
	double value(unsigned int index) const { return m_values[index]; }
	double radius(unsigned int index) const;
	void addData(double position, double value);
	void setData(unsigned int index, double position, double value);
	void removeData(unsigned int index);
	void clear();
	const Beam& beam(double wavelength) const;
	double rho2(double wavelength) const;

private:
	void fitBeam(double wavelength) const;

private:
	std::string m_name;
	FitDataType m_dataType;
	std::vector<double> m_positions;
	std::vector<double> m_values;
	unsigned int m_color;

	mutable bool m_dirty;
	mutable Beam m_beam;
	mutable double m_rho2;
	mutable double m_lastWavelength;
};

#endif
