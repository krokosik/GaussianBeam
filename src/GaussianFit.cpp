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

double Fit::rho2(double wavelength) const
{
	fitBeam(wavelength);
	return m_rho2;
}

double Fit::residue(double wavelength) const
{
	fitBeam(wavelength);
	return m_residue;
}

/////////////////////////////////////////////////
// Actually do the fit

void Fit::error(double* par, double* fvec) const
{
	int j = 0;
	/// @todo index = 1., M2 = 1. ?
	Beam beam(par[0], par[1], m_lastWavelength, 1., 1.);

	cerr << beam << endl;

	for (int i = 0; i < size(); i++)
		if (radius(i) > epsilon)
			cerr << (fvec[j++] = radius(i) - beam.radius(position(i)))<< endl;
}

/**
* @p par   input array. At the end of the minimization, it contains the approximate solution vector.
* @p m_dat positive integer input variable set to the number of functions.
* @p fvec  is an output array of length m_dat which contains the function values the square sum of which ought to be minimized.
* @p data  user data. Here null
* @p info  integer output variable. If set to a negative value, the minimization procedure will stop.
*/
void lm_evaluate_beam(double *par, int m_dat, double *fvec, void *data, int *info)
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
void lm_print_beam(int n_par, double *par, int m_dat, double *fvec, void *data, int iflag, int iter, int nfev)
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

void Fit::fitBeam(double wavelength) const
{
	if ((!m_dirty && (wavelength == m_lastWavelength)) || m_positions.empty())
		return;

	m_lastWavelength = wavelength;

	////////////////////////////////////
	// 1st part of the fit : linear fit

	vector<double> positions, radii;
	for (int i = 0; i < size(); i++)
		if (radius(i) > epsilon)
		{
			positions.push_back(position(i));
			radii.push_back(radius(i));
		}

	int nData = positions.size();

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
	m_beam = Beam(fz/sqrt(1. + sqr(alpha)), 0., wavelength, 1., 1.);
	m_beam.setWaistPosition(z - m_beam.rayleigh()*alpha);
	m_rho2 = stats.rho2;

	/////////////////////////////
	// 2nd part : non linear fit
	const int nPar = 2;
	double par[nPar] = {m_beam.waist(), m_beam.waistPosition()};

	// Try to make bad situations better
	for (int i = 0; i < size(); i++)
		if ((radius(i) > epsilon) && (radius(i) < par[0]))
		{
			par[0] = radius(i);
			par[1] = position(i);
		}

	lm_control_type control;
	lm_initialize_control(&control);
	lm_minimize(nData, nPar, par, lm_evaluate_beam, lm_print_beam, (void*)(this), &control);
	printf("status: %s after %d evaluations\n", lm_shortmsg[control.info], control.nfev);

	m_beam.setWaist(par[0]);
	m_beam.setWaistPosition(par[1]);
	m_residue = control.fnorm;

	///////////////
	// Terminaison

	m_dirty = false;
}
