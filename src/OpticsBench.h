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

#ifndef OPTICSBENCH_H
#define OPTICSBENCH_H

#include "Optics.h"
#include "GaussianBeam.h"

#include <vector>
#include <list>

class OpticsBenchNotify
{
public:
	OpticsBenchNotify() {}
	virtual void OpticsBenchDataChanged(int startOptics, int endOptics) = 0;
};

class OpticsBench
{
public:
	OpticsBench();
	~OpticsBench();

public:
	int nOptics() const { return m_optics.size(); }
	const Optics* optics(int index) const { return m_optics[index]; }
//	Optics* optics(int index) { return m_optics[index]; }
	const Beam& beam(int row) const { return m_beams[row]; }
	double wavelength() const { return m_wavelength; }
	void setWavelength(double wavelength);

	void setInputBeam(const Beam& beam) { setInputBeam(beam, true); }

	void registerNotify(OpticsBenchNotify* notify);

private:
	void setInputBeam(const Beam& beam, bool update);

/// @todo make this private
public:
	std::vector<Optics*> m_optics;
	std::vector<Beam> m_beams;
	void computeBeams(int changedRow = 0, bool backward = false);

private:
	double m_wavelength;
	Beam m_targetBeam;
	std::list<OpticsBenchNotify*> m_notifyList;

/// Cavity stuff : @todo make a new class ?
public:
	bool isCavityStable() const;
	const Beam cavityEigenBeam(int row) const;

private:
	GenericABCD m_cavity;
	int m_first_cavity_row;
	int m_last_cavity_row;
	bool m_ring_cavity;
};

namespace GaussianBeam
{
	struct MagicWaistTarget
	{
		Beam beam;
		/// True for tolerance on overlap, false for tolerance on waist and position
		bool overlap;
		double minOverlap;
		double waistTolerance;
		double positionTolerance;
		bool scramble;
	};

	/**
	* Find a lens arrangement to produce @p targetBeam from @p inputBeam
	* @p optics vector of optics to use
	* @p target data structure gathering target properties
	* @return true for success
	*/
	bool magicWaist(std::vector<Optics*>& optics, const MagicWaistTarget& target);

	/**
	* Find the waist radius and position for a given set of radii measurement of a Gaussian beam
	* It fits the given data with a linear fit, and finds the only hyperbola
	* that is tangent to the resulting line
	* @p positions vector of positions
	* @p radii vector of radii @ 1/e^2
	* @p wavelength wavelength
	* @p rho2 a pointer to a double set to the squared correlation coefficient
	*/
	Beam fitBeam(std::vector<double> positions, std::vector<double> radii, double wavelength, double* rho2 = 0);

	/**
	* Compute the intensity overlap between beams @p beam1 and @p beam2 at position @p z
	* This overlap does not depend on @p z if both beams have the same wavelength,
	* hence the default value for z
	*/
	double overlap(const Beam& beam1, const Beam& beam2, double z = 0.);
}

#endif
