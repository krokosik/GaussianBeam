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

/////////////////////////////////////////////////
// Beam class

Beam::Beam()
{
	m_valid = false;
}

Beam::Beam(double waist, double waistPosition, double wavelength)
	: m_waist(waist)
	, m_waistPosition(waistPosition)
	, m_wavelength(wavelength)
{
	m_valid = true;
}

Beam::Beam(const complex<double>& q, double z, double wavelength)
	: m_wavelength(wavelength)
{
	const double z0 = q.imag();
	m_waist = sqrt(z0*wavelength/M_PI);
	m_waistPosition = z - q.real();
	m_valid = true;
}

double Beam::divergence() const
{
	if (m_waist == 0.)
		return 0.;

	return atan(m_wavelength/(M_PI*m_waist));
}

void Beam::setDivergence(double divergence)
{
	if ((divergence > 0.) && (divergence < M_PI/2.))
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
	return waist()*sqrt(1. + sqr(zred(z)));
}

double Beam::radiusDerivative(double z) const
{
	return waist()/rayleigh()/sqrt(1. + 1./sqr(zred(z)));
}

double Beam::radiusSecondDerivative(double z) const
{
	return waist()/sqr(rayleigh())/pow(1. + sqr(zred(z)), 1.5);
}

double Beam::curvature(double z) const
{
	return (z - waistPosition())*(1. + 1./sqr(zred(z)));
}

double Beam::gouyPhase(double z) const
{
	return atan(zred(z));
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

Optics::Optics(OpticsType type, bool ABCD, double position, string name)
	: m_type(type)
	, m_ABCD(ABCD)
	, m_position(position)
	, m_width(0.)
	, m_name(name)
	, m_absoluteLock(false)
	, m_relativeLockParent(0)
{}

Optics::~Optics()
{
	// Detach all children
	for (list<Optics*>::iterator it = m_relativeLockChildren.begin(); it != m_relativeLockChildren.end(); it++)
		(*it)->m_relativeLockParent = 0;

	relativeUnlock();
}

void Optics::setAbsoluteLock(bool absoluteLock)
{
	if (absoluteLock)
		relativeUnlock();

	m_absoluteLock = absoluteLock;
}

bool Optics::relativeLockedTo(const Optics* const optics) const
{
	return relativeLockRootConst()->isRelativeLockDescendant(optics);
}

bool Optics::relativeLockTo(Optics* optics)
{
	if (relativeLockedTo(optics))
		return false;

	if (m_relativeLockParent)
		relativeUnlock();

	m_relativeLockParent = optics;
	optics->m_relativeLockChildren.push_back(this);

	m_absoluteLock = false;
	return true;
}

bool Optics::relativeUnlock()
{
	if (!m_relativeLockParent)
		return false;

	m_relativeLockParent->m_relativeLockChildren.remove(this);
	m_relativeLockParent = 0;

	return true;
}

Optics* Optics::relativeLockRoot()
{
	if (!m_relativeLockParent)
		return this;
	else
		return m_relativeLockParent->relativeLockRoot();
}

const Optics* Optics::relativeLockRootConst() const
{
	if (!m_relativeLockParent)
		return this;
	else
		return m_relativeLockParent->relativeLockRootConst();
}

bool Optics::isRelativeLockDescendant(const Optics* const optics) const
{
	if (optics == this)
		return true;

	cerr << "Checking if " << name() << " is connected to " << optics->name() << endl;

	for (list<Optics*>::const_iterator it = m_relativeLockChildren.begin(); it != m_relativeLockChildren.end(); it++)
		if ((*it)->isRelativeLockDescendant(optics))
			return true;

	return false;
}

void Optics::moveDescendant(double distance)
{
	setPosition(position() + distance);

	for (list<Optics*>::iterator it = m_relativeLockChildren.begin(); it != m_relativeLockChildren.end(); it++)
		(*it)->moveDescendant(distance);
}

void Optics::setPositionCheckLock(double pos)
{
	if (relativeLockedTreeAbsoluteLock())
		return;

	relativeLockRoot()->moveDescendant(pos - position());
}

/////////////////////////////////////////////////
// CreateBeam class

CreateBeam::CreateBeam(double waist, double waistPosition, string name)
	: Optics(CreateBeamType, false/*Not ABCD*/, waistPosition, name)
	, m_waist(waist)
{}

Beam CreateBeam::image(const Beam& inputBeam) const
{
	return Beam(m_waist, position(), inputBeam.wavelength());
}

Beam CreateBeam::antecedent(const Beam& outputBeam) const
{
	return Beam(m_waist, position(), outputBeam.wavelength());
}

/////////////////////////////////////////////////
// ABCD class

ABCD::ABCD(OpticsType type, double position, std::string name)
	: Optics(type, true/*Is ABCD*/, position, name)
{}

Beam ABCD::image(const Beam& inputBeam) const
{
	const complex<double> qIn = inputBeam.q(position());
	const complex<double> qOut = (A()*qIn + B()) / (C()*qIn + D());
	return Beam(qOut, position() + width(), inputBeam.wavelength());
}

Beam ABCD::antecedent(const Beam& outputBeam) const
{
	const complex<double> qOut = outputBeam.q(position() + width());
	const complex<double> qIn = (B() - D()*qOut) / (C()*qOut - A());
	return Beam(qIn, position(), outputBeam.wavelength());
}

bool ABCD::stabilityCriterion1() const
{
	return fabs((A()+B())/2.) < 1.;
}

bool ABCD::stabilityCriterion2() const
{
	return sqr(D() - A()) + 4.*C()*B() < 0.;
}

Beam ABCD::eigenMode(double wavelength) const
{
	return Beam(complex<double>(-(D() - A())/(2.*C()), -sqrt(-(sqr(D() - A()) + 4.*C()*B()))/(2.*C())), position(), wavelength);
}

GenericABCD operator*(const ABCD& abcd1, const ABCD& abcd2)
{
	double A = abcd1.A()*abcd2.A() + abcd1.B()*abcd2.C();
	double B = abcd1.A()*abcd2.B() + abcd1.B()*abcd2.D();
	double C = abcd1.C()*abcd2.A() + abcd1.D()*abcd2.C();
	double D = abcd1.C()*abcd2.B() + abcd1.D()*abcd2.D();

	/// @todo check if the two abjects are adjacent ?
	return GenericABCD(A, B, C, D, abcd1.width() + abcd2.width(), abcd1.position());
}

GenericABCD operator*=(const ABCD& abcd1, const ABCD& abcd2)
{
	return abcd1*abcd2;
}

/////////////////////////////////////////////////
// GaussianBeam namespace

bool GaussianBeam::magicWaist(vector<Optics*>& optics, const MagicWaistTarget& target)
{
	const int nTry = 1000000;

	for (int i = 0; i < nTry; i++)
	{
		// Scramble lenses
		if (target.scramble)
			for (unsigned int j = 0; j < 3*optics.size(); j++)
				::swap(optics[rand() % (optics.size()-1) + 1], optics[rand() % (optics.size()-1) + 1]);
		// Place lenses
		Beam beam;
		beam.setWavelength(target.beam.wavelength());
		double previousPos = 0.;
		for (unsigned int l = 0; l < optics.size(); l++)
		{
			if (!optics[l]->absoluteLock())
			{
				/// @todo better range determination
				double position = double(rand())/double(RAND_MAX)*(target.beam.waistPosition() - previousPos) + previousPos;
				optics[l]->setPosition(position);
			}
			beam = optics[l]->image(beam);
			previousPos = optics[l]->position();
			/// @bug this is a hack !
			if ((optics[l]->type() == CreateBeamType) && (previousPos > 0.))
				previousPos = 0.;
		}
		// Check waist
		if (target.overlap &&
			(overlap(beam, target.beam) > target.minOverlap) ||
			(!target.overlap) &&
			(fabs(beam.waist() - target.beam.waist()) < target.waistTolerance*target.beam.waist()) &&
		    (fabs(beam.waistPosition() - target.beam.waistPosition()) < target.positionTolerance*target.beam.rayleigh()))
		{
			cerr << "found waist : " << beam.waist() << " @ " << beam.waistPosition() << " // try = " << i << endl;
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

double GaussianBeam::overlap(const Beam& beam1, const Beam& beam2, double z)
{
//	double w1 = beam1.radius(z);
//	double w2 = beam2.radius(z);
//	double w12 = sqr(beam1.radius(z));
//	double w22 = sqr(beam2.radius(z));
//	double k1 = 2.*M_PI/beam1.wavelength();
//	double k2 = 2.*M_PI/beam2.wavelength();
//	double R1 = beam1.curvature(z);
//	double R2 = beam2.curvature(z);
	double zred1 = beam1.zred(z);
	double zred2 = beam2.zred(z);
	double rho = sqr(beam1.radius(z)/beam2.radius(z));

	//double eta = 4./sqr(w1*w2)/(sqr(1./sqr(w1) + 1./sqr(w2)) + sqr((k1/R1 - k2/R2)/2.));
	//double eta = 4./(w12*w22)/(sqr(1./w12 + 1./w22) + sqr(zred1/w12 - zred2/w22));
	double eta = 4.*rho/(sqr(1. + rho) + sqr(zred1 - zred2*rho));

//	cerr << "Coupling = " << eta << " // " << zred1 << " " << zred2 << " " << rho << endl;

	return eta;
}
