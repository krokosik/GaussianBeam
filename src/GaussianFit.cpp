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

#include "GaussianFit.h"
#include "Statistics.h"
#include "lmmin.h"

#include <iostream>
#include <cmath>

#define epsilon 1e-50

int Fit::m_fitCount = 0;

using namespace std;

Fit::Fit(int nData, string name)
{
	if (name.empty())
	{
		stringstream stream;
		stream << "Fit" << m_fitCount++ << ends;
		stream >> name;
	}

	m_name = name;
	m_dirty = true;
	m_lastWavelength = 0.;
	/// @todo change this default to Radius_e2
	m_dataType = Diameter_e2;
	m_color = 0;

	for (int i = 0; i < nData; i++)
		addData(0., 0.);
}

int Fit::nonZeroSize() const
{
	int result = 0;

	for (int i = 0; i < size(); i++)
		if (radius(i) > epsilon)
			result++;

	return result;
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

void Fit::removeData(unsigned int index)
{
	m_positions.erase(m_positions.begin() + index);
	m_values.erase(m_values.begin() + index);
}

double Fit::radius(unsigned int index) const
{
	if (m_dataType == Radius_e2)
		return m_values[index];
	else if (m_dataType == Diameter_e2)
		return m_values[index]/2.;
	else  if (m_dataType == standardDeviation)
		return m_values[index]*2.;
	else  if (m_dataType == FWHM)
		return m_values[index]/sqrt(2.*log(2.));
	else  if (m_dataType == HWHM)
		return m_values[index]*sqrt(2./log(2.));

	return 0.;
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

double Fit::residue(double wavelength) const
{
	fitBeam(wavelength);
	return m_residue;
}

/////////////////////////////////////////////////
// Non linear fit functions

void Fit::error(double* par, double* fvec) const
{
	int j = 0;
	/// @todo index = 1., M2 = 1. ?
	Beam beam(par[0], par[1], m_lastWavelength, 1., 1.);

	for (int i = 0; i < size(); i++)
		if (radius(i) > epsilon)
			fvec[j++] = radius(i) - beam.radius(position(i));
}

/**
* @p par   input array. At the end of the minimization, it contains the approximate solution vector.
* @p m_dat positive integer input variable set to the number of functions.
* @p fvec  is an output array of length m_dat which contains the function values the square sum of which ought to be minimized.
* @p data  user data. Here null
* @p info  integer output variable. If set to a negative value, the minimization procedure will stop.
*/
void Fit::lm_evaluate_beam(double *par, int m_dat, double *fvec, void *data, int *info)
{
	const Fit* fit = static_cast<const Fit*>(data);
	fit->error(par, fvec);
	*info = *info;		// to prevent a 'unused variable' warning
	// if <parameters drifted away> { *info = -1; }
}

/**
* @p data  for soft control of printout behaviour, add control variables to the data struct
* @p iflag 0 (init) 1 (outer loop) 2(inner loop) -1(terminated)
* @p iter  outer loop counter
* @p nfev  number of calls to evaluate
*/
void Fit::lm_print_beam(int n_par, double *par, int m_dat, double *fvec, void *data, int iflag, int iter, int nfev)
{
	return;

	if (iflag == 2)
		printf("trying step in gradient direction\n");
	else if (iflag == 1)
		printf("determining gradient (iteration %d)\n", iter);
	else if (iflag == 0)
		printf("starting minimization\n");
	else if (iflag == -1)
		printf("terminated after %d evaluations\n", nfev);

	printf("  par: ");
	for (int i = 0; i < n_par; ++i)
		printf(" %12g", par[i]);
	printf(" => norm: %12g\n", lm_enorm(m_dat, fvec));
}

Beam Fit::nonLinearFit(const Beam& guessBeam, double* residue) const
{
	const int nPar = 2;
	double par[nPar] = {guessBeam.waist(), guessBeam.waistPosition()};

	lm_control_type control;
	lm_initialize_control(&control);
	lm_minimize(nonZeroSize(), nPar, par, Fit::lm_evaluate_beam, NULL, (void*)(this), &control);

	Beam result = guessBeam;
	result.setWaist(par[0]);
	result.setWaistPosition(par[1]);
	*residue = control.fnorm;

	return result;
}

/////////////////////////////////////////////////
// Linear fit functions

Beam Fit::linearFit(const vector<double>& positions, const vector<double>& radii, double wavelength) const
{
	Statistics stats(positions, radii);

	// Some point whithin the fit
	const double z = stats.meanX;
	// beam radius at z
	const double fz = stats.m*z + stats.p;
	// derivative of the beam radius at z
	const double fpz = stats.m;
	// (z - zw)/z0  (zw : position of the waist, z0 : Rayleigh range)
	const double alpha = M_PI*fz*fpz/wavelength;
	/// @todo index = 1., M2 = 1. ?
	Beam result(fz/sqrt(1. + sqr(alpha)), 0., wavelength, 1., 1.);
	result.setWaistPosition(z - result.rayleigh()*alpha);

	return result;
}

/////////////////////////////////////////////////
// Actually do the fit

void Fit::fitBeam(double wavelength) const
{
	if ((!m_dirty && (wavelength == m_lastWavelength)) || (nonZeroSize() < 2))
		return;

	m_lastWavelength = wavelength;

	// Gather all non zero data
	vector<double> positions, radii;
	for (int i = 0; i < size(); i++)
		if (radius(i) > epsilon)
		{
			positions.push_back(position(i));
			radii.push_back(radius(i));
		}

	double residue, tmpResidue;
	Beam tmpBeam;

	// 1/ linear fit with all data
	m_beam = linearFit(positions, radii, wavelength);
	// 2/ non linear fit with the linear fit result as initial guess
	m_beam = nonLinearFit(m_beam, &residue);
	// 3/ Try a linear fit with the first two points + non linear fit on all points
	positions.clear();
	radii.clear();
	for (int i = 0; (i < size()) && (positions.size() < 2); i++)
		if (radius(i) > epsilon)
		{
			positions.push_back(position(i));
			radii.push_back(radius(i));
		}
	tmpBeam = linearFit(positions, radii, wavelength);
	tmpBeam = nonLinearFit(tmpBeam, &tmpResidue);
	if (tmpResidue < residue)
	{
		residue = tmpResidue;
		m_beam = tmpBeam;
	}
	// 4/ Try a linear fit with the last two points + non linear fit on all points
	positions.clear();
	radii.clear();
	for (int i = size()-1; (i >= 0) && (positions.size() < 2); i--)
		if (radius(i) > epsilon)
		{
			positions.push_back(position(i));
			radii.push_back(radius(i));
		}
	tmpBeam = linearFit(positions, radii, wavelength);
	tmpBeam = nonLinearFit(tmpBeam, &tmpResidue);
	if (tmpResidue < residue)
	{
		residue = tmpResidue;
		m_beam = tmpBeam;
	}

	///////////////
	// Terminaison

	m_residue = residue;
	m_dirty = false;
}
