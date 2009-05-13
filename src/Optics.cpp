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

#include "Optics.h"
#include "Utils.h"

#include <iostream>
#include <cmath>

using namespace std;

int Optics::m_lastId = 0;

Optics::Optics(OpticsType type, double position, string name)
	: m_id(++m_lastId)
	, m_type(type)
	, m_position(position)
	, m_width(0.)
	, m_ABCD(false)
	, m_rotable(false)
	, m_orientable(false)
	, m_orientation(Spherical)
	, m_angle(0.)
	, m_name(name)
	, m_absoluteLock(false)
	, m_relativeLockParent(0)
{
}

Optics::~Optics()
{
	// Detach all children
	for (list<Optics*>::iterator it = m_relativeLockChildren.begin(); it != m_relativeLockChildren.end(); it++)
		(*it)->m_relativeLockParent = 0;

	relativeUnlock();
}

/////////////////////////////////////////////////
// Locking functions

void Optics::setAbsoluteLock(bool absoluteLock)
{
	if (absoluteLock)
		relativeUnlock();

	m_absoluteLock = absoluteLock;
}

bool Optics::relativeLockedTo(const Optics* const optics) const
{
	return relativeLockRoot()->isRelativeLockDescendant(optics);
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

const Optics* Optics::relativeLockRoot() const
{
	if (!m_relativeLockParent)
		return this;
	else
		return m_relativeLockParent->relativeLockRoot();
}

bool Optics::isRelativeLockDescendant(const Optics* const optics) const
{
	if (optics == this)
		return true;

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

void Optics::setPositionCheckLock(double pos, bool respectAbsoluteLock)
{
	if (relativeLockTreeAbsoluteLock() && respectAbsoluteLock)
		return;

	relativeLockRoot()->moveDescendant(pos - position());
}

void Optics::eraseLockingTree()
{
	m_relativeLockChildren.clear();
	m_relativeLockParent = 0;
}

/////////////////////////////////////////////////
// CreateBeam class

CreateBeam::CreateBeam(double waist, double waistPosition, double index, string name)
	: Optics(CreateBeamType, 0., name)
{
	m_beam.setWaist(waist);
	m_beam.setWaistPosition(waistPosition);
	m_beam.setIndex(index);

	setRotable();
	setOrientable();
}

const Beam* CreateBeam::beam() const
{
	return &m_beam;
}


void CreateBeam::setBeam(const Beam& beam)
{
	m_beam = beam;
	if (!m_beam.isSpherical())
		setOrientation(Ellipsoidal);
}

Beam CreateBeam::image(const Beam& inputBeam, const Beam& /*opticalAxis*/) const
{
	Beam outputBeam = m_beam;
	outputBeam.setWavelength(inputBeam.wavelength());
	outputBeam.rotate(0, angle());
	if (orientation() == Spherical)
		outputBeam.makeSpherical();

	return outputBeam;
}

Beam CreateBeam::antecedent(const Beam& outputBeam, const Beam& opticalAxis) const
{
	return image(outputBeam, opticalAxis);
}

/////////////////////////////////////////////////
// Lens class

void Lens::setFocal(double focal)
{
	if (focal != 0.)
		m_focal = focal;
}

/////////////////////////////////////////////////
// FlatMirror class

Beam FlatMirror::image(const Beam& inputBeam, const Beam& opticalAxis) const
{
	double relativeAngle = angle() + opticalAxis.angle() - inputBeam.angle();

	if ((relativeAngle > M_PI/2.) && (relativeAngle < 3.*M_PI/2.))
		return inputBeam;

	Beam result = ABCD::image(inputBeam, opticalAxis);

	double rotationAngle = fmod(2.*relativeAngle + M_PI, 2.*M_PI);
	result.rotate(position(), rotationAngle);
	return result;
}

Beam FlatMirror::antecedent(const Beam& outputBeam, const Beam& opticalAxis) const
{
	Beam result = ABCD::antecedent(outputBeam, opticalAxis);

	/// @bug rotate this beam

	return result;
}

/////////////////////////////////////////////////
// CurvedMirror class

void CurvedMirror::setCurvatureRadius(double curvatureRadius)
{
	if (curvatureRadius != 0.)
		m_curvatureRadius = curvatureRadius;
}

/////////////////////////////////////////////////
// Dielectric class

void Dielectric::setIndexRatio(double indexRatio)
{
	if (indexRatio > 0.)
		m_indexRatio = indexRatio;
}

/////////////////////////////////////////////////
// CurvedInterface class

void CurvedInterface::setSurfaceRadius(double surfaceRadius)
{
	if (surfaceRadius != 0.)
		m_surfaceRadius = surfaceRadius;
}

/////////////////////////////////////////////////
// ABCD class

void ABCD::forward(const Beam& inputBeam, Beam& outputBeam, Orientation orientation) const
{
	if (isAligned(orientation))
	{
		complex<double> q = inputBeam.q(position(), orientation);
		q = (A()*q + B()) / (C()*q + D());
		outputBeam.setQ(q, position() + width(), orientation);
	}
}

Beam ABCD::image(const Beam& inputBeam, const Beam& /*opticalAxis*/) const
{
	Beam outputBeam = inputBeam;
	outputBeam.setIndex(inputBeam.index()*indexJump());

	if ((orientation() == Spherical) && (inputBeam.isSpherical()))
		forward(inputBeam, outputBeam, Spherical);
	else
	{
		forward(inputBeam, outputBeam, Horizontal);
		forward(inputBeam, outputBeam, Vertical);
	}

	return outputBeam;
}

void ABCD::backward(const Beam& outputBeam, Beam& inputBeam, Orientation orientation) const
{
	if (isAligned(orientation))
	{
		complex<double> q = outputBeam.q(position() + width(), orientation);
		q = (B() - D()*q) / (C()*q - A());
		inputBeam.setQ(q, position(), orientation);
	}
}

Beam ABCD::antecedent(const Beam& outputBeam, const Beam& /*opticalAxis*/) const
{
	Beam inputBeam = outputBeam;
	inputBeam.setIndex(outputBeam.index()/indexJump());

	if ((orientation() == Spherical) && (outputBeam.isSpherical()))
		backward(outputBeam, inputBeam, Spherical);
	else
	{
		backward(outputBeam, inputBeam, Horizontal);
		backward(outputBeam, inputBeam, Vertical);
	}

	return inputBeam;
}

bool ABCD::stabilityCriterion1() const
{
	return fabs((A() + B())/2.) < 1.;
}

bool ABCD::stabilityCriterion2() const
{
	return sqr(D() - A()) + 4.*C()*B() < 0.;
}

Beam ABCD::eigenMode(double wavelength) const
{
	/// @todo what is the index ?
	return Beam(complex<double>(-(D() - A())/(2.*C()), -sqrt(-(sqr(D() - A()) + 4.*C()*B()))/(2.*C())), position(), wavelength, 1.0, 1.0);
}

/////////////////////////////////////////////////
// GenericABCD class

GenericABCD& GenericABCD::operator*=(const ABCD& abcd)
{
	setA(A()*abcd.A() + B()*abcd.C());
	setB(A()*abcd.B() + B()*abcd.D());
	setC(C()*abcd.A() + D()*abcd.C());
	setD(C()*abcd.B() + D()*abcd.D());
	/// @todo check if the two objects are adjacent ?
	setWidth(width() + abcd.width());

	return *this;
}

GenericABCD operator*(const ABCD& abcd1, const ABCD& abcd2)
{
	GenericABCD r(abcd1);
	r *= abcd2;
	return r;
}

ostream& operator<<(ostream& out, const ABCD& abcd)
{
	out << "A = " << abcd.A() << " B = " << abcd.B() << " C = " << abcd.C() << " D = " << abcd.D();
	return out;
}
