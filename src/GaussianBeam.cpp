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

#include "GaussianBeam.h"
#include "Utils.h"

#include <iostream>
#include <algorithm>
#include <cmath>

using namespace std;

Beam::Beam()
{
	init();
}

Beam::Beam(double wavelength)
{
	init();
	setWavelength(wavelength);
}

Beam::Beam(double waist, double waistPosition, double wavelength, double index, double M2)
{
	init();
	setWavelength(wavelength);
	setIndex(index);
	setM2(M2);
	setWaist(waist);
	setWaistPosition(waistPosition);
}

Beam::Beam(const complex<double>& q, double z, double wavelength, double index, double M2)
{
	init();
	setWavelength(wavelength);
	setIndex(index);
	setM2(M2);
	setQ(q, z);
}

void Beam::init()
{
	m_wavelength = 461e-9;
	m_index = 1.;
	m_M2 = 1.;
	setWaist(1e-4);
	setWaistPosition(0.);
	m_angle = 0.;
	m_start = 0.;
	m_stop = 0.;
	m_origin = vector<double>(2, 0.);
}

/////////////
// Properties

double Beam::waistPosition(Orientation orientation) const
{
	if (orientation == Vertical)
		return m_waistPosition.second;

	return m_waistPosition.first;
}

void Beam::setWaistPosition(double waistPosition, Orientation orientation)
{
	if (orientation != Vertical)
		m_waistPosition.first = waistPosition;

	if (orientation != Horizontal)
		m_waistPosition.second = waistPosition;
}

double Beam::waist(Orientation orientation) const
{
	if (orientation == Vertical)
		return m_waist.second;

	return m_waist.first;
}

void Beam::setWaist(double waist, Orientation orientation)
{
	if (orientation != Vertical)
		m_waist.first = waist;

	if (orientation != Horizontal)
		m_waist.second = waist;
}

double Beam::divergence(Orientation orientation) const
{
	if (waist(orientation) == 0.)
		return 0.;

	return atan(m_wavelength*m_M2/(m_index*M_PI*waist(orientation)));
}

void Beam::setDivergence(double divergence, Orientation orientation)
{
	if ((divergence > 0.) && (divergence < M_PI/2.))
		setWaist(m_wavelength*m_M2/(m_index*M_PI*tan(divergence)), orientation);
}

double Beam::rayleigh(Orientation orientation) const
{
	if (m_wavelength == 0.)
		return 0.;

	return m_index*M_PI*sqr(waist(orientation))/(m_wavelength*m_M2);
}

void Beam::setRayleigh(double rayleigh, Orientation orientation)
{
	if (rayleigh > 0.)
		setWaist(sqrt(rayleigh*m_wavelength*m_M2/(m_index*M_PI)), orientation);
}

double Beam::wavelength() const
{
	return m_wavelength;
}

void Beam::setWavelength(double wavelength)
{
	m_wavelength = wavelength;
}

double Beam::index() const
{
	return m_index;
}

void Beam::setIndex(double index)
{
	if (index > 0.)
		m_index = index;
}

double Beam::M2() const
{
	return m_M2;
}

void Beam::setM2(double M2)
{
	if (M2 >= 1.)
		m_M2 = M2;
}

////////////////////////////////
// Position dependent properties

double Beam::radius(double z, Orientation orientation) const
{
	return waist(orientation)*sqrt(1. + sqr(zred(z, orientation)));
}

double Beam::radiusDerivative(double z, Orientation orientation) const
{
	return waist(orientation)/rayleigh(orientation)/sqrt(1. + 1./sqr(zred(z, orientation)));
}

double Beam::radiusSecondDerivative(double z, Orientation orientation) const
{
	return waist(orientation)/sqr(rayleigh(orientation))/pow(1. + sqr(zred(z, orientation)), 1.5);
}

double Beam::curvature(double z, Orientation orientation) const
{
	return (z - waistPosition(orientation))*(1. + 1./sqr(zred(z, orientation)));
}

double Beam::gouyPhase(double z, Orientation orientation) const
{
	return atan(zred(z, orientation));
}

void Beam::setQ(complex<double> q, double z, Orientation orientation)
{
	setRayleigh(q.imag(), orientation);
	setWaistPosition(z - q.real(), orientation);
}

complex<double> Beam::q(double z, Orientation orientation) const
{
	return complex<double>(z - waistPosition(orientation), rayleigh(orientation));
}

/////////////////////////
// Geometrical properties

double Beam::start() const
{
	return m_start;
}

void Beam::setStart(double start)
{
	m_start = start;
}

double Beam::stop() const
{
	return m_stop;
}

void Beam::setStop(double stop)
{
	m_stop = stop;
}

vector<double> Beam::origin() const
{
	return m_origin;
}

double Beam::angle() const
{
	return m_angle;
}

void Beam::rotate(double pivot, double angle)
{
	double l = 2.*pivot*sin(angle/2.);

	m_origin[0] += l*sin(m_angle + angle/2.);
	m_origin[1] -= l*cos(m_angle + angle/2.);

	m_angle = fmodPos(m_angle + angle, 2.*M_PI);
}

vector<double> Beam::beamCoordinates(const vector<double>& point) const
{
	vector<double> result(2, 0.);
	result[0] =  cos(m_angle)*(point[0] - m_origin[0]) + sin(m_angle)*(point[1] - m_origin[1]);
	result[1] = -sin(m_angle)*(point[0] - m_origin[0]) + cos(m_angle)*(point[1] - m_origin[1]);
	return result;
}

vector<double> Beam::absoluteCoordinates(double position, double distance) const
{
	vector<double> result = m_origin;
	result[0] += position*cos(m_angle) - distance*sin(m_angle);
	result[1] += position*sin(m_angle) + distance*cos(m_angle);
	return result;
}

vector<double> Beam::rectangleIntersection(vector<double> p1, vector<double> p2)
{
	static const double infinity = 1e100;
	p1 -= m_origin;
	p2 -= m_origin;
	vector<double> intersections;

	if (cos(m_angle) != 0.)
		intersections << p1[0]/cos(m_angle) << p2[0]/cos(m_angle);
	else
		intersections << -infinity << +infinity;

	if (sin(m_angle) != 0.)
		intersections << p1[1]/sin(m_angle) << p2[1]/sin(m_angle);
	else
		intersections << -infinity << +infinity;

	sort(intersections.begin(), intersections.end());
	intersections.pop_back();
	intersections.erase(intersections.begin());
	return intersections;
}

//////////////////////
// static computations

/// @todo 2D overlap
double Beam::overlap(const Beam& beam1, const Beam& beam2, double z, Orientation orientation)
{
	if (orientation == Spherical)
		return overlap(beam1, beam2, z, Horizontal);
	else if (orientation == Ellipsoidal)
		return sqrt(overlap(beam1, beam2, z, Horizontal)*overlap(beam1, beam2, z, Vertical));
	else if ((orientation == Horizontal) || (orientation == Vertical))
	{
	//	double w1 = beam1.radius(z);
	//	double w2 = beam2.radius(z);
	//	double w12 = sqr(beam1.radius(z));
	//	double w22 = sqr(beam2.radius(z));
	//	double k1 = 2.*M_PI/beam1.wavelength();
	//	double k2 = 2.*M_PI/beam2.wavelength();
	//	double R1 = beam1.curvature(z);
	//	double R2 = beam2.curvature(z);
		double zred1 = beam1.zred(z, Horizontal);
		double zred2 = beam2.zred(z, Horizontal);
		double rho = sqr(beam1.radius(z, Horizontal)/beam2.radius(z, Horizontal));

		//double eta = 4./sqr(w1*w2)/(sqr(1./sqr(w1) + 1./sqr(w2)) + sqr((k1/R1 - k2/R2)/2.));
		//double eta = 4./(w12*w22)/(sqr(1./w12 + 1./w22) + sqr(zred1/w12 - zred2/w22));
		double eta = 4.*rho/(sqr(1. + rho) + sqr(zred1 - zred2*rho));

		return eta;
	}

	return 0.;
}

bool Beam::copropagating(const Beam& beam1, const Beam& beam2)
{
	static const double epsilon = 1e-7;
	vector<double> deltaOrigin = beam1.origin() - beam2.origin();
	double deltaAngle = fmodPos(beam1.angle() - beam2.angle(), 2.*M_PI);

	cerr << " angles = " << beam1.angle() << " " << beam2.angle() << " " << deltaAngle << endl;
	cerr << " criterion = " << deltaOrigin[1] - tan(beam1.angle())*deltaOrigin[0] << endl;

	if (   (   (deltaAngle < epsilon)
		    || (deltaAngle > (2.*M_PI - epsilon)))
		&& (fabs(deltaOrigin[1] - tan(beam1.angle())*deltaOrigin[0]) < epsilon))
		return true;

	return false;
}

ostream& operator<<(ostream& out, const Beam& beam)
{
	out << "Waist = (" << beam.waist(Horizontal) << "," << beam.waist(Vertical)
	    << ")  Waist position = (" << beam.waistPosition(Horizontal) << "," << beam.waistPosition(Vertical) << ")";
	return out;
}

//////////
// Private

inline double Beam::zred(double z, Orientation orientation) const
{
	return (z - waistPosition(orientation))/rayleigh(orientation);
}

