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

#include "GaussianFit.h"
#include "Statistics.h"

#include <iostream>
#include <cmath>

using namespace std;

Fit::Fit()
{
	m_name = "Fit";
	m_dirty = true;
	m_lastWavelength = 0.;
}

void Fit::setData(unsigned int index, double position, double value)
{
	if (m_positions.size() <= index)
	{
		m_positions.resize(index + 1);
		m_values.resize(index + 1);
	}

	m_positions[index] = position;
	m_values[index] = value;
	m_dirty = true;
}

void Fit::addData(double position, double value)
{
	m_positions.push_back(position);
	m_values.push_back(value);
	m_dirty = true;
}

void Fit::clear()
{
	m_positions.clear();
	m_values.clear();
	m_dirty = true;
}

const Beam& Fit::beam(double wavelength) const
{
	fitBeam(wavelength);
	return m_beam;
}

double Fit::rho2(double wavelength) const
{
	fitBeam(wavelength);
	return m_rho2;
}

void Fit::fitBeam(double wavelength) const
{
	if (!m_dirty && (wavelength == m_lastWavelength) || m_positions.empty())
		return;

	cerr << "Fit::fitBeam recomputing fit" << endl;
/*	for (int i = 0; i < size(); i++)
		cerr << position(i) << " " << radius(i) << endl;
*/
	Statistics stats(m_positions, m_values);

	// Some point whithin the fit
	const double z = stats.meanX;
	// beam radius at z
	const double fz = stats.m*z + stats.p;
	// derivative of the beam radius at z
	const double fpz = stats.m;
	// (z - zw)/z0  (zw : position of the waist, z0 : Rayleigh range)
	const double alpha = M_PI*fz*fpz/wavelength;
	m_beam = Beam(fz/sqrt(1. + sqr(alpha)), 0., wavelength);
	m_beam.setWaistPosition(z - m_beam.rayleigh()*alpha);
	m_rho2 = stats.rho2;
	m_dirty = false;
	m_lastWavelength = wavelength;
}
