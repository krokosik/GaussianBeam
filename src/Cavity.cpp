/* This file is part of the GaussianBeam project
   Copyright (C) 2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>

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

#include "Cavity.h"
#include "OpticsBench.h"

#include <iostream>
#include <algorithm>

using namespace std;

Cavity::Cavity()
{
	m_ringCavity = true;
}

void Cavity::addOptics(const ABCD* optics)
{
	if (!isOpticsInCavity(optics))
		m_opticsList.push_back(optics);

	/// @todo sort
}

void Cavity::removeOptics(const ABCD* optics)
{
	m_opticsList.remove(optics);
}

bool Cavity::isOpticsInCavity(const ABCD* optics) const
{
	if (find(m_opticsList.begin(), m_opticsList.end(), optics) != m_opticsList.end())
		return true;

	return false;
}

void Cavity::computeMatrix() const
{
	if (m_opticsList.empty())
		return;

	// Compute cavity
	m_matrix = *m_opticsList.front();
	cerr << "Initial Cavity ABCD = " << m_matrix << endl;
	list<const ABCD*>::const_iterator lastIt = m_opticsList.begin();
	for (list<const ABCD*>::const_iterator it = lastIt; ++it != m_opticsList.end(); lastIt = it)
	{
		static FreeSpace freeSpace(0., 0.);
		freeSpace.setWidth((*it)->position() - (*(lastIt))->endPosition());
		freeSpace.setPosition((*it)->endPosition());
		cerr << "freespace B = " << freeSpace.B() << endl;
		m_matrix = m_matrix * freeSpace;
		cerr << "freespace Cavity ABCD = " << m_matrix << endl;
		m_matrix *= *(*it);
		cerr << "added Cavity ABCD = " << m_matrix << endl;
	}
	/// @todo matrix.setWidth()

	cerr << "Backwards" << endl;

	if (m_ringCavity)
	{
		list<const ABCD*>::const_iterator lastIt = m_opticsList.end();
		m_matrix = *m_opticsList.back();
		for (list<const ABCD*>::const_iterator it = lastIt; --it != m_opticsList.begin(); lastIt = it)
		{
			static FreeSpace freeSpace(0., 0.);
			freeSpace.setWidth((*it)->endPosition() - (*(lastIt))->position());
			freeSpace.setPosition((*it)->endPosition());
			m_matrix *= freeSpace;
			m_matrix *= *(*it);
		}
		//FreeSpace freeSpace(optics(i+1)->position() - m_bench.optics(i)->endPosition(), m_bench.optics(i)->endPosition());
		//m_matrix *= freeSpace;
		cerr << "freespace Cavity ABCD = " << m_matrix << endl;
	}
	else
	{
		/// @todo add a user defined free space between the last and the first optics for linear cavities.
	}

/*	static FreeSpace freeSpace(0., 0.);
	freeSpace.setWidth(m_bench.optics(m_lastCavityIndex)->position() - m_bench.optics(m_firstCavityIndex)->endPosition());
	freeSpace.setPosition(m_bench.optics(m_firstCavityIndex)->endPosition());
	m_matrix *= freeSpace;
*/
//	cerr << "Final Cavity ABCD = " << m_matrix.A() << " " << m_matrix.B() << " " << m_matrix.C() << " " << m_matrix.D() << endl;
}

bool Cavity::isStable() const
{
	if (m_opticsList.empty())
		return false;

	if (m_matrix.stabilityCriterion1())
	{
		if (m_matrix.stabilityCriterion2())
			return true;
		else
			cerr << "Cavity stable for 1 and not 2 !!!!";
	}
	else if (m_matrix.stabilityCriterion2())
		cerr << "Cavity stable for 2 and not 1 !!!!";

	return false;
}

const Beam* Cavity::eigenBeam(double wavelength, int index) const
{
	/// @todo cache compute beam ?
	computeMatrix();
	m_beam = m_matrix.eigenMode(wavelength);

	/// @todo reimplement checks
/*	if (!isStable() ||
	   (index < m_firstCavityIndex) ||
	   (m_ringCavity && (index >= m_lastCavityIndex)) ||
	   (!m_ringCavity && (index > m_lastCavityIndex)))
		return Beam();
*/

//	for (int i = m_firstCavityIndex; i <= index; i++)
//		beam = m_bench.optics(i)->image(beam);

	return &m_beam;
}
