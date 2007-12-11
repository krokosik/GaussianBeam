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
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <functional>

using namespace std;

/////////////////////////////////////////////////
// OpticsBenchNotify

OpticsBenchNotify::OpticsBenchNotify(OpticsBench& opticsBench)
	: m_bench(opticsBench)
{
}

/////////////////////////////////////////////////
// Fit

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

/////////////////////////////////////////////////
// OpticsBench

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

	m_fits.push_back(Fit());
}

OpticsBench::~OpticsBench()
{
	// Delete optics
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		delete (*it);
}

int OpticsBench::nFit()
{
	return m_fits.size();
}

Fit& OpticsBench::fit(unsigned int index)
{
	if (index >= m_fits.size())
		m_fits.resize(index + 1);

	return m_fits[index];
}

int OpticsBench::opticsIndex(const Optics* optics) const
{
	/// @todo implement a lookup table
	for (vector<Optics*>::const_iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if (*it == optics)
			return it - m_optics.begin();

	cerr << "Error : looking for an optics that is no more in the optics list" << endl;
	return -1;
}

void OpticsBench::registerNotify(OpticsBenchNotify* notify)
{
	m_notifyList.push_back(notify);
	for (int i = 0; i < nOptics(); i++)
		notify->OpticsBenchOpticsAdded(i);
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
	cerr << "WAIST addOptics 1 " << m_beams[index].waist() << "\n";
	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchOpticsAdded(index);
	cerr << "WAIST addOptics 2 " << m_beams[index].waist() << "\n";
	computeBeams(index);
	cerr << "WAIST addOptics 3 " << m_beams[index].waist() << "\n";
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

int OpticsBench::setOpticsPosition(int index, double position, bool respectAbsoluteLock)
{
	Optics* movedOptics = m_optics[index];
	setOpticsPosition(m_optics, index, position, respectAbsoluteLock);
	computeBeams();

	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it) == movedOptics)
			return it - m_optics.begin();

	return index;
}

void OpticsBench::setOpticsPosition(vector<Optics*>& opticsVector, int index, double position, bool respectAbsoluteLock) const
{
	opticsVector[index]->setPositionCheckLock(position, respectAbsoluteLock);
	sort(opticsVector.begin() + 1, opticsVector.end(), less<Optics*>());
}

void OpticsBench::lockTo(int index, string opticsName)
{
	lockTo(m_optics, index, opticsName);
	emitChange(0, nOptics()-1);
}

void OpticsBench::lockTo(vector<Optics*>& opticsVector, int index, string opticsName) const
{
	for (vector<Optics*>::iterator it = opticsVector.begin(); it != opticsVector.end(); it++)
		if ((*it)->name() == opticsName)
		{
			opticsVector[index]->relativeLockTo(*it);
			break;
		}
}

void OpticsBench::setOpticsName(int index, std::string name)
{
	// Check that the name is not already attributed
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

void OpticsBench::setInputBeam(const Beam& beam)
{
	if (m_optics.size() <= 0)
		addOptics(new CreateBeam(180e-6, 10e-3, "w0"), 0);

	CreateBeam* createBeam = dynamic_cast<CreateBeam*>(m_optics[0]);
	createBeam->setWaist(beam.waist());
	setOpticsPosition(0, beam.waistPosition(), false);
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
	if (m_optics.size() == 0)
		return;

	Beam beam;

	if (backward)
	{
		beam = m_beams[changedIndex];
		for (int i = changedIndex + 1; i < nOptics(); i++)
			m_beams[i] = beam = m_optics[i]->image(beam);
		beam = m_beams[changedIndex];
		for (int i = changedIndex - 1; i >= 0; i--)
			m_beams[i] = beam = m_optics[i+1]->antecedent(beam);
		CreateBeam* createBeam = dynamic_cast<CreateBeam*>(m_optics[0]);
		createBeam->setBeam(beam);
	}
	else
	{
		if (changedIndex == 0)
			beam.setWavelength(wavelength());
		else
			beam = m_beams[changedIndex - 1];

		for (int i = changedIndex; i < nOptics(); i++)
		{
			m_beams[i] = beam = m_optics[i]->image(beam);
			cerr << "  Beam i waist = " << beam.waist() << " position = " << beam.waistPosition() << endl;
		}
	}

	m_sensitivity = gradient(m_optics, m_beams.back(), false/*CheckLock*/, true/*Curvature*/);
//	for (unsigned int i = 0; i < m_sensitivity.size(); i++)
//		cerr << m_sensitivity[i] << endl;
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

vector<Optics*> OpticsBench::cloneOptics() const
{
	vector<Optics*> opticsClone;

	for (vector<Optics*>::const_iterator it = m_optics.begin(); it != m_optics.end(); it++)
	{
		opticsClone.push_back((*it)->clone());
		opticsClone.back()->eraseLockingTree();
	}

	for (vector<Optics*>::const_iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it)->relativeLockParent())
			lockTo(opticsClone, it - m_optics.begin(), (*it)->relativeLockParent()->name());

	return opticsClone;
}

Beam OpticsBench::computeSingleBeam(const std::vector<Optics*>& opticsVector, int index) const
{
	Beam beam;
	beam.setWavelength(wavelength());

	for (int i = 0; i <= index; i++)
		beam = opticsVector[i]->image(beam);

	return beam;
}

vector<double> OpticsBench::gradient(const vector<Optics*>& opticsVector, const Beam& beam, bool checkLock, bool curvature) const
{
	double epsilon = 1e-6;
	vector<double> result;

	vector<Optics*> opticsClone = cloneOptics();

	double initOverlap = GaussianBeam::overlap(beam, computeSingleBeam(opticsVector, opticsVector.size() - 1));

	for (vector<Optics*>::iterator it = opticsClone.begin(); it != opticsClone.end(); it++)
	{
		double initPosition = (*it)->position();
		if (checkLock)
			(*it)->setPositionCheckLock(initPosition + epsilon);
		else
			(*it)->setPosition(initPosition + epsilon);
		double finalOverlap = GaussianBeam::overlap(beam, computeSingleBeam(opticsClone, opticsClone.size() - 1));
		if (checkLock)
			(*it)->setPositionCheckLock(initPosition);
		else
			(*it)->setPosition(initPosition);
		double slope = (finalOverlap - initOverlap)/(curvature ? sqr(epsilon) : epsilon);
		result.push_back(slope);
		//cerr << setprecision(20) << initOverlap << " " << finalOverlap << " " << epsilon << endl;
	}

	for (vector<Optics*>::const_iterator it = opticsClone.begin(); it != opticsClone.end(); it++)
		delete (*it);

	return result;
}

void OpticsBench::emitChange(int startOptics, int endOptics) const
{
	for (std::list<OpticsBenchNotify*>::const_iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchDataChanged(startOptics, endOptics);
}

bool OpticsBench::magicWaist(const Tolerance& tolerance)
{
//	for (int i = 0; i < nOptics(); i++)
//		cerr << "m_optics[" << i << "] " << m_optics[i] << " has parent " << m_optics[i]->relativeLockParent() << endl;

	vector<Optics*> opticsClone = cloneOptics();

	vector<int> opticsMovable;
	for (vector<Optics*>::iterator it = opticsClone.begin(); it != opticsClone.end(); it++)
		if (!(*it)->absoluteLock() && !(*it)->relativeLockParent())
			opticsMovable.push_back(it - opticsClone.begin());

	if (opticsMovable.empty())
		return false;

	const int nTry = 500000;
	bool found = false;

	const double minPos = m_leftBoundary;
	const double maxPos = m_rightBoundary;

	cerr << m_leftBoundary << " LR " << m_rightBoundary << endl;

	for (int i = 0; i < nTry; i++)
	{
		/// @todo find a suitable RNG
		// Randomly moves a random optics
		int index = rand() % (opticsMovable.size());
		double position = double(rand())/double(RAND_MAX)*(maxPos - minPos) + minPos;
		setOpticsPosition(opticsClone, opticsMovable[index], position);

		// Check waist
		Beam beam = computeSingleBeam(opticsClone, nOptics()-1);
		if (tolerance.overlap &&
			(GaussianBeam::overlap(beam, m_targetBeam) > tolerance.minOverlap) ||
			(!tolerance.overlap) &&
			(fabs(beam.waist() - m_targetBeam.waist()) < tolerance.waistTolerance*m_targetBeam.waist()) &&
		    (fabs(beam.waistPosition() - m_targetBeam.waistPosition()) < tolerance.positionTolerance*m_targetBeam.rayleigh()))
		{
			cerr << "found waist : " << beam.waist() << " @ " << beam.waistPosition() << " // try = " << i << endl;
			found = true;
			break;
		}
	}

	if (found)
	{
		/// @todo gradient method
		/// @todo is there a better way to identify an optics than a name ? Create a UID ?
		for (vector<Optics*>::iterator itClone = opticsClone.begin(); itClone != opticsClone.end(); itClone++)
			for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
				if ((*itClone)->name() == (*it)->name())
				{
					(*it)->setPosition((*itClone)->position());
					break;
				}
		sort(m_optics.begin() + 1, m_optics.end(), less<Optics*>());
		computeBeams();
    }
	else
		cerr << "Beam not found !!!" << endl;

	for (vector<Optics*>::iterator it = opticsClone.begin(); it != opticsClone.end(); it++)
		delete (*it);

	return found;
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
