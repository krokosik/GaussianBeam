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

#include "OpticsBench.h"
#include "Statistics.h"

#include <iostream>
#include <cmath>

using namespace std;

OpticsBench::OpticsBench()
	: m_cavity(1., 0., 1., 0., 0., 0.)
{
	m_firstCavityIndex = 0;
	m_lastCavityIndex = 0;
	m_ringCavity = true;
	m_optics.clear();

	addOptics(new CreateBeam(180e-6, 10e-3, "w0"), 0);
	m_optics[0]->setAbsoluteLock(true);

	/// @todo remove this later
	m_firstCavityIndex = 1;
	m_lastCavityIndex = 2;
}

OpticsBench::~OpticsBench()
{
	// Delete optics
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		delete (*it);
}

void OpticsBench::registerNotify(OpticsBenchNotify* notify)
{
	m_notifyList.push_back(notify);
}

void OpticsBench::setWavelength(double wavelength)
{
	m_wavelength = wavelength;
	computeBeams();
}

void OpticsBench::addOptics(Optics* optics, int index)
{
	m_optics.insert(m_optics.begin() + index,  optics);
	m_beams.insert(m_beams.begin() + index, Beam());
	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchOpticsAdded(index);
	computeBeams(index);
}

void OpticsBench::removeOptics(int index, int count)
{
	for (int i = index; i < index + count; i++)
	{
		delete m_optics[index];
		m_optics.erase(m_optics.begin() + index);
		m_beams.erase(m_beams.begin() + index);
	}
	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchOpticsRemoved(index, count);
	computeBeams(index);
}

void OpticsBench::setOpticsPosition(int index, double position)
{
	m_optics[index]->setPositionCheckLock(position);
	computeBeams();
}

void OpticsBench::lockTo(int index, std::string opticsName)
{
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it)->name() == opticsName)
		{
			m_optics[index]->relativeLockTo(*it);
			break;
		}
	emitChange(0, nOptics()-1);
}

void OpticsBench::setOpticsName(int index, std::string name)
{
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it)->name() == name)
			return;

	m_optics[index]->setName(name);
	emitChange(0, nOptics()-1);
}

void OpticsBench::opticsPropertyChanged(int /*index*/)
{
	computeBeams();
}

void OpticsBench::setInputBeam(const Beam& beam, bool update)
{
	CreateBeam* createBeam = dynamic_cast<CreateBeam*>(m_optics[0]);
	createBeam->setPosition(beam.waistPosition());
	createBeam->setWaist(beam.waist());
	if (update)
		computeBeams();
}

void OpticsBench::setBeam(const Beam& beam, int index)
{
	m_beams[index] = beam;
	computeBeams(index, true);
}

void OpticsBench::setTargetBeam(const Beam& beam)
{
	m_targetBeam = beam;
	emitChange(0, nOptics()-1);
}

void OpticsBench::computeBeams(int changedIndex, bool backward)
{
	cerr << "computeBeams " << wavelength() << endl;

	Beam beam;

	if (backward)
	{
		beam = m_beams[changedIndex];
		for (int i = changedIndex + 1; i < nOptics(); i++)
			m_beams[i] = beam = m_optics[i]->image(beam);
		beam = m_beams[changedIndex];
		for (int i = changedIndex - 1; i >= 0; i--)
			m_beams[i] = beam = m_optics[i+1]->antecedent(beam);
		setInputBeam(beam, false);
	}
	else
	{
		if (changedIndex == 0)
			beam.setWavelength(wavelength());
		else
			beam = m_beams[changedIndex - 1];

		for (int i = changedIndex; i < nOptics(); i++)
			m_beams[i] = beam = m_optics[i]->image(beam);
	}
/*
	// Compute the cavity
	if (m_firstCavityIndex > 0)
	{
		const Optics& first_cavity_optics = optics(m_firstCavityIndex);
		const Optics& last_cavity_optics  = optics(m_lastCavityIndex);

		// Test coherence
		if (!first_cavity_optics.isABCD() || ! last_cavity_optics.isABCD())
			qDebug() << "Warning : cavity boundaries are not of ABCD type";
		if (!(m_lastCavityIndex > 0) || !(m_lastCavityIndex >= m_firstCavityIndex))
			qDebug() << "Warning m_lastCavityIndex seems to be wrong !";

		// Compute cavity
		m_cavity = dynamic_cast<const ABCD&>(first_cavity_optics);
		for (int i = m_firstCavityIndex + 1; i < m_lastCavityIndex; i++)
		{
			if (optics(i).isABCD())
				m_cavity *= dynamic_cast<const ABCD&>(optics(i));
			FreeSpace freeSpace(optics(i+1).position() - optics(i).endPosition(), optics(i).endPosition());
			m_cavity*= freeSpace;
		}
		if (m_ringCavity)
			for (int i = m_lastCavityIndex; i > m_firstCavityIndex; i--)
			{
				if (optics(i).isABCD())
					m_cavity *= dynamic_cast<const ABCD&>(optics(i));
				FreeSpace freeSpace(optics(i).endPosition() - optics(i-1).endPosition(), optics(i-1).endPosition());
				m_cavity*= freeSpace;
			}
		else
		{
			m_cavity *= dynamic_cast<const ABCD&>(last_cavity_optics);
			/// @todo add a user defined free space between the last and the first optics for linear cavities.
		}

	}*/

	emitChange(changedIndex, nOptics()-1);
}

void OpticsBench::emitChange(int startOptics, int endOptics)
{
	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchDataChanged(startOptics, endOptics);
}

//////////////////////////////////////////
// Cavity stuff

bool OpticsBench::isCavityStable() const
{
	if (m_firstCavityIndex == 0)
		return false;

	if (m_cavity.stabilityCriterion1())
	{
		if (m_cavity.stabilityCriterion2())
			return true;
		else
			cerr << "Cavity stable for 1 and not 2 !!!!";
	}
	else if (m_cavity.stabilityCriterion2())
		cerr << "Cavity stable for 2 and not 1 !!!!";

	return false;
}

const Beam OpticsBench::cavityEigenBeam(int index) const
{
	if (!isCavityStable() ||
	   (index < m_firstCavityIndex) ||
	   (m_ringCavity && (index >= m_lastCavityIndex)) ||
	   (!m_ringCavity && (index > m_lastCavityIndex)))
		return Beam();

	Beam beam = m_cavity.eigenMode(wavelength());

	for (int i = m_firstCavityIndex; i <= index; i++)
		beam = optics(i)->image(beam);

	return beam;
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
