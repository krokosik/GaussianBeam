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

class OpticsBench;

class OpticsBenchNotify
{
public:
	OpticsBenchNotify(OpticsBench& opticsBench);
	virtual void OpticsBenchDataChanged(int startOptics, int endOptics) = 0;
	virtual void OpticsBenchOpticsAdded(int index) = 0;
	virtual void OpticsBenchOpticsRemoved(int index, int count) = 0;

protected:
	OpticsBench& m_bench;
};

struct Tolerance
{
	/// True for tolerance on overlap, false for tolerance on waist and position
	bool overlap;
	double minOverlap;
	double waistTolerance;
	double positionTolerance;
};

class Fit
{
public:
	Fit();

public:
	std::string name() const { return m_name; }
	void setName(std::string name) { m_name = name; }
	void setData(int index, double position, double value);
	void clear();
	const Beam& beam(double wavelength) const;
	double rho2(double wavelength) const;

private:
	void fitBeam(double wavelength) const;

private:
	std::string m_name;
	std::vector<double> m_positions;
	std::vector<double> m_values;
	mutable bool m_dirty;
	mutable Beam m_beam;
	mutable double m_rho2;
	mutable double m_lastWavelength;
};

class OpticsBench
{
public:
	OpticsBench();
	~OpticsBench();

public:
	/// Parameters
	double wavelength() const { return m_wavelength; }
	void setWavelength(double wavelength);
	double leftBoundary() { return m_leftBoundary; }
	void setLeftBoundary(double leftBoundary) { m_leftBoundary = leftBoundary; }
	double rightBoundary() { return m_rightBoundary; }
	void setRightBoundary(double rightBoundary) { m_rightBoundary = rightBoundary; }

	/// Handle optics
	int nOptics() const { return m_optics.size(); }
	int opticsIndex(const Optics* optics) const;
	const Optics* optics(int index) const { return m_optics[index]; }
	void addOptics(Optics* optics, int index);
	void removeOptics(int index, int count = 1);
	/**
	* Set the optics at @p index to position @p position. Takes care of locks,
	* exclusion areas, and optics ordering.
	* @return the new index of the optics
	*/
	int setOpticsPosition(int index, double position, bool respectAbsoluteLock = true);
	void setOpticsName(int index, std::string name);
	void lockTo(int index, std::string opticsName);
	/**
	* Retrieve a non constant pointer to an optics for property change.
	* When finished, call opticsPropertyChanged()
	* If opticsBench proposes a direct function for changing a property
	* (position, name, lockTo), rather use these functions.
	*/
	Optics* opticsForPropertyChange(int index) { return m_optics[index]; }
	void opticsPropertyChanged(int index);

	/// Beams handling
	const Beam& beam(int index) const { return m_beams[index]; }
	const Beam* beamPtr(int index) const { return &(m_beams[index]); }
	void setInputBeam(const Beam& beam);
	void setBeam(const Beam& beam, int index);
	double sensitivity(int index) const { return m_sensitivity[index]; }

	/// Waist fit
	Fit& fit(int index);

	/// Magic waist
	const Beam& targetBeam() const { return m_targetBeam; }
	void setTargetBeam(const Beam& beam);
	bool magicWaist(const Tolerance& tolerance);

	/// Cavity stuff : @todo make a new class ?
	bool isCavityStable() const;
	const Beam cavityEigenBeam(int index) const;

	/// Optics change callback
	void registerNotify(OpticsBenchNotify* notify);

private:
	std::vector<Optics*> cloneOptics() const;
	void lockTo(std::vector<Optics*>& opticsVector, int index, std::string opticsName) const;
	void setOpticsPosition(std::vector<Optics*>& opticsVector, int index, double position, bool respectAbsoluteLock = true) const;
	/// @todo on demand computing of beam, cavity and sensitity
	void computeBeams(int changedIndex = 0, bool backward = false);
	Beam computeSingleBeam(const std::vector<Optics*>& opticsVector, int index) const;
	void emitChange(int startOptics, int endOptics) const;
	std::vector<double> gradient(const std::vector<Optics*>& opticsVector, const Beam& beam, bool checkLock, bool curvature) const;

private:
	double m_wavelength;
	std::vector<Optics*> m_optics;
	std::vector<Beam> m_beams;
	std::vector<double> m_sensitivity;
	double m_leftBoundary, m_rightBoundary;

	/// Waist fit
	std::vector<Fit> m_fits;

	/// Magic waist
	Beam m_targetBeam;

	/// Cavity
	GenericABCD m_cavity;
	int m_firstCavityIndex;
	int m_lastCavityIndex;
	bool m_ringCavity;

	/// Callback
	std::list<OpticsBenchNotify*> m_notifyList;
};

namespace GaussianBeam
{
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
