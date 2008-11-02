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

#include <gui/Names.h>

namespace Property
{
	QMap<Property::Type, QString> fullName;
	QMap<Property::Type, QString> shortName;
	QMap<Property::Type, UnitType> unit;
}

namespace OpticsName
{
	QMap<OpticsType, QString> fullName;
}

void initNames(QApplication* app)
{
	Property::fullName.insert(Property::BeamPosition,           app->translate("Names", "Position"));
	Property::fullName.insert(Property::BeamRadius,             app->translate("Names", "Beam radius"));
	Property::fullName.insert(Property::BeamDiameter,           app->translate("Names", "Beam diameter"));
	Property::fullName.insert(Property::BeamCurvature,          app->translate("Names", "Beam curvature"));
	Property::fullName.insert(Property::BeamGouyPhase,          app->translate("Names", "Gouy phase"));
	Property::fullName.insert(Property::BeamDistanceToWaist,    app->translate("Names", "Distance to waist"));
	Property::fullName.insert(Property::BeamParameter,          app->translate("Names", "Beam parameter"));
	Property::fullName.insert(Property::Index,                  app->translate("Names", "Index"));
	Property::fullName.insert(Property::OpticsType,             app->translate("Names", "Optics"));
	Property::fullName.insert(Property::OpticsPosition,         app->translate("Names", "Position"));
	Property::fullName.insert(Property::OpticsRelativePosition, app->translate("Names", "Relative position"));
	Property::fullName.insert(Property::OpticsProperties,       app->translate("Names", "Properties"));
	Property::fullName.insert(Property::BeamWaist,              app->translate("Names", "Waist"));
	Property::fullName.insert(Property::BeamWaistPosition,      app->translate("Names", "Waist position"));
	Property::fullName.insert(Property::BeamRayleigh,           app->translate("Names", "Rayleigh range"));
	Property::fullName.insert(Property::BeamDivergence,         app->translate("Names", "Divergence"));
	Property::fullName.insert(Property::OpticsSensitivity,      app->translate("Names", "Sensitivity"));
	Property::fullName.insert(Property::OpticsName,             app->translate("Names", "Name"));
	Property::fullName.insert(Property::OpticsLock,             app->translate("Names", "Lock"));
	Property::fullName.insert(Property::OpticsCavity,           app->translate("Names", "Cavity"));

	Property::shortName.insert(Property::BeamPosition,           app->translate("Names", "z"));
	Property::shortName.insert(Property::BeamRadius,             app->translate("Names", "w"));
	Property::shortName.insert(Property::BeamDiameter,           app->translate("Names", "2w"));
	Property::shortName.insert(Property::BeamCurvature,          app->translate("Names", "R"));
	Property::shortName.insert(Property::BeamGouyPhase,          app->translate("Names", "ζ"));
	Property::shortName.insert(Property::BeamDistanceToWaist,    app->translate("Names", "z-zw"));
	Property::shortName.insert(Property::BeamParameter,          app->translate("Names", "q"));
	Property::shortName.insert(Property::Index,                  app->translate("Names", "n"));
	Property::shortName.insert(Property::OpticsType,             app->translate("Names", "Optics"));
	Property::shortName.insert(Property::OpticsPosition,         app->translate("Names", "z"));
	Property::shortName.insert(Property::OpticsRelativePosition, app->translate("Names", "zr"));
	Property::shortName.insert(Property::OpticsProperties,       app->translate("Names", "Prop."));
	Property::shortName.insert(Property::BeamWaist,              app->translate("Names", "w₀"));
	Property::shortName.insert(Property::BeamWaistPosition,      app->translate("Names", "zw"));
	Property::shortName.insert(Property::BeamRayleigh,           app->translate("Names", "z₀"));
	Property::shortName.insert(Property::BeamDivergence,         app->translate("Names", "Θ₀"));
	Property::shortName.insert(Property::OpticsSensitivity,      app->translate("Names", "Sens."));
	Property::shortName.insert(Property::OpticsName,             app->translate("Names", "Name"));
	Property::shortName.insert(Property::OpticsLock,             app->translate("Names", "Lock"));
	Property::shortName.insert(Property::OpticsCavity,           app->translate("Names", "Cav."));

	Property::unit.insert(Property::BeamPosition,           UnitPosition);
	Property::unit.insert(Property::BeamRadius,             UnitWaist);
	Property::unit.insert(Property::BeamDiameter,           UnitWaist);
	Property::unit.insert(Property::BeamCurvature,          UnitCurvature);
	Property::unit.insert(Property::BeamGouyPhase,          UnitPhase);
	Property::unit.insert(Property::BeamDistanceToWaist,    UnitPosition);
	Property::unit.insert(Property::BeamParameter,          UnitPosition);
	Property::unit.insert(Property::Index,                  UnitLess);
	Property::unit.insert(Property::OpticsType,             UnitLess);
	Property::unit.insert(Property::OpticsPosition,         UnitPosition);
	Property::unit.insert(Property::OpticsRelativePosition, UnitPosition);
	Property::unit.insert(Property::OpticsProperties,       UnitLess);
	Property::unit.insert(Property::BeamWaist,              UnitWaist);
	Property::unit.insert(Property::BeamWaistPosition,      UnitPosition);
	Property::unit.insert(Property::BeamRayleigh,           UnitRayleigh);
	Property::unit.insert(Property::BeamDivergence,         UnitDivergence);
	Property::unit.insert(Property::OpticsSensitivity,      UnitLess);//app->translate(       "Sensitivity") + "\n(%/" + Units::getUnit(UnitPosition).string(false) + tr("²") + ")";
	Property::unit.insert(Property::OpticsName,             UnitLess);
	Property::unit.insert(Property::OpticsLock,             UnitLess);
	Property::unit.insert(Property::OpticsCavity,           UnitLess);

	OpticsName::fullName.insert(CreateBeamType,      app->translate("Names", "Input beam"));
	OpticsName::fullName.insert(LensType,            app->translate("Names", "Lens"));
	OpticsName::fullName.insert(ThickLensType,       app->translate("Names", "Thick lens"));
	OpticsName::fullName.insert(FlatMirrorType,      app->translate("Names", "Flat mirror"));
	OpticsName::fullName.insert(CurvedMirrorType,    app->translate("Names", "Curved mirror"));
	OpticsName::fullName.insert(FlatInterfaceType,   app->translate("Names", "Flat interface"));
	OpticsName::fullName.insert(CurvedInterfaceType, app->translate("Names", "Curved interface"));
	OpticsName::fullName.insert(DielectricSlabType,  app->translate("Names", "Dielectric slab"));
	OpticsName::fullName.insert(ThermalLensType,     app->translate("Names", "Thermal lens"));
	OpticsName::fullName.insert(GenericABCDType,     app->translate("Names", "Generic ABCD"));
}

QString breakString(QString string)
{
	if (string.size() < 12)
		return string;

	int lastSplitPos = -1;
	int bestSplitPos = -1;
	int bestLength = 1024;
	while ((lastSplitPos = string.indexOf(QChar(' '), lastSplitPos + 1)) != -1)
	{
		int length = qMax(lastSplitPos, string.size());
		if (length < bestLength)
		{
			bestSplitPos = lastSplitPos;
			bestLength = length;
		}
	}

	if (bestSplitPos != -1)
		string.replace(bestSplitPos, 1, QChar('\n'));

	return string;
}
