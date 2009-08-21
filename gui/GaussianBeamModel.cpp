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

#include "src/OpticsBench.h"
#include "src/GaussianBeam.h"
#include "src/Utils.h"
#include "gui/GaussianBeamModel.h"
#include "gui/OpticsWidgets.h"
#include "gui/Unit.h"
#include "gui/Names.h"

#include <QtGui>
#include <QDebug>
#include <QTextStream>

GaussianBeamModel::GaussianBeamModel(OpticsBench* bench, TablePropertySelector* propertySelector, QObject* parent)
	: QAbstractTableModel(parent)
	, m_propertySelector(propertySelector)

{
	m_bench = bench;
	m_bench->registerEventListener(this);

	m_columns = m_propertySelector->checkedItems();

	connect(m_propertySelector, SIGNAL(propertyChanged()), this, SLOT(propertyWidgetModified()));

	// Sync with bench
	for (int i = 0; i < m_bench->nOptics(); i++)
		onOpticsBenchOpticsAdded(i);
}

GaussianBeamModel::~GaussianBeamModel()
{
}

void GaussianBeamModel::propertyWidgetModified()
{
	m_columns = m_propertySelector->checkedItems();
	reset();
}

int GaussianBeamModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_bench->nOptics();
}

int GaussianBeamModel::columnCount(const QModelIndex& /*parent*/) const
{
	return m_columns.size();
}

QVariant GaussianBeamModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || ((role != Qt::DisplayRole) && (role != Qt::EditRole)))
		return QVariant();

	int row = index.row();
	Property::Type column = m_columns[index.column()];
	const Optics* optics = m_bench->optics(row);

	QList<QVariant> values;

	if (column == Property::OpticsType)
		return OpticsName::fullName[m_bench->optics(row)->type()];
	else if ((column == Property::OpticsPosition) && (optics->type() != CreateBeamType))
		values << optics->position()*Units::getUnit(UnitPosition).divider();
	else if ((column == Property::OpticsRelativePosition) && (row > 0))
		values << (optics->position() - m_bench->optics(row-1)->position())*Units::getUnit(UnitPosition).divider();
	else if (column == Property::OpticsProperties)
	{
		if (optics->type() == CreateBeamType)
		{
			return QString("n = ") + QString::number(dynamic_cast<const CreateBeam*>(optics)->beam()->index()) +
			       QString(", " + tr("M²") + " = ") + QString::number(dynamic_cast<const CreateBeam*>(optics)->beam()->M2());
		}
		else if (optics->type() == LensType)
		{
			return QString("f = ") + QString::number(dynamic_cast<const Lens*>(optics)->focal()*Units::getUnit(UnitFocal).divider())
			                       + Units::getUnit(UnitFocal).string();
		}
		else if (optics->type() == CurvedMirrorType)
		{
			return QString("R = ") + QString::number(dynamic_cast<const CurvedMirror*>(optics)->curvatureRadius()*Units::getUnit(UnitCurvature).divider())
			                       + Units::getUnit(UnitCurvature).string();
		}
		else if (optics->type() == FlatInterfaceType)
		{
			return QString("n2/n1 = ") + QString::number(dynamic_cast<const FlatInterface*>(optics)->indexRatio());
		}
		else if (optics->type() == CurvedInterfaceType)
		{
			const CurvedInterface* interface = dynamic_cast<const CurvedInterface*>(optics);
			return QString("n2/n1 = ") + QString::number(interface->indexRatio()) +
			       QString("\nR = ") + QString::number(interface->surfaceRadius()*Units::getUnit(UnitCurvature).divider())
			                       + Units::getUnit(UnitCurvature).string();
		}
		else if (optics->type() == DielectricSlabType)
		{
			const DielectricSlab* slab = dynamic_cast<const DielectricSlab*>(optics);
			return QString("n2/n1 = ") + QString::number(slab->indexRatio()) +
			       QString("\n") + tr("width") + " = " + QString::number(optics->width()*Units::getUnit(UnitWidth).divider())
			                       + Units::getUnit(UnitWidth).string();
		}
		else if (optics->type() == GenericABCDType)
		{
			const ABCD* abcd = dynamic_cast<const ABCD*>(optics);
			return QString("A = ") + QString::number(abcd->A()) +
			       QString("\nB = ") + QString::number(abcd->B()*Units::getUnit(UnitABCD).divider())
			                         + Units::getUnit(UnitABCD).string() +
			       QString("\nC = ") + QString::number(abcd->C()/Units::getUnit(UnitABCD).divider())
			                         + " /" + Units::getUnit(UnitABCD).string(false) +
			       QString("\nD = ") + QString::number(abcd->D()) +
			       QString("\n") + tr("width") + " = " + QString::number(optics->width()*Units::getUnit(UnitWidth).divider())
			                       + Units::getUnit(UnitWidth).string();
		}
	}
	else if (column == Property::BeamWaist)
	{
		values << m_bench->beam(row)->waist(Horizontal)*Units::getUnit(UnitWaist).divider();
		if (!m_bench->isSpherical()) values << m_bench->beam(row)->waist(Vertical)*Units::getUnit(UnitWaist).divider();
	}
	else if (column == Property::BeamWaistPosition)
	{
		values << m_bench->beam(row)->waistPosition(Horizontal)*Units::getUnit(UnitPosition).divider();
		if (!m_bench->isSpherical()) values << m_bench->beam(row)->waistPosition(Vertical)*Units::getUnit(UnitPosition).divider();
	}
	else if (column == Property::BeamRayleigh)
	{
		values << m_bench->beam(row)->rayleigh(Horizontal)*Units::getUnit(UnitRayleigh).divider();
		if (!m_bench->isSpherical()) values << m_bench->beam(row)->rayleigh(Vertical)*Units::getUnit(UnitRayleigh).divider();
	}
	else if (column == Property::BeamDivergence)
	{
		values << m_bench->beam(row)->divergence(Horizontal)*Units::getUnit(UnitDivergence).divider();
		if (!m_bench->isSpherical()) values << m_bench->beam(row)->divergence(Vertical)*Units::getUnit(UnitDivergence).divider();
	}
	else if (column == Property::OpticsSensitivity)
		values << fabs(m_bench->sensitivity(row))*100./sqr(Units::getUnit(UnitPosition).divider());
	else if (column == Property::OpticsName)
	{
		return QString::fromUtf8(optics->name().c_str());
	}
	else if (column == Property::OpticsLock)
	{
		if (optics->absoluteLock())
			return tr("absolute");
		else if (optics->relativeLockParent())
			return QString::fromUtf8(optics->relativeLockParent()->name().c_str());
		else
			return tr("none");
	}
	else if ((column == Property::OpticsAngle) && optics->isRotable())
		values << optics->angle()*180./M_PI;
	else if ((column == Property::OpticsOrientation) && optics->isOrientable())
		return OrientationName::fullName[optics->orientation()];
	else
		return QVariant();

	if (role == Qt::EditRole)
		return values;

	QString string;
	QTextStream data(&string);
	data.setRealNumberNotation(QTextStream::SmartNotation);
//	data.setRealNumberPrecision(2);
	bool start = true;
	foreach(QVariant value, values)
	{
		if (!start) data << "\n";
		data << value.toDouble();
		start = false;
	}

	return string;
}

QVariant GaussianBeamModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal)
		{
			Property::Type type = m_columns[section];
			QString header = m_propertySelector->showFullName() ? Property::fullName[type] : Property::shortName[type];
			if (Property::unit[type] != UnitLess)
				header += " (" + Units::getUnit(Property::unit[type]).string(false) + ")";
			/// @todo handle this special case in a more general way
			if (type == Property::OpticsSensitivity)
				header += " (%/" + Units::getUnit(UnitPosition).string(false) + tr("²") + ")";
			return breakString(header);
		}
		else if (orientation == Qt::Vertical)
			return section;
	}

	return QVariant();
}

bool GaussianBeamModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid() || (role != Qt::EditRole) || value.isNull() || !value.isValid())
		return false;

	int row = index.row();
	Property::Type column = m_columns[index.column()];

	QList<Orientation> orientations;
	if (m_bench->isSpherical())
		orientations << Spherical;
	else
		orientations << Horizontal << Vertical;

	if (column == Property::OpticsPosition)
		m_bench->setOpticsPosition(row, value.toDouble()*Units::getUnit(UnitPosition).multiplier(), row != 0);
	else if (column == Property::OpticsRelativePosition)
	{
		if (row == 0)
			return false;

		const Optics* optics = m_bench->optics(row);
		const Optics* preceedingOptics = m_bench->optics(row-1);
		if (optics->relativeLockedTo(preceedingOptics))
			return false;
		m_bench->setOpticsPosition(row, value.toDouble()*Units::getUnit(UnitPosition).multiplier() + preceedingOptics->position());
	}
	else if (column == Property::OpticsProperties)
	{
		Optics* optics = m_bench->opticsForPropertyChange(row);

		if (optics->type() == CreateBeamType)
		{
			CreateBeam* createBeam = dynamic_cast<CreateBeam*>(optics);
			Beam beam = *createBeam->beam();
			beam.setIndex(value.toList()[0].toDouble());
			beam.setM2(value.toList()[1].toDouble());
			createBeam->setBeam(beam);
		}
		else if (optics->type() == LensType)
			dynamic_cast<Lens*>(optics)->setFocal(value.toList()[0].toDouble()*Units::getUnit(UnitFocal).multiplier());
		else if (optics->type() == CurvedMirrorType)
			dynamic_cast<CurvedMirror*>(optics)->setCurvatureRadius(value.toList()[0].toDouble()*Units::getUnit(UnitCurvature).multiplier());
		else if (optics->type() == FlatInterfaceType)
			dynamic_cast<FlatInterface*>(optics)->setIndexRatio(value.toList()[0].toDouble());
		else if (optics->type() == CurvedInterfaceType)
		{
			CurvedInterface* curvedInterfaceOptics = dynamic_cast<CurvedInterface*>(optics);
			curvedInterfaceOptics->setIndexRatio(value.toList()[0].toDouble());
			curvedInterfaceOptics->setSurfaceRadius(value.toList()[1].toDouble()*Units::getUnit(UnitCurvature).multiplier());
		}
		else if (optics->type() == DielectricSlabType)
		{
			DielectricSlab* dielectricSlabOptics = dynamic_cast<DielectricSlab*>(optics);
			dielectricSlabOptics->setIndexRatio(value.toList()[0].toDouble());
			dielectricSlabOptics->setWidth(value.toList()[1].toDouble()*Units::getUnit(UnitWidth).multiplier());
		}
		else if (optics->type() == GenericABCDType)
		{
			/// @todo check that the ABCD matrix is valid, e.g. by introducing bool GenericABCD::isValid()
			GenericABCD* ABCDOptics = dynamic_cast<GenericABCD*>(optics);
			ABCDOptics->setA(value.toList()[0].toDouble());
			ABCDOptics->setB(value.toList()[1].toDouble()*Units::getUnit(UnitABCD).multiplier());
			ABCDOptics->setC(value.toList()[2].toDouble()/Units::getUnit(UnitABCD).multiplier());
			ABCDOptics->setD(value.toList()[3].toDouble());
			ABCDOptics->setWidth(value.toList()[4].toDouble()*Units::getUnit(UnitWidth).multiplier());
		}
		m_bench->opticsPropertyChanged(row);
	}
	else if (column == Property::BeamWaist)
		for (int i = 0; i < orientations.size(); i++)
		{
			Beam beam = *m_bench->beam(row);
			beam.setWaist(value.toList()[i].toDouble()*Units::getUnit(UnitWaist).multiplier(), orientations[i]);
			m_bench->setBeam(beam, row);
		}
	else if (column == Property::BeamWaistPosition)
		for (int i = 0; i < orientations.size(); i++)
		{
			Beam beam = *m_bench->beam(row);
			beam.setWaistPosition(value.toList()[i].toDouble()*Units::getUnit(UnitPosition).multiplier(), orientations[i]);
			m_bench->setBeam(beam, row);
		}
	else if (column == Property::BeamRayleigh)
		for (int i = 0; i < orientations.size(); i++)
		{
			Beam beam = *m_bench->beam(row);
			beam.setRayleigh(value.toList()[i].toDouble()*Units::getUnit(UnitRayleigh).multiplier(), orientations[i]);
			m_bench->setBeam(beam, row);
		}
	else if (column == Property::BeamDivergence)
		for (int i = 0; i < orientations.size(); i++)
		{
			Beam beam = *m_bench->beam(row);
			beam.setDivergence(value.toList()[i].toDouble()*Units::getUnit(UnitDivergence).multiplier(), orientations[i]);
			m_bench->setBeam(beam, row);
		}
	else if (column == Property::OpticsName)
		m_bench->setOpticsName(row, value.toString().toUtf8().data());
	else if (column == Property::OpticsLock)
	{
		/// @todo make specific functions in OpticsBench to change this. Move this logic to OpticsBench
		int lockId = value.toInt();
		if (value == -2)
		{
			Optics* optics = m_bench->opticsForPropertyChange(row);
			optics->setAbsoluteLock(false);
			optics->relativeUnlock();
			m_bench->opticsPropertyChanged(row);
		}
		else if (value == -1)
		{
			Optics* optics = m_bench->opticsForPropertyChange(row);
			optics->setAbsoluteLock(true);
			m_bench->opticsPropertyChanged(row);
		}
		else
			m_bench->lockTo(row, lockId);
	}
	else if (column == Property::OpticsAngle)
	{
		Optics* optics = m_bench->opticsForPropertyChange(row);
		optics->setAngle(value.toDouble()*M_PI/180.);
		m_bench->opticsPropertyChanged(row);
	}
	else if (column == Property::OpticsOrientation)
	{
		Optics* optics = m_bench->opticsForPropertyChange(row);
		optics->setOrientation(Orientation(value.toInt()));
		m_bench->opticsPropertyChanged(row);
	}

	return true;
}

Qt::ItemFlags GaussianBeamModel::flags(const QModelIndex& index) const
{
	int row = index.row();
	Property::Type column = m_columns[index.column()];

	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

	const Optics* optics = m_bench->optics(row);

	if ((column == Property::OpticsName) ||
		(column == Property::OpticsLock) ||
		(column == Property::BeamWaist) ||
		(column == Property::BeamWaistPosition) ||
		(column == Property::BeamRayleigh) ||
		(column == Property::BeamDivergence) ||
		((column == Property::OpticsProperties)  && (optics->type() != FlatMirrorType)) ||
		((column == Property::OpticsAngle)       && (optics->isRotable())) ||
		((column == Property::OpticsOrientation) && (optics->isOrientable())))
			flags |= Qt::ItemIsEditable;

	if (column == Property::OpticsPosition)
		if (!optics->relativeLockTreeAbsoluteLock() && (optics->type() != CreateBeamType))
			flags |= Qt::ItemIsEditable;

	if ((column == Property::OpticsRelativePosition) && (row != 0))
	{
		const Optics* preceedingOptics = m_bench->optics(row-1);
		if (!optics->relativeLockedTo(preceedingOptics) &&
		    !optics->relativeLockTreeAbsoluteLock())
			flags |= Qt::ItemIsEditable;
	}


	return flags;
}

bool GaussianBeamModel::insertRows(int row, int count, const QModelIndex& parent)
{
	beginInsertRows(parent, row, row + count -1);
	endInsertRows();
	return true;
}

bool GaussianBeamModel::removeRows(int row, int count, const QModelIndex& parent)
{
	beginRemoveRows(parent, row, row + count -1);
	endRemoveRows();
	return true;
}

void GaussianBeamModel::onOpticsBenchDataChanged(int startOptics, int endOptics)
{
	emit dataChanged(index(startOptics, 0), index(endOptics, columnCount()-1));
}

void GaussianBeamModel::onOpticsBenchOpticsAdded(int index)
{
	insertRows(index, 1);
}

void GaussianBeamModel::onOpticsBenchOpticsRemoved(int index, int count)
{
	removeRows(index, count);
}
