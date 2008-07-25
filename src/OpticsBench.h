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
#include "GaussianFit.h"

#include <vector>
#include <list>
#include <map>

class OpticsBench;

class OpticsBenchNotify
{
public:
	OpticsBenchNotify(OpticsBench& opticsBench);
	virtual void OpticsBenchWavelengthChanged() {};
	virtual void OpticsBenchOpticsAdded(int /*index*/) {};
	virtual void OpticsBenchOpticsRemoved(int /*index*/, int /*count*/) {};
	virtual void OpticsBenchDataChanged(int /*startOptics*/, int /*endOptics*/) {};
	virtual void OpticsBenchTargetBeamChanged() {};
	virtual void OpticsBenchBoundariesChanged() {};
	virtual void OpticsBenchFitAdded(int /*index*/) {};
	virtual void OpticsBenchFitsRemoved(int /*index*/, int /*count*/) {};
	virtual void OpticsBenchFitDataChanged(int /*index*/) {};

protected:
	OpticsBench& m_bench;
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
	void setLeftBoundary(double leftBoundary);
	double rightBoundary() { return m_rightBoundary; }
	void setRightBoundary(double rightBoundary);

	/// Handle optics
	int nOptics() const { return m_optics.size(); }
	int opticsIndex(const Optics* optics) const;
	const Optics* optics(int index) const { return m_optics[index]; }
	void addOptics(Optics* optics, int index);
	void addOptics(OpticsType opticsType, int index);
	void removeOptics(int index, int count = 1);
	/**
	* Set the optics at @p index to position @p position. Takes care of locks,
	* exclusion areas, and optics ordering.
	* @return the new index of the optics
	*/
	int setOpticsPosition(int index, double position, bool respectAbsoluteLock = true);
	void setOpticsName(int index, std::string name);
	void lockTo(int index, std::string opticsName);
	void lockTo(int index, int id);
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
//	const Beam* beamPtr(int index) const { return &(m_beams[index]); }
	void setInputBeam(const Beam& beam);
	void setBeam(const Beam& beam, int index);
	double sensitivity(int index) const { return m_sensitivity[index]; }

	/// Waist fit
	int nFit() const;
	Fit& addFit(unsigned int index, int nData = 0);
	Fit& fit(unsigned int index);
	void removeFit(unsigned int index);
	void removeFits(unsigned int startIndex, int n);
	/**
	* If you modified a Fit class obtained by the fit function, call this
	* function to notify this modification
	*/
	void notifyFitChange(unsigned int index);

	/// Magic waist
	const TargetBeam& targetBeam() const { return m_targetBeam; }
	void setTargetBeam(const TargetBeam& beam);
	bool magicWaist();

	/// Cavity stuff : @todo make a new class ?
	bool isCavityStable() const;
	const Beam cavityEigenBeam(int index) const;

	/// Optics change callback
	void registerNotify(OpticsBenchNotify* notify);

	void printTree();

private:
	std::vector<Optics*> cloneOptics() const;
	void lockTo(std::vector<Optics*>& opticsVector, int index, std::string opticsName) const;
	void lockTo(std::vector<Optics*>& opticsVector, int index, int id) const;
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

	/// Exclusion area
	double m_leftBoundary, m_rightBoundary;

	/// Waist fit
	std::vector<Fit> m_fits;

	/// Magic waist
	TargetBeam m_targetBeam;

	/// Cavity
	GenericABCD m_cavity;
	int m_firstCavityIndex;
	int m_lastCavityIndex;
	bool m_ringCavity;

	/// Optics naming
	std::map<OpticsType, int> m_lastOpticsName;
	std::map<OpticsType, std::string> m_opticsPrefix;

	/// Callback
	std::list<OpticsBenchNotify*> m_notifyList;
};

#endif
