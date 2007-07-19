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

#include "GaussianBeam.h"
#include "Statistics.h"

#include <iostream>
#include <cmath>

using namespace std;

/// Beam class

Beam::Beam()
{}

Beam::Beam(double waist, double waistPosition, double wavelength)
	: m_waist(waist)
	, m_waistPosition(waistPosition)
	, m_wavelength(wavelength)
{}

Beam::Beam(const complex<double>& q, double z, double wavelength)
	: m_wavelength(wavelength)
{
	const double z0 = q.imag();
	m_waist = sqrt(z0*wavelength/M_PI);
	m_waistPosition = z - q.real();
}

double Beam::divergence() const
{
	if (m_waist == 0.)
		return 0.;

	return atan(m_wavelength/(M_PI*m_waist));
}

void Beam::setDivergence(double divergence)
{
	if (divergence > 0.)
		m_waist = m_wavelength/(M_PI*tan(divergence));
}

double Beam::rayleigh() const
{
	if (m_wavelength == 0.)
		return 0.;

	return M_PI*sqr(m_waist)/m_wavelength;
}

void Beam::setRayleigh(double rayleigh)
{
	if (rayleigh > 0.)
		m_waist = sqrt(rayleigh*m_wavelength/M_PI);
}

double Beam::radius(double z) const
{
	return waist()*sqrt(1. + sqr(alpha(z)));
}

double Beam::radiusDerivative(double z) const
{
	return waist()/rayleigh()/sqrt(1. + 1./sqr(alpha(z)));
}

double Beam::radiusSecondDerivative(double z) const
{
	return waist()/sqr(rayleigh())/pow(1. + sqr(alpha(z)), 1.5);
}

double Beam::curvature(double z) const
{
	return (z - waistPosition())*(1. + 1./sqr(alpha(z)));
}

double Beam::gouyPhase(double z) const
{
	return atan(alpha(z));
}

complex<double> Beam::q(double z) const
{
	return complex<double>(z - waistPosition(), rayleigh());
}

const int nApprox = 11;
const double xApprox[nApprox] = {0., 0.202, 0.417, 0.659, 0.95, 1.321, 1.827, 2.569, 3.756, 5.878, 10.33};
const double gapApprox = 0.005;

double Beam::approxNextPosition(double currentPosition, Approximation& approximation) const
{
	return currentPosition + (approximation.maxZ - approximation.minZ)/20.;

	/// @todo activate this
	/// @bug shift by waist center

	double lowBound, highBound;

	if (currentPosition < -xApprox[nApprox-1])
		return -xApprox[nApprox-1];
	if (currentPosition >= xApprox[nApprox-1])
		return xApprox[nApprox-1];

	int i;
	for (i = nApprox - 1; i >= 0; i--)
		if (fabs(currentPosition) > xApprox[i])
			break;

	if (i == nApprox - 1)
		cerr << "UNEXPECTED INTERVAL " << currentPosition << endl;

	if (currentPosition < 0.)
	{
		highBound = -xApprox[i];
		lowBound = -xApprox[i+1];
	}
	else
	{
		highBound = xApprox[i+1];
		lowBound = -xApprox[i];
	}

	return currentPosition + (highBound - lowBound)/(gapApprox*waist()/approximation.resolution);
}

/////////////////////////////////////////////////
// Optics class

Optics::Optics(OpticsType type, double position, string name)
	: m_type(type)
	, m_position(position)
	, m_name(name)
	, m_locked(false)
{}

/////////////////////////////////////////////////
// CreateBeam class

CreateBeam::CreateBeam(double waist, double waistPosition, string name)
	: Optics(CreateBeamType, waistPosition, name)
	, m_waist(waist)
{}

Beam CreateBeam::image(const Beam& inputBeam) const
{
	return Beam(m_waist, m_position, inputBeam.wavelength());
}

Beam CreateBeam::antecedent(const Beam& outputBeam) const
{
	return Beam(m_waist, m_position, outputBeam.wavelength());
}

/////////////////////////////////////////////////
// Lens class

Lens::Lens(double focal, double position, string name)
	: Optics(LensType, position, name)
	, m_focal(focal)
{}

Beam Lens::image(const Beam& inputBeam) const
{
/*	old method
	double s        = m_position - inputBeam.waistPosition();
	double prev_z0  = inputBeam.rayleigh();
	double mag      = 1./sqrt(sqr(1. - s/m_focal) + sqr(prev_z0/m_focal));
	return Beam(mag*inputBeam.waist(), 1./(1./m_focal - 1./(s + sqr(prev_z0)/(s-m_focal))) + m_position, inputBeam.wavelength());
*/

	const complex<double> qIn = inputBeam.q(m_position);
	const complex<double> qOut = 1./(1./qIn - 1./focal());
	return Beam(qOut, m_position, inputBeam.wavelength());
}

Beam Lens::antecedent(const Beam& outputBeam) const
{
	const complex<double> qOut = outputBeam.q(m_position);
	const complex<double> qIn = 1./(1./qOut + 1./focal());
	return Beam(qIn, m_position, outputBeam.wavelength());
}

/////////////////////////////////////////////////
// Interface class

Interface::Interface(OpticsType type, double indexRatio, double position, string name)
	: Optics(type, position, name)
	, m_indexRatio(indexRatio)
{}

/////////////////////////////////////////////////
// FlatInterface class

FlatInterface::FlatInterface(double indexRatio, double position, string name)
	: Interface(FlatInterfaceType, indexRatio, position, name)
{}

Beam FlatInterface::image(const Beam& inputBeam) const
{
	const complex<double> qIn = inputBeam.q(m_position);
	const complex<double> qOut = qIn/indexRatio();
	return Beam(qOut, m_position, inputBeam.wavelength());
}

Beam FlatInterface::antecedent(const Beam& outputBeam) const
{
	const complex<double> qOut = outputBeam.q(m_position);
	const complex<double> qIn = qOut*indexRatio();
	return Beam(qIn, m_position, outputBeam.wavelength());
}

/////////////////////////////////////////////////
// CurvedInterface class

CurvedInterface::CurvedInterface(double surfaceRadius, double indexRatio, double position, string name)
	: Interface(CurvedInterfaceType, indexRatio, position, name)
	, m_surfaceRadius(surfaceRadius)
{}

Beam CurvedInterface::image(const Beam& inputBeam) const
{
	const complex<double> qIn = inputBeam.q(m_position);
	const complex<double> qOut = indexRatio()/((1. - indexRatio())/surfaceRadius()  + 1./qIn);
	return Beam(qOut, m_position, inputBeam.wavelength());
}

Beam CurvedInterface::antecedent(const Beam& outputBeam) const
{
	const complex<double> qOut = outputBeam.q(m_position);
	const complex<double> qIn = 1./((1. - indexRatio())/surfaceRadius()  - indexRatio()/qOut);
	return Beam(qIn, m_position, outputBeam.wavelength());
}

/////////////////////////////////////////////////
// GaussianBeam namespace

bool GaussianBeam::magicWaist(const Beam& inputBeam, const Beam& targetBeam, vector<Lens>& optics,
                              double waistTolerance, double positionTolerance, bool scramble)
{
	const int nTry = 1000000;

	for (int i = 0; i < nTry; i++)
	{
		// Scramble lenses
		if (scramble)
			for (unsigned int j = 0; j < 3*optics.size(); j++)
				::swap(optics[rand()%optics.size()], optics[rand()%optics.size()]);
		// Place lenses
		Beam beam = inputBeam;
		double previousPos = inputBeam.waistPosition();
		for (unsigned int l = 0; l < optics.size(); l++)
		{
			if (!optics[l].locked())
			{
				/// @todo better range determination
				double position = double(rand())/double(RAND_MAX)*(targetBeam.waistPosition() - previousPos) + previousPos;
				optics[l].setPosition(position);
			}
			beam = optics[l].image(beam);
			previousPos = optics[l].position();
		}
		// Check waist
		if ((fabs(beam.waist() - targetBeam.waist()) < waistTolerance*targetBeam.waist()) &&
		    (fabs(beam.waistPosition() - targetBeam.waistPosition()) < positionTolerance*targetBeam.rayleigh()))
		{
			cerr << "found waist : " << beam.waist() << " @ " << beam.waistPosition() << " // try = " << i << endl;
			cerr << targetBeam.rayleigh() << endl;
			return true;
		}
	}

	cerr << "Beam not found !!!" << endl;

	return false;
}

Beam GaussianBeam::fitBeam(vector<double> positions, vector<double> radii, double wavelength, double* rho2)
{
	Beam beam;
	beam.setWavelength(wavelength);

	Statistics stats(positions, radii);

	// Some point whithin the fit
	const double z = stats.meanX;
	// beam radius at z
	const double fz = stats.m*z + stats.p;
	// derivative of the beam radius at z
	const double fpz = stats.m;
	// (z - zw)/z0  (zw : position of the waist, z0 : Rayleigh range)
	const double alpha = M_PI*fz*fpz/wavelength;
	// waist
	beam.setWaist(fz/sqrt(1. + sqr(alpha)));
	beam.setWaistPosition(z - beam.rayleigh()*alpha);

	if (rho2)
		*rho2 = stats.rho2;

	return beam;
}
