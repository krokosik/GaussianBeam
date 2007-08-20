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

	double radius(double z) const;
	double radiusDerivative(double z) const;
	double radiusSecondDerivative(double z) const;
	double curvature(double z) const;
	double gouyPhase(double z) const;
	std::complex<double> q(double z) const;

	double approxNextPosition(double currentPosition, Approximation& approximation) const;

private:
	inline double alpha(double z) const { return (z - waistPosition())/rayleigh(); }

private:
	double m_waist;
	double m_waistPosition;
	double m_wavelength;
};

enum OpticsType {CreateBeamType, LensType, FlatInterfaceType, CurvedInterfaceType};

class Optics
{
public:
	Optics(OpticsType type, double position, std::string name);
	virtual ~Optics() {}

public:
	virtual Beam image(const Beam& inputBeam) const = 0;
	virtual Beam antecedent(const Beam& outputBeam) const = 0;

public:
	OpticsType type() const { return m_type; }
	double position() const { return m_position; }
	void setPosition(double position) { m_position = position; }
	bool locked() const { return m_locked; }
	void setLocked(bool locked) { m_locked = locked; }
	std::string name() const { return m_name; }
	void setName(std::string name) { m_name = name; }

protected:
	OpticsType m_type;
	double m_position;
	std::string m_name;
	bool m_locked;
};

class CreateBeam : public Optics
{
public:
	CreateBeam(double waist, double waistPosition, std::string name = "");

public:
	virtual Beam image(const Beam& inputBeam) const;
	virtual Beam antecedent(const Beam& outputBeam) const;

public:
	double waist() const { return m_waist; }
	void setWaist(double waist) { m_waist = waist; }

protected:
	double m_waist;
};

class Lens : public Optics
{
public:
	Lens(double focal, double position, std::string name = "");
	virtual ~Lens() {}

public:
	virtual Beam image(const Beam& inputBeam) const;
	virtual Beam antecedent(const Beam& outputBeam) const;

public:
	double focal() const { return m_focal; }
	void setFocal(double focal) { m_focal = focal; }

protected:
	double m_focal;
};

/**
* Virtual class
*/
class Interface : public Optics
{
public:
	/**
	* Constructor
	* @param indexRation ratio between the final index and the initial index
	* @param position position of the interface
	* @param name user name for the optics
	*/
	Interface(OpticsType type, double indexRatio, double position, std::string name = "");
	virtual ~Interface() {}

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
	FlatInterface(double indexRatio, double position, std::string name = "");
	virtual ~FlatInterface() {}

public:
	virtual Beam image(const Beam& inputBeam) const;
	virtual Beam antecedent(const Beam& outputBeam) const;

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
	CurvedInterface(double surfaceRadius, double indexRatio, double position, std::string name = "");
	virtual ~CurvedInterface() {}

public:
	virtual Beam image(const Beam& inputBeam) const;
	virtual Beam antecedent(const Beam& outputBeam) const;

public:
	double surfaceRadius() const { return m_surfaceRadius; }
	void setSurfaceRadius(double surfaceRadius) { m_surfaceRadius = surfaceRadius; }

protected:
	double m_surfaceRadius;
};

namespace GaussianBeam
{
	/**
	* Find a lens arrangement to produce @p targetBeam from @p inputBeam
	* @p inputBeam input waist
	* @p targetBeam target waist
	* @p lenses vector of lenses to use
	* @p waistTolerance relative tolerance on the waist
	* @p positionTolerance tolerance on waist position in Rayleigh ranges
	* @p scramble change lens ordering
	* @return true for success
	*/
	bool magicWaist(const Beam& inputBeam, const Beam& targetBeam, std::vector<Lens>& lenses,
	                double waistTolerance = 0.05, double positionTolerance = 0.05, bool scramble = false);
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
}

#endif
