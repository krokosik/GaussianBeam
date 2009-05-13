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

#ifndef OPTICS_H
#define OPTICS_H

#include "GaussianBeam.h"

#include <list>
#include <string>

#include <QObject>

enum OpticsType {CreateBeamType, FreeSpaceType,
                 LensType, FlatMirrorType, CurvedMirrorType,
                 FlatInterfaceType, CurvedInterfaceType,
                 DielectricSlabType, ThickLensType, ThermalLensType, GenericABCDType, UserType};

/**
* Generic optics class. An optics is a transformation of a Gaussian beam
*/
class Optics
{
public:
	/**
	* Constructor
	* @p type enum type of the optics
	* @p position stating position of the optics
	* @p name name of the optics
	*/
	Optics(OpticsType type, double position, std::string name);
	/**
	* Destructor
	*/
	virtual ~Optics();
	/**
	* Duplicate the optics into a new independant class.
	* This function can clone an optics without knowing its actual type.
	*/
	virtual Optics* clone() const = 0;

public:
	/**
	* Compute the image of a given input beam
	* @p inputBeam input beam
	*/
	Beam image(const Beam& inputBeam) const { return image(inputBeam, inputBeam); }
	/**
	* Compute the image of a given input beam
	* using a different optical axis as angle reference
	* @p inputBeam input beam
	* @p opticalAxis input optical axis
	*/
	virtual Beam image(const Beam& inputBeam, const Beam& opticalAxis) const = 0;
	/**
	* Compute the input beam corresponding to a given output beam
	* @p outputBeam output beam
	*/
	Beam antecedent(const Beam& outputBeam) const { return antecedent(outputBeam, outputBeam); }
	/**
	* Compute the input beam corresponding to a given output beam
	* using a different optical axis as angle reference
	* @p outputBeam output beam
	* @p opticalAxis output optical axis
	*/
	virtual Beam antecedent(const Beam& outputBeam, const Beam& opticalAxis) const = 0;
	/**
	* Index jump from one side of the optics to the other
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
	/// @return the angle between the optics and the optical axis
	double angle() const { return m_angle; }
	/// Set the angle between the optics and the optical axis
	void setAngle(double angle) { if (isRotable()) m_angle = angle; }
	/// @return the orientaition of the optics, i.e. its anisotropy (e.g. cylindric aspect for lenses)
	Orientation orientation() const { return m_orientation; }
	/// Set the orientation of the optics
	void setOrientation(Orientation orientation) { if (isOrientable()) m_orientation = orientation; }
	/// @return the name of the optics
	std::string name() const { return m_name; }
	/// Set the name of the optics
	void setName(std::string name) { m_name = name; }
	/// @return true if the optics is rotable, i.e. if it can form an angle different from Pi/2 with the optical axis
	bool isRotable() const { return m_rotable; }
	/// @return true if the optics is orientable, i.e. if it can be anisotropic
	bool isOrientable() const { return m_orientable; }
	/// @return true if the optics is of type ABCD
	bool isABCD() const { return m_ABCD; }
	/// @return the unique ID of the optics
	int id() const { return m_id; }
	/// set the optics id. This function is reserved to loading functions: do not use
	void setId(int id) { m_id = id; }
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
	bool relativeLockTreeAbsoluteLock() const { return relativeLockRoot()->absoluteLock(); }
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

protected:
	void setType(OpticsType type) { m_type = type; }
	void setABCD(bool ABCD = true) { m_ABCD = ABCD; }
	void setRotable(bool rotable = true) { m_rotable = rotable; }
	void setOrientable(bool orientable = true) { m_orientable = orientable; }
	bool isAligned(Orientation orientation) const { return (m_orientation == Spherical) || (m_orientation == orientation); }

private:
	// Return the locking tree root, const and non-const versions
	Optics* relativeLockRoot();
	const Optics* relativeLockRoot() const;
	// Check if @p optics is this optics or recursively one of this optic's descendants
	bool isRelativeLockDescendant(const Optics* const optics) const;
	// Translate this optics and recursively its descendant by @p distance
	void moveDescendant(double distance);
	// Notify all registered classes
	void notify() const;

private:
	int m_id;
	OpticsType m_type;
	double m_position;
	double m_width;
	bool m_ABCD, m_rotable, m_orientable;
	Orientation m_orientation;
	double m_angle;
	std::string m_name;
	bool m_absoluteLock;
	Optics* m_relativeLockParent;
	std::list<Optics*> m_relativeLockChildren;
	static int m_lastId;
};

/////////////////////////////////////////////////
// Pure virtual Optics classes

/**
* Generic class for ABCD tranformation optics
* All ABCD type optics should inherit from this class
* By default, this classs implements the identity optics
*/
class ABCD : public Optics
{
public:
	/// Constructor
	ABCD(OpticsType type, double position, std::string name = "")
		: Optics(type, position, name) { setABCD(); }
	/// Destructor
	virtual ~ABCD() {}

public:
	/// @return the image of @p inputBeam
	virtual Beam image(const Beam& inputBeam, const Beam& opticalAxis) const;
	/// @return the antecedent if @p outputBeam
	virtual Beam antecedent(const Beam& outputBeam, const Beam& opticalAxis) const;

public:
	/// @return coefficient A of the ABCD matrix
	virtual double A() const { return 1.; }
	/// @return coefficient B of the ABCD matrix
	virtual double B() const { return 0.; }
	/// @return coefficient C of the ABCD matrix
	virtual double C() const { return 0.; }
	/// @return coefficient D of the ABCD matrix
	virtual double D() const { return 1.; }

/// @todo transfer this to the cavity class
public:
	bool stabilityCriterion1() const;
	bool stabilityCriterion2() const;
	Beam eigenMode(double wavelength) const;

private:
	inline void forward(const Beam& inputBeam, Beam& outputBeam, Orientation orientation) const;
	inline void backward(const Beam& outputBeam, Beam& inputBeam, Orientation orientation) const;
};

/**
* Generic class for dielectric materials
*/
class Dielectric
{
public:
	/// Constructor
	Dielectric(double indexRatio) : m_indexRatio(indexRatio) {}

public:
	/**
	* @return the refractive index jump characterizing the dielectric.
	* This index jump is the ratio between the index on the
	* right hand side of the optics (i.e. output index) and the
	* index on the left hand side of the optics (i.e. input index)
	*/
	double indexRatio() const { return m_indexRatio; }
	/// Set the index jump to @p indexRatio
	void setIndexRatio(double indexRatio);

private:
	double m_indexRatio;
};

/**
* Generic class for interface type optics
*/
class Interface : public ABCD, public Dielectric
{
public:
	/// Constructor
	Interface(OpticsType type, double indexRatio, double position, std::string name = "")
		: ABCD(type, position, name), Dielectric(indexRatio) { setRotable(); }
	/// Destructor
	virtual ~Interface() {}

public:
	virtual double indexJump() const { return indexRatio(); }
	virtual double D() const { return 1./indexRatio(); }
};

/////////////////////////////////////////////////
// Non ABCD optics

/**
* This optics is used to construct the input beam of an optics set
* Its image is constant whatever the antecedent is, and is defined
* by the class parameters. This  class enables to include the input
* beam specifications in an optics set.
*/
class CreateBeam : public Optics
{
public:
	/// Constructor
	CreateBeam(double waist, double waistPosition, double index, std::string name = "");
	virtual CreateBeam* clone() const { return new CreateBeam(*this); }

public:
	virtual Beam image(const Beam& inputBeam, const Beam& opticalAxis) const;
	virtual Beam antecedent(const Beam& outputBeam, const Beam& opticalAxis) const;

public:
	const Beam* beam() const;
	void setBeam(const Beam& beam);

private:
	Beam m_beam;
};

/////////////////////////////////////////////////
// ABCD optics

/**
* This class inplements the free space propagation of a Gaussian beam
* This is the identity tranformation with respect to beam transformation,
* but not regarding ABCD matrices.
*/
class FreeSpace : public ABCD
{
public:
	/// Constructor
	FreeSpace(double width, double position, std::string name = "")
		: ABCD(FreeSpaceType, position, name) { setWidth(width); }
	virtual FreeSpace* clone() const { return new FreeSpace(*this); }

public:
	virtual double B() const { return width(); }
};

/**
* Thin lens optics, defined by its focal lens
*/
class Lens : public ABCD
{
public:
	/// Constructor
	Lens(double focal, double position, std::string name = "")
		: ABCD(LensType, position, name) , m_focal(focal) { setOrientable(); }
	virtual Lens* clone() const { return new Lens(*this); }

public:
	virtual double C() const { return -1./focal(); }

public:
	/// @return the lens focal length
	double focal() const { return m_focal; }
	/// Set the lens focal
	void setFocal(double focal);

private:
	double m_focal;
};

/**
* Thick lens class.
* @warning This class is not finished yet
* @todo finish this class
*/
class ThickLens : public Lens, public Dielectric
{
public:
	/// Constructor
	ThickLens(double focal, double indexRatio, double position, std::string name = "")
		: Lens(focal, position, name), Dielectric(indexRatio) { setType(ThickLensType); }
	virtual ThickLens* clone() const { return new ThickLens(*this); }

public:
	virtual double A() const { return -1./focal(); }
	virtual double B() const { return -1./focal(); }
	virtual double D() const { return -1./focal(); }
};

/**
* Flat mirror optics. With repsect to beam propagation, this optics
* is the indentiy, either for beams or ABCD matrices.
* This class is used for cavities, for instance to bound a cavity
*/
class FlatMirror : public ABCD
{
public:
	/// Constructor
	FlatMirror(double position, std::string name = "")
		: ABCD(FlatMirrorType, position, name) { setRotable(); }
	virtual FlatMirror* clone() const { return new FlatMirror(*this); }

public:
	virtual Beam image(const Beam& inputBeam, const Beam& opticalAxis) const;
	virtual Beam antecedent(const Beam& outputBeam, const Beam& opticalAxis) const;
};

/**
* Implements a curved mirror.
* This is equivalent to a lens of focal curvatureRadius()/2 and the reflection properties of a mirror
*/
class CurvedMirror : public FlatMirror
{
public:
	/// Constructor
	CurvedMirror(double curvatureRadius, double position, std::string name = "")
		: FlatMirror(position, name), m_curvatureRadius(curvatureRadius) { setType(CurvedMirrorType); setOrientable(); }
	virtual CurvedMirror* clone() const { return new CurvedMirror(*this); }

public:
 	virtual double C() const { return -2./curvatureRadius(); }

public:
	/// @return the mirror curvature radius
	double curvatureRadius() const { return m_curvatureRadius; }
	/// Set the mirror curvature radius
	void setCurvatureRadius(double curvatureRadius);

private:
	double m_curvatureRadius;
};

/**
* Flat interface between to medium of different refraction index
*/
class FlatInterface : public Interface
{
public:
	/// Constructor
	FlatInterface(double indexRatio, double position, std::string name = "")
		: Interface(FlatInterfaceType, indexRatio, position, name) {}
	virtual FlatInterface* clone() const { return new FlatInterface(*this); }
};

/**
* Curved interface between to medium of different refraction index
*/
class CurvedInterface : public Interface
{
public:
	/// Constructor
	CurvedInterface(double surfaceRadius, double indexRatio, double position, std::string name = "")
		: Interface(CurvedInterfaceType, indexRatio, position, name), m_surfaceRadius(surfaceRadius) { setOrientable(); }
	virtual CurvedInterface* clone() const { return new CurvedInterface(*this); }

public:
	virtual double C() const { return (1./indexRatio()-1.)/surfaceRadius(); }

public:
	/// @return the surface curvature radius
	double surfaceRadius() const { return m_surfaceRadius; }
	/// set the surface curvature radius
	void setSurfaceRadius(double surfaceRadius);

private:
	double m_surfaceRadius;
};

/**
* Finite extension in space of a dielectric medium, defined by its index jum
* and its width
*/
class DielectricSlab : public ABCD, public Dielectric
{
public:
	/// Constructor
	DielectricSlab(double indexRatio, double width, double position, std::string name = "")
		: ABCD(DielectricSlabType, position, name), Dielectric(indexRatio) { setWidth(width); }
	virtual DielectricSlab* clone() const { return new DielectricSlab(*this); }

public:
	virtual double B() const { return width()/indexRatio(); }
};

/**
* User defined ABCD transformation
*/
class GenericABCD : public ABCD
{
public:
	/// Default constructor. Build an identity matrix
	GenericABCD()
		: ABCD(GenericABCDType, 0.)
		, m_A(1.), m_B(0.), m_C(0.), m_D(1.) {}
	/// Copy constructor
	GenericABCD(const ABCD& abcd)
		: ABCD(abcd)
		, m_A(abcd.A()), m_B(abcd.B()), m_C(abcd.C()), m_D(abcd.D())
		{ setType(GenericABCDType); }
	/// Full constructor
	GenericABCD(double A, double B, double C, double D, double width, double position, std::string name = "")
		: ABCD(GenericABCDType, position, name)
		, m_A(A), m_B(B), m_C(C), m_D(D) { setWidth(width); }
	virtual GenericABCD* clone() const { return new GenericABCD(*this); }
	/// Compose ABCD matrices
	GenericABCD& operator*=(const ABCD& abcd1);

public:
	virtual double A() const { return m_A; }
	virtual double B() const { return m_B; }
	virtual double C() const { return m_C; }
	virtual double D() const { return m_D; }

/// @todo check if the given ABCD matrix is valid
public:
	/// Set the matrix coefficient A
	void setA(double A) { m_A = A; }
	/// Set the matrix coefficient B
	void setB(double B) { m_B = B; }
	/// Set the matrix coefficient C
	void setC(double C) { m_C = C; }
	/// Set the matrix coefficient D
	void setD(double D) { m_D = D; }

private:
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

std::ostream& operator<<(std::ostream& out, const ABCD& abcd);

#endif
