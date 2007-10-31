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

#ifndef GAUSSIANBEAM_H
#define GAUSSIANBEAM_H

#include <vector>
#include <list>
#include <string>
#include <complex>

inline double sqr(double x)
{
	return x*x;
}

inline double min(double x, double y)
{
	return y < x ? y : x;
}

inline double max(double x, double y)
{
	return y > x ? y : x;
}

inline double sign(double x)
{
	return x < 0. ? -1. : 1.;
}

template<typename T> void swap(T& x, T& y)
{
	T tmp = x;
	x = y;
	y = tmp;
}

struct Approximation
{
	double minZ, maxZ;
	double resolution;

//	int currentInterval;
//	double currentValue;
};

class Beam
{
public:
	Beam();
	Beam(double waist, double waistPosition, double wavelength);
	Beam(const std::complex<double>& q, double z, double wavelength);

public:
	double waistPosition() const { return m_waistPosition; }
	void setWaistPosition(double waistPosition) { m_waistPosition = waistPosition; }
	double waist() const { return m_waist; }
	void setWaist(double waist) { m_waist = waist; }
	double divergence() const;
	void setDivergence(double divergence);
	double rayleigh() const;
	void setRayleigh(double rayleigh);
	double wavelength() const { return m_wavelength; }
	void setWavelength(double wavelength) { m_wavelength = wavelength; }

	bool isValid() const { return m_valid; }
	double radius(double z) const;
	double radiusDerivative(double z) const;
	double radiusSecondDerivative(double z) const;
	double curvature(double z) const;
	double gouyPhase(double z) const;
	std::complex<double> q(double z) const;
	inline double zred(double z) const { return (z - waistPosition())/rayleigh(); }

	double approxNextPosition(double currentPosition, Approximation& approximation) const;

private:
	double m_waist;
	double m_waistPosition;
	double m_wavelength;
	bool m_valid;
};

enum OpticsType {CreateBeamType, FreeSpaceType,
                 LensType, FlatMirrorType, CurvedMirrorType,
                 FlatInterfaceType, CurvedInterfaceType, GenericABCDType};

class Optics
{
public:
	Optics(OpticsType type, bool ABCD, double position, std::string name);
	virtual ~Optics() {}
	virtual Optics* clone() const = 0;

public:
	virtual Beam image(const Beam& inputBeam) const = 0;
	virtual Beam antecedent(const Beam& outputBeam) const = 0;

public:
	OpticsType type() const { return m_type; }
	double position() const { return m_position; }
	void setPosition(double position) { m_position = position; }
	double endPosition() const { return position() + width(); }
	double width() const { return m_width; }
	void setWidth(double width) { m_width = width; }
	std::string name() const { return m_name; }
	void setName(std::string name) { m_name = name; }
	bool isABCD() const { return m_ABCD; }
	/**
	* Query absolute lock
	* @return true if the lock is absolute, false otherwise
	*/
	bool absoluteLock() const { return m_absoluteLock; }
	/**
	* Set absolute lock state. Setting an absolute lock removes any relative lock
	* @param absoluteLock if true, remove relative lock and set absolute lock. If false, removes absolute lock if present.
	*/
	void setAbsoluteLock(bool absoluteLock);
	/**
	* @return the relative lock parent. 0 if there is no parent
	*/
	const Optics* relativeLockParent() const { return m_relativeLockParent; }
	/**
	* @return a list of relative lock children
	*/
	const std::list<Optics*>& relativeLockChildren() const { return m_relativeLockChildren; }
	/**
	* Query relative lock
	* @return true if the optics is within the locking tree of @p optics
	*/
	bool relativeLockedTo(const Optics* const optics) const;
	/**
	* @return true if the locking tree is absolutely locked, i.e. if the root of the locking tree is absolutely locked
	*/
	bool relativeLockedTreeAbsoluteLock() const { return relativeLockRoot()->absoluteLock(); }
	/**
	* Lock the optics to the given optics. Only works if @p optics is not within the locking tree of this optics.
	* If the locking succeeds, the absolute lock is set to false.
	* @return true if success, false otherwise
	*/
	bool relativeLockTo(Optics* optics);
	/**
	* Detach the optics from the relative lock tree.
	* @return true if success, false otherwise
	*/
	bool relativeUnlock();
	/**
	* Move the optics to @p position while taking care of locks :
	* - move the whole lock tree
	* - if the lock tree is abolutly locked, don't do anything
	*/
	void setPositionCheckLock(double position);

private:
	const Optics* relativeLockRoot() const;
	bool isRelativeLockDescendant(const Optics* const optics) const;

protected:
	void setType(OpticsType type) { m_type = type; }

private:
	OpticsType m_type;
	bool m_ABCD;
	double m_position;
	double m_width;
	std::string m_name;
	bool m_absoluteLock;
	Optics* m_relativeLockParent;
	std::list<Optics*> m_relativeLockChildren;
};

class CreateBeam : public Optics
{
public:
	CreateBeam(double waist, double waistPosition, std::string name = "");
	CreateBeam* clone() const { return new CreateBeam(*this); }

public:
	Beam image(const Beam& inputBeam) const;
	Beam antecedent(const Beam& outputBeam) const;

public:
	double waist() const { return m_waist; }
	void setWaist(double waist) { m_waist = waist; }

private:
	double m_waist;
};

class ABCD : public Optics
{
public:
	ABCD(OpticsType type, double position, std::string name = "");
	virtual ~ABCD() {}

public:
	Beam image(const Beam& inputBeam) const;
	Beam antecedent(const Beam& outputBeam) const;

public:
	virtual double A() const { return 1.; }
	virtual double B() const { return 0.; }
	virtual double C() const { return 0.; }
	virtual double D() const { return 1.; }

/// Cavity functions
public:
	bool stabilityCriterion1() const;
	bool stabilityCriterion2() const;
	Beam eigenMode(double wavelength) const;
};

class FreeSpace : public ABCD
{
public:
	FreeSpace(double width, double position, std::string name = "")
		: ABCD(FreeSpaceType, position, name) { setWidth(width); }
	FreeSpace* clone() const { return new FreeSpace(*this); }

public:
	double B() const { return width(); }
};

class Lens : public ABCD
{
public:
	Lens(double focal, double position, std::string name = "")
		: ABCD(LensType, position, name) , m_focal(focal) {}
	Lens* clone() const { return new Lens(*this); }

public:
	double C() const { return -1./focal(); }

public:
	double focal() const { return m_focal; }
	void setFocal(double focal) { m_focal = focal; }

private:
	double m_focal;
};

class FlatMirror : public ABCD
{
public:
	FlatMirror(double position, std::string name = "")
		: ABCD(FlatMirrorType, position, name) {}
	FlatMirror* clone() const { return new FlatMirror(*this); }
};

class CurvedMirror : public ABCD
{
public:
	CurvedMirror(double curvatureRadius, double position, std::string name = "")
		: ABCD(CurvedMirrorType, position, name), m_curvatureRadius(curvatureRadius) {}
	CurvedMirror* clone() const { return new CurvedMirror(*this); }

public:
 	double C() const { return -2./curvatureRadius(); }

public:
	double curvatureRadius() const { return m_curvatureRadius; }
	void setCurvatureRadius(double curvatureRadius) { m_curvatureRadius = curvatureRadius; }

protected:
	double m_curvatureRadius;
};

/**
* Virtual class
*/
class Interface : public ABCD
{
public:
	/**
	* Constructor
	* @param indexRation ratio between the final index and the initial index
	* @param position position of the interface
	* @param name user name for the optics
	*/
	Interface(OpticsType type, double indexRatio, double position, std::string name = "")
		: ABCD(type, position, name), m_indexRatio(indexRatio) {}
	virtual ~Interface() {}

public:
	virtual double D() const { return 1./indexRatio(); }

public:
	double indexRatio() const { return m_indexRatio; }
	void setIndexRatio(double indexRatio) { m_indexRatio = indexRatio; }

protected:
	double m_indexRatio;
};

class FlatInterface : public Interface
{
public:
	/**
	* Constructor
	* @param indexRation ratio between the final index and the initial index
	* @param position position of the interface
	* @param name user name for the optics
	*/
	FlatInterface(double indexRatio, double position, std::string name = "")
		: Interface(FlatInterfaceType, indexRatio, position, name) {}
	FlatInterface* clone() const { return new FlatInterface(*this); }
};

class CurvedInterface : public Interface
{
public:
	/**
	* Constructor
	* @param surfaceRadius curvature radius of the surface
	* @param indexRation ratio between the final index and the initial index
	* @param position position of the interface
	* @param name user name for the optics
	*/
	CurvedInterface(double surfaceRadius, double indexRatio, double position, std::string name = "")
		: Interface(CurvedInterfaceType, indexRatio, position, name), m_surfaceRadius(surfaceRadius) {}
	CurvedInterface* clone() const { return new CurvedInterface(*this); }

public:
	double C() const { return (1./indexRatio()-1.)/surfaceRadius(); }

public:
	double surfaceRadius() const { return m_surfaceRadius; }
	void setSurfaceRadius(double surfaceRadius) { m_surfaceRadius = surfaceRadius; }

protected:
	double m_surfaceRadius;
};

class GenericABCD : public ABCD
{
public:
	GenericABCD(const ABCD& abcd)
		: ABCD(abcd)
		, m_A(abcd.A()), m_B(abcd.B()), m_C(abcd.C()), m_D(abcd.D())
		{ setType(GenericABCDType); }
	GenericABCD(double A, double B, double C, double D, double width, double position, std::string name = "")
		: ABCD(GenericABCDType, position, name)
		, m_A(A), m_B(B), m_C(C), m_D(D) { setWidth(width); }
	GenericABCD* clone() const { return new GenericABCD(*this); }

public:
	double A() const { return m_A; }
	double B() const { return m_B; }
	double C() const { return m_C; }
	double D() const { return m_D; }

public:
	void setA(double A) { m_A = A; }
	void setB(double B) { m_B = B; }
	void setC(double C) { m_C = C; }
	void setD(double D) { m_D = D; }

protected:
	double m_A, m_B, m_C, m_D;
};

GenericABCD operator*(const ABCD& abcd1, const ABCD& abcd2);
GenericABCD operator*=(const ABCD& abcd1, const ABCD& abcd2);

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
