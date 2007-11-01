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

#include "OpticsBench.h"
#include "Statistics.h"

#include <iostream>
#include <cmath>

using namespace std;

OpticsBench::OpticsBench()
{
}

/////////////////////////////////////////////////
// GaussianBeam namespace

bool GaussianBeam::magicWaist(vector<Optics*>& optics, const MagicWaistTarget& target)
{
	const int nTry = 1000000;

	for (int i = 0; i < nTry; i++)
	{
		// Scramble lenses
		if (target.scramble)
			for (unsigned int j = 0; j < 3*optics.size(); j++)
				::swap(optics[rand() % (optics.size()-1) + 1], optics[rand() % (optics.size()-1) + 1]);
		// Place lenses
		Beam beam;
		beam.setWavelength(target.beam.wavelength());
		double previousPos = 0.;
		for (unsigned int l = 0; l < optics.size(); l++)
		{
			if (!optics[l]->absoluteLock())
			{
				/// @todo better range determination
				double position = double(rand())/double(RAND_MAX)*(target.beam.waistPosition() - previousPos) + previousPos;
				optics[l]->setPosition(position);
			}
			beam = optics[l]->image(beam);
			previousPos = optics[l]->position();
			/// @bug this is a hack !
			if ((optics[l]->type() == CreateBeamType) && (previousPos > 0.))
				previousPos = 0.;
		}
		// Check waist
		if (target.overlap &&
			(overlap(beam, target.beam) > target.minOverlap) ||
			(!target.overlap) &&
			(fabs(beam.waist() - target.beam.waist()) < target.waistTolerance*target.beam.waist()) &&
		    (fabs(beam.waistPosition() - target.beam.waistPosition()) < target.positionTolerance*target.beam.rayleigh()))
		{
			cerr << "found waist : " << beam.waist() << " @ " << beam.waistPosition() << " // try = " << i << endl;
			return true;
		}
	}

	cerr << "Beam not found !!!" << endl;

	return false;
}

Beam GaussianBeam::fitBeam(vector<double> positions, vector<double> radii, double wavelength, double* rho2)
{
	Beam beam;
	beam.setWavelength(wavelength);

	Statistics stats(positions, radii);

	// Some point whithin the fit
	const double z = stats.meanX;
	// beam radius at z
	const double fz = stats.m*z + stats.p;
	// derivative of the beam radius at z
	const double fpz = stats.m;
	// (z - zw)/z0  (zw : position of the waist, z0 : Rayleigh range)
	const double alpha = M_PI*fz*fpz/wavelength;
	// waist
	beam.setWaist(fz/sqrt(1. + sqr(alpha)));
	beam.setWaistPosition(z - beam.rayleigh()*alpha);

	if (rho2)
		*rho2 = stats.rho2;

	return beam;
}

double GaussianBeam::overlap(const Beam& beam1, const Beam& beam2, double z)
{
//	double w1 = beam1.radius(z);
//	double w2 = beam2.radius(z);
//	double w12 = sqr(beam1.radius(z));
//	double w22 = sqr(beam2.radius(z));
//	double k1 = 2.*M_PI/beam1.wavelength();
//	double k2 = 2.*M_PI/beam2.wavelength();
//	double R1 = beam1.curvature(z);
//	double R2 = beam2.curvature(z);
	double zred1 = beam1.zred(z);
	double zred2 = beam2.zred(z);
	double rho = sqr(beam1.radius(z)/beam2.radius(z));

	//double eta = 4./sqr(w1*w2)/(sqr(1./sqr(w1) + 1./sqr(w2)) + sqr((k1/R1 - k2/R2)/2.));
	//double eta = 4./(w12*w22)/(sqr(1./w12 + 1./w22) + sqr(zred1/w12 - zred2/w22));
	double eta = 4.*rho/(sqr(1. + rho) + sqr(zred1 - zred2*rho));

//	cerr << "Coupling = " << eta << " // " << zred1 << " " << zred2 << " " << rho << endl;

	return eta;
}
