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

#ifndef OPTICS_H
#define OPTICS_H

#include "GaussianBeam.h"

#include <list>
#include <string>

enum OpticsType {CreateBeamType, FreeSpaceType,
                 LensType, FlatMirrorType, CurvedMirrorType,
                 FlatInterfaceType, CurvedInterfaceType,
                 DielectricSlabType, ThickLensType, ThermalLensType, GenericABCDType, UserType};

class Optics
{
public:
	/**
	* Constructor
	* @p type enum type of the optics
	* @p ABCD tell ifthe optics is of ABCD type
	* @p position stating position of the optics
	* @p name name of the optics
	*/
	Optics(OpticsType type, bool ABCD, double position, std::string name);
	/**
	* Destructor
	*/
	virtual ~Optics();
	/**
	* Duplicate the optics into a new independant class
	*/
	virtual Optics* clone() const = 0;

public:
	/**
	* Compute the image of a given input beam
	* @p inputBeam input beam
	*/
	virtual Beam image(const Beam& inputBeam) const = 0;
	/**
	* Compute the input beam corresponding to a given output beam
	* @p outputBeam output beam
	*/
	virtual Beam antecedent(const Beam& outputBeam) const = 0;
	/** Index jump from one side of the optics to the other
	* @return final index / initial index
	*/
	virtual double indexJump() const { return 1.; }

public:
	/// @return the enum type of the optics
	OpticsType type() const { return m_type; }
	/// @return the position of the left boundary of the optics
	double position() const { return m_position; }
	/// Set the left position of the optics to @p position
	void setPosition(double position) { m_position = position; }
	/// @return the right position of the optics
	double endPosition() const { return position() + width(); }
	/// @return the width of the optics
	double width() const { return m_width; }
	/// Set the width of the optics
	void setWidth(double width) { m_width = width; }
	/// @return the name of the optics
	std::string name() const { return m_name; }
	/// Set the name of the optics
	void setName(std::string name) { m_name = name; }
	/// @return true if the optics is of type ABCD
	bool isABCD() const { return m_ABCD; }
	/// @return the unique ID of the optics
	int id() const { return m_id; }
	/// Query absolute lock. @return true if the lock is absolute, false otherwise
	bool absoluteLock() const { return m_absoluteLock; }
	/**
	* Set absolute lock state. Setting an absolute lock removes any relative lock
	* @param absoluteLock if true, remove relative lock and set absolute lock. If false, removes absolute lock if present.
	*/
	void setAbsoluteLock(bool absoluteLock);
	/// @return the relative lock parent. 0 if there is no parent
	const Optics* relativeLockParent() const { return m_relativeLockParent; }
	/// @return a list of relative lock children
	const std::list<Optics*>& relativeLockChildren() const { return m_relativeLockChildren; }
	/// Query relative lock. @return true if @p optics within the locking tree of this optics
	bool relativeLockedTo(const Optics* const optics) const;
	/// @return true if the locking tree is absolutely locked, i.e. if the root of the locking tree is absolutely locked
	bool relativeLockTreeAbsoluteLock() const { return relativeLockRootConst()->absoluteLock(); }
	/**
	* Lock the optics to the given @p optics. Only works if @p optics is not within the locking tree of this optics.
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
	* - if the lock tree is abolutly locked, don't do anything,
	*   except if @p respectAbsoluteLock is set to false
	*/
	void setPositionCheckLock(double position, bool respectAbsoluteLock = true);
	/**
	* Erase the locking tree structure of this optics without affecting ascendant or descendant.
	* This is usefull for preserving or recreating the locking tree after cloning.
	*/
	void eraseLockingTree();

private:
	/// @todo this is not clean. Think...
	Optics* relativeLockRoot();
	const Optics* relativeLockRootConst() const;
	/// Check if @p optics is this optics or recursively one of this optic's descendants
	bool isRelativeLockDescendant(const Optics* const optics) const;
	/// translate this optics and recursively its descendant by @p distance
	void moveDescendant(double distance);

protected:
	void setType(OpticsType type) { m_type = type; }

private:
	int m_id;
	OpticsType m_type;
	bool m_ABCD;
	double m_position;
	double m_width;
	std::string m_name;
	bool m_absoluteLock;
	Optics* m_relativeLockParent;
	std::list<Optics*> m_relativeLockChildren;
	static int m_lastId;
};

/////////////////////////////////////////////////
// Pure virtual Optics classes

class ABCD : public Optics
{
public:
	ABCD(OpticsType type, double position, std::string name = "");
	virtual ~ABCD() {}

public:
	virtual Beam image(const Beam& inputBeam) const;
	virtual Beam antecedent(const Beam& outputBeam) const;

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

class GenericLens : public ABCD
{
public:
	GenericLens(OpticsType type, double focal, double position, std::string name = "")
		: ABCD(type, position, name) , m_focal(focal) {}
	virtual ~GenericLens() {}

public:
	virtual double C() const { return -1./focal(); }

public:
	double focal() const { return m_focal; }
	void setFocal(double focal);

private:
	double m_focal;
};

class Dielectric
{
public:
	Dielectric(double indexRatio) : m_indexRatio(indexRatio) {}

public:
	double indexRatio() const { return m_indexRatio; }
	void setIndexRatio(double indexRatio);

private:
	double m_indexRatio;
};

class Interface : public ABCD, public Dielectric
{
public:
	Interface(OpticsType type, double indexRatio, double position, std::string name = "")
		: ABCD(type, position, name), Dielectric(indexRatio) {}
	virtual ~Interface() {}

public:
	virtual double indexJump() const { return indexRatio(); }
	virtual double D() const { return 1./indexRatio(); }
};

/////////////////////////////////////////////////
// Non ABCD optics

class CreateBeam : public Optics
{
public:
	CreateBeam(double waist, double waistPosition, double index, std::string name = "");
	CreateBeam* clone() const { return new CreateBeam(*this); }

public:
	Beam image(const Beam& inputBeam) const;
	Beam antecedent(const Beam& outputBeam) const;

public:
	double waist() const { return m_waist; }
	void setWaist(double waist);
	double index() const { return m_index; }
	void setIndex(double index);
	void setBeam(const Beam& beam);

private:
	double m_waist;
	double m_index;
};

/////////////////////////////////////////////////
// ABCD optics

class FreeSpace : public ABCD
{
public:
	FreeSpace(double width, double position, std::string name = "")
		: ABCD(FreeSpaceType, position, name) { setWidth(width); }
	FreeSpace* clone() const { return new FreeSpace(*this); }

public:
	virtual double B() const { return width(); }
};

class Lens : public GenericLens
{
public:
	Lens(double focal, double position, std::string name = "")
		: GenericLens(LensType, focal, position, name) {}
	Lens* clone() const { return new Lens(*this); }
};

/// @todo finish this
class ThickLens : public GenericLens, public Dielectric
{
public:
	ThickLens(double focal, double indexRatio, double position, std::string name = "")
		: GenericLens(ThickLensType, focal, position, name), Dielectric(indexRatio) {}
	ThickLens* clone() const { return new ThickLens(*this); }

public:
	virtual double A() const { return -1./focal(); }
	virtual double B() const { return -1./focal(); }
	virtual double D() const { return -1./focal(); }
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
 	virtual double C() const { return -2./curvatureRadius(); }

public:
	double curvatureRadius() const { return m_curvatureRadius; }
	void setCurvatureRadius(double curvatureRadius);

protected:
	double m_curvatureRadius;
};

class FlatInterface : public Interface
{
public:
	FlatInterface(double indexRatio, double position, std::string name = "")
		: Interface(FlatInterfaceType, indexRatio, position, name) {}
	FlatInterface* clone() const { return new FlatInterface(*this); }
};

class CurvedInterface : public Interface
{
public:
	CurvedInterface(double surfaceRadius, double indexRatio, double position, std::string name = "")
		: Interface(CurvedInterfaceType, indexRatio, position, name), m_surfaceRadius(surfaceRadius) {}
	CurvedInterface* clone() const { return new CurvedInterface(*this); }

public:
	virtual double C() const { return (1./indexRatio()-1.)/surfaceRadius(); }

public:
	double surfaceRadius() const { return m_surfaceRadius; }
	void setSurfaceRadius(double surfaceRadius);

protected:
	double m_surfaceRadius;
};

class DielectricSlab : public ABCD, public Dielectric
{
public:
	DielectricSlab(double indexRatio, double width, double position, std::string name = "")
		: ABCD(DielectricSlabType, position, name), Dielectric(indexRatio) { setWidth(width); }
	DielectricSlab* clone() const { return new DielectricSlab(*this); }

public:
	virtual double B() const { return width()/indexRatio(); }
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
	GenericABCD& operator*=(const ABCD& abcd1);

public:
	virtual double A() const { return m_A; }
	virtual double B() const { return m_B; }
	virtual double C() const { return m_C; }
	virtual double D() const { return m_D; }

/// @todo check if the given ABCD matrix is valid
public:
	void setA(double A) { m_A = A; }
	void setB(double B) { m_B = B; }
	void setC(double C) { m_C = C; }
	void setD(double D) { m_D = D; }

protected:
	double m_A, m_B, m_C, m_D;
};

GenericABCD operator*(const ABCD& abcd1, const ABCD& abcd2);

namespace std
{
	/// Sort function for stl::sort : compare optics positions
	template<> struct less<Optics*>
	{
		bool operator()(Optics const* optics1, Optics const* optics2)
		{
			if (!optics1) return true;
			if (!optics2) return false;
			return optics1->position() < optics2->position();
		}
	};
}

#endif
