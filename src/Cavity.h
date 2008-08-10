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

#ifndef CAVITY_H
#define CAVITY_H

#include "GaussianBeam.h"
#include "Optics.h"

class OpticsBench;

class Cavity
{
public:
	Cavity(OpticsBench& bench);

public:
	void computeBeam();
	bool isStable() const;
	const Beam eigenBeam(int index) const;
	void addOptics(const Optics* optics);
	void removeOptics(const Optics* optics);

private:
	OpticsBench& m_bench;
	GenericABCD m_matrix;
	bool m_ringCavity;
	std::list<const ABCD*> m_opticsList;
};

#endif
