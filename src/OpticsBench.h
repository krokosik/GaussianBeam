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

#ifndef OPTICSBENCH_H
#define OPTICSBENCH_H

#include "Optics.h"
#include "GaussianBeam.h"
#include "GaussianFit.h"
#include "Cavity.h"
#include "Utils.h"

#include <vector>
#include <list>
#include <map>

/*
* OpticsTreeItem
class OpticsTreeItem
{
public:
	OpticsTreeItem(Optics* optics, OpticsTreeItem* parent = 0);

public:
	void move(OpticsTreeItem* parent);
	void remove();
	Beam* axis();
	Optics* optics() { return m_optics; }
	Beam* image() { return m_image; }
	void sort();

private:
	void insert(OpticsTreeItem* parent);

private:
	OpticsTreeItem* m_parent;
	Optics* m_optics;
	Beam* m_image;
	OpticsTreeItem* m_child;
};
*/

class OpticsBench;

/**
* @class OpticsBenchEvents
* Inherit from this class, reimplement virtual functions and register to the bench
* to receive events from the optics bench
*/
class OpticsBenchEventListener
{
public:
	virtual void onOpticsBenchWavelengthChanged() {}
	virtual void onOpticsBenchOpticsAdded(int /*index*/) {}
	virtual void onOpticsBenchOpticsRemoved(int /*index*/, int /*count*/) {}
	virtual void onOpticsBenchDataChanged(int /*startOptics*/, int /*endOptics*/) {}
	virtual void onOpticsBenchTargetBeamChanged() {}
	virtual void onOpticsBenchBoundariesChanged() {}
	virtual void onOpticsBenchFitAdded(int /*index*/) {}
	virtual void onOpticsBenchFitsRemoved(int /*index*/, int /*count*/) {}
	virtual void onOpticsBenchFitDataChanged(int /*index*/) {}
	virtual void onOpticsBenchSphericityChanged() {}

protected:
	OpticsBench* m_bench;
};

/**
* OpticsBench
*/
class OpticsBench
{
public:
	OpticsBench();
	~OpticsBench();

public:
	void registerEventListener(OpticsBenchEventListener* listener);

	// Properties

	/// @return the bench wavelength
	double wavelength() const { return m_wavelength; }
	/// Set the bench wavelength to @p wavelength
	void setWavelength(double wavelength);
	/// @return true if the bench contains only spherical optics, i.e. if horizontal beams are identical to vertical beams
	bool isSpherical() const;

	// Boundaries

	Utils::Rect boundary() const { return m_boundary; }

	/// @todo remove all these functions

	/// @return the bench left boundary
	double leftBoundary() const { return m_boundary.x1(); }
	/// Set the bench left boundary to @p leftBoundary
	void setLeftBoundary(double leftBoundary);
	/// @return the bench right boundary
	double rightBoundary() const { return m_boundary.x2(); }
	/// Set the bench right boundary to @p leftBoundary
	void setRightBoundary(double rightBoundary);

	// Optics

	/// @return the number of optics in the bench
	int nOptics() const { return m_optics.size(); }
	/**
	* @return the sorted index of optics @p optics in the optics bench
	* This index may change when an optics position is changed
	*/
	int opticsIndex(const Optics* optics) const;
	/// @return a pointer to optics situated at index @p index
	const Optics* optics(int index) const { return m_optics[index]; }
	/// Add the optics @p optics at index @p index
	void addOptics(Optics* optics, int index);
	/// Add an optics of type @p opticsType at index @p index
	void addOptics(OpticsType opticsType, int index);
	/// Remove the optics situated at index @p index
	void removeOptics(int index, int count = 1);
	/**
	* Set the optics at @p index to position @p position. Takes care of locks,
	* exclusion areas, and optics ordering.
	* @return the new index of the optics
	*/
	int setOpticsPosition(int index, double position, bool respectAbsoluteLock = true);
	/// set the name of optics at @p index to @p name
	void setOpticsName(int index, std::string name);
	/// lock the optics at index @p index to the first optics called @p opticsName
	void lockTo(int index, std::string opticsName);
	/// lock the optics at index @p index to the optics which id is @p id
	void lockTo(int index, int id);
	/**
	* Retrieve a non constant pointer to the optics @p index for property change.
	* When finished, call opticsPropertyChanged() to propagate your changes
	* If opticsBench proposes a direct function for changing a property
	* (position, name, lockTo), rather use these functions.
	*/
	Optics* opticsForPropertyChange(int index) { return m_optics[index]; }
	/// Call this function after changing the properties of an optics returned by opticsForPropertyChange( @p index )
	void opticsPropertyChanged(int index);

	/// Beams handling
	const Beam* beam(int index) const;
	void setBeam(const Beam& beam, int index);
	const Beam* inputBeam() const { return m_beams[0]; }
	void setInputBeam(const Beam& beam);
	const Beam* axis(int index) const;
	std::pair<Beam*, double> closestPosition(const Utils::Point& point, int preferedSide = 1) const;
	double sensitivity(int index) const;

	/// Cavity
	Cavity& cavity() { return m_cavity; }

	/// Waist fit
	int nFit() const;
	Fit* addFit(unsigned int index, int nData = 0);
	Fit* fit(unsigned int index);
	void removeFit(unsigned int index);
	void removeFits(unsigned int startIndex, int n);

	/// Magic waist
	const Beam* targetBeam() const { return &m_targetBeam; }
	void setTargetBeam(const Beam& beam);
	double targetOverlap() const { return m_targetOverlap; }
	void setTargetOverlap(double targetOverlap);
	Orientation targetOrientation() const { return m_targetOrientation; }
	void setTargetOrientation(Orientation orientation);
	bool magicWaist();
	bool localOptimum();

	/// Debugging
	void printTree();

/// These functions are called by objects of the bench to notify that they have been changed
public:
	void notifyFitChanged(Fit* fit);

private:
	/// @todo on demand computing of beam, cavity and sensitity
	void computeBeams(int changedIndex = 0, bool backwards = false);
	void updateExtremeBeams();
	void detectCavities();
	void checkFitSpherical();

private:
	double m_wavelength;
	std::vector<Optics*> m_optics;
	std::vector<Beam*> m_beams;
	std::vector<double> m_sensitivity;
	bool m_beamSpherical, m_fitSpherical;
	std::list<OpticsBenchEventListener*> m_listeners;

//	std::vector<OpticsTreeItem> m_opticsTree;

	/// Exclusion area
	Utils::Rect m_boundary;

	/// Waist fit
	std::vector<Fit*> m_fits;

	/// Magic waist
	Beam m_targetBeam;
	double m_targetOverlap;
	Orientation m_targetOrientation;

	/// Cavity
	Cavity m_cavity;

	/// Optics naming
	std::map<OpticsType, int> m_lastOpticsName;
	std::map<OpticsType, std::string> m_opticsPrefix;

	/// @todo it might be possible to remove this
	friend class OpticsFunction;
};

#endif
