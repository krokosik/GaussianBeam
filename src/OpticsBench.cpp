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

#include "OpticsBench.h"
#include "OpticsFunction.h"
#include "Utils.h"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <functional>
#include <sstream>

using namespace std;

/*
OpticsTreeItem::OpticsTreeItem(Optics* optics, OpticsTreeItem* parent)
{
	m_optics = optics;
	m_parent = 0;
	m_child = 0;
	m_image = new Beam();
	insert(parent);
}

Beam* OpticsTreeItem::axis()
{
	if (m_parent == 0)
		return 0;

	return m_parent->image();
}

void OpticsTreeItem::move(OpticsTreeItem* parent)
{
	if (parent == 0)
		return;

	remove();
	insert(parent);
}

void OpticsTreeItem::insert(OpticsTreeItem* parent)
{
	if (parent == 0)
		return;

	// Attach old parent's child to this item
	OpticsTreeItem* child = parent->m_child;
	m_child = child;
	if (child)
		child->m_parent = this;

	// Attach to the parent
	m_parent = parent;
	m_parent->m_child = this;
}

void OpticsTreeItem::remove()
{
	m_parent->m_child = m_child;
	if (m_child)
		m_child->m_parent = m_parent;

	m_parent = 0;
	m_child = 0;
}

void OpticsTreeItem::sort()
{
	double position = optics()->position();
	for (OpticsTreeItem* newParent = parent; (newParent != 0) && (newParent->optics()->position()
	while (
}

*/

/////////////////////////////////////////////////
// OpticsBench

OpticsBench::OpticsBench(QObject* parent) : QObject(parent)
{
	m_opticsPrefix[LensType]            = "L";
	m_opticsPrefix[FlatMirrorType]      = "M";
	m_opticsPrefix[CurvedMirrorType]    = "R";
	m_opticsPrefix[FlatInterfaceType]   = "I";
	m_opticsPrefix[CurvedInterfaceType] = "C";
	m_opticsPrefix[GenericABCDType]     = "G";
	m_opticsPrefix[DielectricSlabType]  = "D";

	m_wavelength = 461e-9;
	m_leftBottomBoundary = vector<double>(2);
	m_leftBottomBoundary[0] = -0.1;
	m_leftBottomBoundary[1] = -1.; /// @bug change
	m_rightTopBoundary = vector<double>(2);
	m_rightTopBoundary[0] = 0.7;
	m_rightTopBoundary[1] = 1.; /// @bug change

	CreateBeam* inputBeam = new CreateBeam(180e-6, 10e-3, 1., "w0");
	inputBeam->setAbsoluteLock(true);
	addOptics(inputBeam, 0);

	addFit(0, 3);
}

OpticsBench::~OpticsBench()
{
	// Delete optics
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		delete (*it);

	// Delelte beams
	for (vector<Beam*>::iterator it = m_beams.begin(); it != m_beams.end(); it++)
		delete (*it);
}

/////////////////////////////////////////////////
// Cavity

void OpticsBench::detectCavities()
{
	static const double epsilon = 1e-10;

	// Cavity detection criterions for a given beam to close a cavity with a given optics
	// - The optics is on the beam optics axis
	// - The optics is in the beam range
	// - The beam is while it is not copropagating with the optics antecedent.
	// - The image of the beam by the optics is copropagating with the actual optics image

	for (int i = 0; i < nOptics(); i++)
		for (int j = 1; j < i; j++)
		{
			Beam* beam = m_beams[i];
			Optics* optics = m_optics[j];
			vector<double> opticsCoordinates = beam->beamCoordinates(m_beams[j-1]->absoluteCoordinates(optics->position()));
			cerr << "Checking cavity " << i << " and " << j << endl;
			if (   (fabs(opticsCoordinates[1]) < epsilon)
			    && (opticsCoordinates[0] >= beam->start())
			    && (opticsCoordinates[0] <= beam->stop())
			    && !Beam::copropagating(*beam, *m_beams[j-1])
			    && Beam::copropagating(optics->image(*beam, *m_beams[j-1]), *m_beams[j]))
				cerr << " Detected cavity around beams " << i << " and " << j << endl;
		}

}

/////////////////////////////////////////////////
// Fit

int OpticsBench::nFit() const
{
	return m_fits.size();
}

Fit* OpticsBench::addFit(unsigned int index, int nData)
{
	Fit* fit = new Fit(nData, this);
	m_fits.insert(m_fits.begin() + index, fit);
	connect(fit, SIGNAL(changed()), this, SLOT(onFitChanged()));
	emit(fitAdded(index));
	return fit;
}

Fit* OpticsBench::fit(unsigned int index)
{
	if (index < m_fits.size())
		return m_fits[index];

	return 0;
}

void OpticsBench::removeFit(unsigned int index)
{
	removeFits(index, 1);
}

void OpticsBench::removeFits(unsigned int startIndex, int n)
{
	m_fits.erase(m_fits.begin() + startIndex, m_fits.begin() + startIndex + n);
	emit(fitsRemoved(startIndex, n));
}

void OpticsBench::onFitChanged()
{
	int index = 0;

	for (vector<Fit*>::iterator it = m_fits.begin(); it != m_fits.end(); it++)
		if ((*it) == sender())
		{
			index = it - m_fits.begin();
			break;
		}

	emit(fitDataChanged(index));
}

/////////////////////////////////////////////////
// Parameters

void OpticsBench::setWavelength(double wavelength)
{
	m_wavelength = wavelength;
	m_targetBeam.setWavelength(m_wavelength);
	setTargetBeam(m_targetBeam);
	computeBeams();

	emit(wavelengthChanged());
}

void OpticsBench::setLeftBoundary(double leftBoundary)
{
	if (leftBoundary < m_rightTopBoundary[0])
		m_leftBottomBoundary[0] = leftBoundary;

	updateExtremeBeams();

	emit(boundariesChanged());
}

void OpticsBench::setRightBoundary(double rightBoundary)
{
	if (rightBoundary > m_leftBottomBoundary[0])
		m_rightTopBoundary[0] = rightBoundary;

	updateExtremeBeams();

	emit(boundariesChanged());
}

/////////////////////////////////////////////////
// Optics

int OpticsBench::opticsIndex(const Optics* optics) const
{
	/// @todo implement a lookup table
	for (vector<Optics*>::const_iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if (*it == optics)
			return it - m_optics.begin();

	cerr << "Error : looking for an optics that is no more in the optics list" << endl;
	return -1;
}

void OpticsBench::addOptics(Optics* optics, int index)
{
	Beam* beam = new Beam(wavelength());

/*	OpticsTreeItem* parent = index < m_opticsTree.size() ? &m_opticsTree[index] : 0;
	m_opticsTree.insert(m_opticsTree.begin() + index, OpticsTreeItem(optics, parent));
*/
	m_optics.insert(m_optics.begin() + index,  optics);
	m_beams.insert(m_beams.begin() + index, beam);

	emit(opticsAdded(index));
	computeBeams(index);
}

void OpticsBench::addOptics(OpticsType opticsType, int index)
{
	Optics* optics;
	stringstream stream;
	stream << m_opticsPrefix[opticsType] << ++m_lastOpticsName[opticsType] << ends;
	string name;
	stream >> name;

	if (opticsType == LensType)
		optics = new Lens(0.1, 0.0, name);
	else if (opticsType == FlatMirrorType)
		optics = new FlatMirror(0.0, name);
	else if (opticsType == CurvedMirrorType)
		optics = new CurvedMirror(0.05, 0.0, name);
	else if (opticsType == FlatInterfaceType)
		optics = new FlatInterface(1.5, 0.0, name);
	else if (opticsType == CurvedInterfaceType)
		optics = new CurvedInterface(0.1, 1.5, 0.0, name);
	else if (opticsType == DielectricSlabType)
		optics = new DielectricSlab(1.5, 0.1, 0.0, name);
	else if (opticsType == GenericABCDType)
		optics = new GenericABCD(1.0, 0.2, 0.0, 1.0, 0.1, 0.0, name);
	else
		return;

	if (index > 0)
		optics->setPosition(OpticsBench::optics(index-1)->position() + 0.05);

	addOptics(optics, index);
}

void OpticsBench::removeOptics(int index, int count)
{
	for (int i = index; i < index + count; i++)
	{
		delete m_optics[index];
		m_optics.erase(m_optics.begin() + index);
		delete m_beams[index];
		m_beams.erase(m_beams.begin() + index);
	}

	emit(opticsRemoved(index, count));
	computeBeams(index);
}

int OpticsBench::setOpticsPosition(int index, double position, bool respectAbsoluteLock)
{
	Optics* movedOptics = m_optics[index];
	m_optics[index]->setPositionCheckLock(position, respectAbsoluteLock);
	sort(m_optics.begin() + 1, m_optics.end(), less<Optics*>());
	computeBeams();

	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it) == movedOptics)
			return it - m_optics.begin();

	return index;
}

void OpticsBench::lockTo(int index, string opticsName)
{
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it)->name() == opticsName)
		{
			m_optics[index]->relativeLockTo(*it);
			break;
		}
	emit(dataChanged(0, nOptics()-1));
}

void OpticsBench::lockTo(int index, int id)
{
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it)->id() == id)
		{
			m_optics[index]->relativeLockTo(*it);
			break;
		}
	emit(dataChanged(0, nOptics()-1));
}

void OpticsBench::printTree()
{
	cerr << "Locking tree" << endl;

	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
	{
		cerr << " Optics " << (*it)->name() << " " <<  (*it)->id() << endl;
		if ((*it)->absoluteLock())
			cerr << "  absolutely locked" << endl;
		if (const Optics* parent = (*it)->relativeLockParent())
			cerr << "  parent = " << parent->name() << " " << parent->id() << endl;
		for (list<Optics*>::const_iterator cit = (*it)->relativeLockChildren().begin(); cit != (*it)->relativeLockChildren().end(); cit++)
			cerr << "  child = " << (*cit)->name() << " " << (*cit)->id() << endl;
	}
}

void OpticsBench::setOpticsName(int index, std::string name)
{
	// Check that the name is not already attributed
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it)->name() == name)
			return;

	m_optics[index]->setName(name);
	emit(dataChanged(0, nOptics()-1));
}

void OpticsBench::opticsPropertyChanged(int /*index*/)
{
	computeBeams();
}

/////////////////////////////////////////////////
// Beams

void OpticsBench::setInputBeam(const Beam& beam)
{
	if (m_optics.size() <= 0)
		addOptics(new CreateBeam(180e-6, 10e-3, 1., "w0"), 0);

	CreateBeam* createBeam = dynamic_cast<CreateBeam*>(m_optics[0]);
	createBeam->setWaist(beam.waist());
	setOpticsPosition(0, beam.waistPosition(), false);
	computeBeams();
}

void OpticsBench::setBeam(const Beam& beam, int index)
{
	*m_beams[index] = beam;
	computeBeams(index, true);
}

void OpticsBench::setTargetBeam(const TargetBeam& beam)
{
	m_targetBeam = beam;
	emit(targetBeamChanged());
}

const Beam* OpticsBench::axis(int index) const
{
	if (index == 0)
		return m_beams[0];
	else
		return m_beams[index - 1];
}

void OpticsBench::updateExtremeBeams()
{
	if (nOptics() > 0)
	{
		m_beams[0]->setStart(m_beams[0]->rectangleIntersection(m_leftBottomBoundary, m_rightTopBoundary)[0]);
		m_beams[nOptics()-1]->setStop(m_beams[nOptics()-1]->rectangleIntersection(m_leftBottomBoundary, m_rightTopBoundary)[1]);
	}
}

void OpticsBench::computeBeams(int changedIndex, bool backward)
{
	if (m_optics.size() == 0)
		return;

	if (backward)
	{
		for (int i = changedIndex + 1; i < nOptics(); i++)
			*m_beams[i] = m_optics[i]->image(*m_beams[i-1]);
		for (int i = changedIndex - 1; i >= 0; i--)
			*m_beams[i] = m_optics[i+1]->antecedent(*m_beams[i+1]);
		CreateBeam* createBeam = dynamic_cast<CreateBeam*>(m_optics[0]);
		createBeam->setBeam(*m_beams[0]);
	}
	else
	{
		if (changedIndex == 0)
		{
			Beam beam(wavelength());
			*m_beams[0] = m_optics[0]->image(beam);
		}

		for (int i = ::max(changedIndex, 1); i < nOptics(); i++)
			*m_beams[i] = m_optics[i]->image(*m_beams[i-1]);
	}

	for (int i = 0; i < nOptics(); i++)
	{
		if (i != 0)
			m_beams[i]->setStart(m_optics[i]->position() + m_optics[i]->width());
		if (i < nOptics()-1)
			m_beams[i]->setStop(m_optics[i+1]->position());
	}
	updateExtremeBeams();
	detectCavities();

	OpticsFunction function(m_optics, m_wavelength);
	function.setOverlapBeam(*m_beams.back());
	function.setCheckLock(false);

	m_sensitivity = function.curvature(function.currentPosition())/2.;

	emit(dataChanged(changedIndex, nOptics()-1));
}

pair<Beam*, double> OpticsBench::closestPosition(const vector<double>& point, int preferedSide) const
{
	const double epsilon = 1e-5;
	double bestPosition = 0.;
	double bestDistance = 1e300;
	Beam* bestBeam = 0;

	for (int i = 0; i < nOptics(); i++)
	{
		vector<double> coord = m_beams[i]->beamCoordinates(point);
		double newDistance = coord[1];

//		if (newDistance > bestDistance)
//			continue;

		if (coord[0] < m_beams[i]->start())
			newDistance = Utils::distance(point, m_beams[i]->absoluteCoordinates(m_beams[i]->start()));
		else if (coord[0] > m_beams[i]->stop())
			newDistance = Utils::distance(point, m_beams[i]->absoluteCoordinates(m_beams[i]->stop()));

		double rho = fabs(newDistance/bestDistance) - 1.;
		if ((rho < -epsilon) ||
			((fabs(rho) < epsilon) && (intSign(coord[1])*preferedSide > 0)))
		{
			bestPosition = coord[0];
			bestDistance = newDistance;
			bestBeam = m_beams[i];
		}
	}

	return pair<Beam*, double>(bestBeam, bestPosition);
}

/////////////////////////////////////////////////
// Magic waist

bool OpticsBench::magicWaist()
{
	OpticsFunction function(m_optics, m_wavelength);
	function.setOverlapBeam(m_targetBeam);
	function.setCheckLock(true);

	vector<int> opticsMovable;
	for (int i = 0; i < nOptics(); i++)
		if (!optics(i)->absoluteLock() && !optics(i)->relativeLockParent())
			opticsMovable.push_back(i);

	if (opticsMovable.empty())
		return false;

	const int nTry = 500000;
	bool found = false;

	/// @bug this has to change !
	const double minPos = m_leftBottomBoundary[0];
	const double maxPos = m_rightTopBoundary[0];

	vector<double> positions = function.currentPosition();

	for (int i = 0; i < nTry; i++)
	{
		/// @todo find a suitable RNG
		// Randomly moves a random optics
		int index = rand() % nOptics();
		double position = double(rand())/double(RAND_MAX)*(maxPos - minPos) + minPos;
		positions[index] = position;
//		positions = function.localMaximum(positions);

		// Check waist
		Beam beam = function.beam(positions);
		if ((m_targetBeam.overlapCriterion() &&
			(Beam::overlap(beam, m_targetBeam) > m_targetBeam.minOverlap())) ||
			((!m_targetBeam.overlapCriterion()) &&
			(fabs(beam.waist() - m_targetBeam.waist()) < m_targetBeam.waistTolerance()*m_targetBeam.waist()) &&
		    (fabs(beam.waistPosition() - m_targetBeam.waistPosition()) < m_targetBeam.positionTolerance()*m_targetBeam.rayleigh())))
		{
			cerr << "found waist : " << beam.waist() << " @ " << beam.waistPosition() << " // try = " << i << endl;
			found = true;
			break;
		}
	}

	positions = function.localMaximum(positions);

	if (found)
	{
		for (unsigned int i = 0; i < positions.size(); i++)
			m_optics[i]->setPositionCheckLock(positions[i]);
		sort(m_optics.begin() + 1, m_optics.end(), less<Optics*>());
		computeBeams();
    }
	else
		cerr << "Beam not found !!!" << endl;

	return found;
}

bool OpticsBench::localOptimum()
{
	OpticsFunction function(m_optics, m_wavelength);
	function.setOverlapBeam(m_targetBeam);
	function.setCheckLock(true);
	vector<double> positions = function.localMaximum(function.currentPosition());

	if (!function.optimizationSuccess())
		return false;

	for (unsigned int i = 0; i < positions.size(); i++)
		m_optics[i]->setPositionCheckLock(positions[i]);
	sort(m_optics.begin() + 1, m_optics.end(), less<Optics*>());
	computeBeams();

	return true;
}
