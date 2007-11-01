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

#include "Optics.h"

#include <iostream>
#include <cmath>

using namespace std;

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
