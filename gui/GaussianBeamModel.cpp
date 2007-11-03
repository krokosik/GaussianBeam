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

#include "GaussianBeamModel.h"
#include "Unit.h"

#include <QtGui>
#include <QDebug>

GaussianBeamModel::GaussianBeamModel(OpticsBench& bench, QObject* parent)
	: QAbstractTableModel(parent)
	, m_bench(bench)

{
	m_bench.registerNotify(this);
}

GaussianBeamModel::~GaussianBeamModel()
{
}

int GaussianBeamModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_bench.nOptics();
}

int GaussianBeamModel::columnCount(const QModelIndex& /*parent*/) const
{
	return 10;
}

QVariant GaussianBeamModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || role != Qt::DisplayRole)
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (column == COL_OPTICS)
		return opticsName(m_bench.optics(row)->type());
	else if (column == COL_POSITION)
		return m_bench.optics(row)->position()*Units::getUnit(UnitPosition).divider();
	else if ((column == COL_RELATIVE_POSITION) && (row > 0))
		return (m_bench.optics(row)->position() - m_bench.optics(row-1)->position())*Units::getUnit(UnitPosition).divider();
	else if (column == COL_PROPERTIES)
	{
		if (m_bench.optics(row)->type() == LensType)
		{
			return QString("f = ") + QString::number(dynamic_cast<const Lens*>(m_bench.optics(row))->focal()*Units::getUnit(UnitFocal).divider())
			                       + Units::getUnit(UnitFocal).string("m");
		}
		else if (m_bench.optics(row)->type() == CurvedMirrorType)
		{
			return QString("R = ") + QString::number(dynamic_cast<const CurvedMirror*>(m_bench.optics(row))->curvatureRadius()*Units::getUnit(UnitCurvature).divider())
			                       + Units::getUnit(UnitCurvature).string("m");
		}
		else if (m_bench.optics(row)->type() == FlatInterfaceType)
		{
			return QString("n2/n1 = ") + QString::number(dynamic_cast<const FlatInterface*>(m_bench.optics(row))->indexRatio());
		}
		else if (m_bench.optics(row)->type() == CurvedInterfaceType)
		{
			const CurvedInterface* optics = dynamic_cast<const CurvedInterface*>(m_bench.optics(row));
			return QString("n2/n1 = ") + QString::number(optics->indexRatio()) +
			       QString("\nR = ") + QString::number(optics->surfaceRadius()*Units::getUnit(UnitCurvature).divider())
			                       + Units::getUnit(UnitCurvature).string("m");
		}
		else if (m_bench.optics(row)->type() == GenericABCDType)
		{
			const ABCD* optics = dynamic_cast<const ABCD*>(m_bench.optics(row));
			return QString("A = ") + QString::number(optics->A()) +
			       QString("\nB = ") + QString::number(optics->B()*Units::getUnit(UnitABCD).divider())
			                         + Units::getUnit(UnitABCD).string("m") +
			       QString("\nC = ") + QString::number(optics->C()/Units::getUnit(UnitABCD).divider())
			                         + " /" + Units::getUnit(UnitABCD).string("m", false) +
			       QString("\nD = ") + QString::number(optics->D()) +
			       QString("\nwidth = ") + QString::number(m_bench.optics(row)->width()*Units::getUnit(UnitWidth).divider())
			                       + Units::getUnit(UnitWidth).string("m");
		}
	}
	else if (column == COL_WAIST)
		return m_bench.beam(row).waist()*Units::getUnit(UnitWaist).divider();
	else if (column == COL_WAIST_POSITION)
		return m_bench.beam(row).waistPosition()*Units::getUnit(UnitPosition).divider();
	else if (column == COL_RAYLEIGH)
		return m_bench.beam(row).rayleigh()*Units::getUnit(UnitRayleigh).divider();
	else if (column == COL_DIVERGENCE)
		return m_bench.beam(row).divergence()*Units::getUnit(UnitDivergence).divider();
	else if (column == COL_NAME)
		return QString::fromUtf8(m_bench.optics(row)->name().c_str());
	else if (column == COL_LOCK)
	{
		if (m_bench.optics(row)->absoluteLock())
			return tr("absolute");
		else if (m_bench.optics(row)->relativeLockParent())
			return QString::fromUtf8(m_bench.optics(row)->relativeLockParent()->name().c_str());
		else
			return tr("none");
	}

	return QVariant();
}

QVariant GaussianBeamModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
		if (orientation == Qt::Horizontal)
		{
			switch (section)
			{
				case COL_OPTICS:
					return tr("Optics");
				case COL_POSITION:
					return tr("Position") + "\n(" + Units::getUnit(UnitPosition).prefix() + "m)";
				case COL_RELATIVE_POSITION:
					return tr("Relative\nposition");
				case COL_PROPERTIES:
					return tr("Properties");
				case COL_WAIST:
					return tr("Waist") + " (" + Units::getUnit(UnitWaist).prefix() + "m)";
				case COL_WAIST_POSITION:
					return tr("Waist\nPosition") + " (" + Units::getUnit(UnitPosition).prefix() + "m)";
				case COL_RAYLEIGH:
					return tr("Rayleigh\nlength") + " (" + Units::getUnit(UnitRayleigh).prefix() + "m)";
				case COL_DIVERGENCE:
					return tr("Divergence") + "\n(" + Units::getUnit(UnitDivergence).prefix() + "rad)";
				case COL_NAME:
					return tr("Name");
				case COL_LOCK:
					return tr("Lock");
				default:
					return QVariant();
			}
		}
		else if (orientation == Qt::Vertical)
			return section;

	return QVariant();
}

bool GaussianBeamModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid() || (role != Qt::EditRole) || value.isNull() || !value.isValid())
		return false;

	int row = index.row();
	int column = index.column();

	if (column == COL_POSITION)
		m_bench.setOpticsPosition(row, value.toDouble()*Units::getUnit(UnitPosition).multiplier());
	else if (column == COL_PROPERTIES)
	{
		Optics* optics = m_bench.opticsForPropertyChange(row);

		if (optics->type() == LensType)
			dynamic_cast<Lens*>(optics)->setFocal(value.toDouble()*Units::getUnit(UnitFocal).multiplier());
		else if (optics->type() == CurvedMirrorType)
			dynamic_cast<CurvedMirror*>(optics)->setCurvatureRadius(value.toDouble()*Units::getUnit(UnitCurvature).multiplier());
		else if (optics->type() == FlatInterfaceType)
			dynamic_cast<FlatInterface*>(optics)->setIndexRatio(value.toDouble());
		else if (optics->type() == CurvedInterfaceType)
		{
			CurvedInterface* optics = dynamic_cast<CurvedInterface*>(optics);
			optics->setSurfaceRadius(value.toList()[0].toDouble()*Units::getUnit(UnitCurvature).multiplier());
			optics->setIndexRatio(value.toList()[1].toDouble());
		}
		else if (m_bench.optics(row)->type() == GenericABCDType)
		{
			/// @todo check that the ABCD matrix is valid, e.g. by introducing bool GenericABCD::isValid()
			GenericABCD* ABCDOptics = dynamic_cast<GenericABCD*>(optics);
			ABCDOptics->setA(value.toList()[0].toDouble());
			ABCDOptics->setB(value.toList()[1].toDouble()*Units::getUnit(UnitABCD).multiplier());
			ABCDOptics->setC(value.toList()[2].toDouble()/Units::getUnit(UnitABCD).multiplier());
			ABCDOptics->setD(value.toList()[3].toDouble());
			ABCDOptics->setWidth(value.toList()[4].toDouble()*Units::getUnit(UnitWidth).multiplier());
		}
		m_bench.opticsPropertyChanged(row);
	}
	else if (column == COL_WAIST)
	{
		Beam beam = m_bench.beam(row);
		beam.setWaist(value.toDouble()*Units::getUnit(UnitWaist).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == COL_WAIST_POSITION)
	{
		Beam beam = m_bench.beam(row);
		beam.setWaistPosition(value.toDouble()*Units::getUnit(UnitPosition).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == COL_RAYLEIGH)
	{
		Beam beam = m_bench.beam(row);
		beam.setRayleigh(value.toDouble()*Units::getUnit(UnitRayleigh).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == COL_DIVERGENCE)
	{
		Beam beam = m_bench.beam(row);
		beam.setDivergence(value.toDouble()*Units::getUnit(UnitDivergence).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == COL_NAME)
		m_bench.setOpticsName(row, value.toString().toUtf8().data());
 	else if (column == COL_LOCK)
	{
		/// @todo make specific functions in OpticsBench to change this. Move this logic to OpticsBench
		QString string = value.toString();
		if (string == tr("absolute"))
		{
			Optics* optics = m_bench.opticsForPropertyChange(row);
			optics->setAbsoluteLock(true);
			m_bench.opticsPropertyChanged(row);
		}
		else if (string == tr("none"))
		{
			Optics* optics = m_bench.opticsForPropertyChange(row);
			optics->setAbsoluteLock(false);
			optics->relativeUnlock();
			m_bench.opticsPropertyChanged(row);
		}
		else
			m_bench.lockTo(row, string.toUtf8().data());
	}

	return true;
}

Qt::ItemFlags GaussianBeamModel::flags(const QModelIndex& index) const
{
	int row = index.row();
	int column = index.column();

	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

	if ((column == COL_POSITION) ||
		(column == COL_NAME) ||
		(column == COL_LOCK) ||
		(column == COL_WAIST) ||
		(column == COL_WAIST_POSITION) ||
		(column == COL_RAYLEIGH) ||
		(column == COL_DIVERGENCE) ||
		(column == COL_PROPERTIES) && (m_bench.optics(row)->type() != FlatMirrorType))
		flags |= Qt::ItemIsEditable;

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

void GaussianBeamModel::OpticsBenchDataChanged(int startOptics, int endOptics)
{
	emit dataChanged(index(startOptics, 0), index(endOptics, columnCount()-1));
}

void GaussianBeamModel::OpticsBenchOpticsAdded(int index)
{
	insertRows(index, 1);
}

void GaussianBeamModel::OpticsBenchOpticsRemoved(int index, int count)
{
	removeRows(index, count);
}

QString GaussianBeamModel::opticsName(OpticsType opticsType) const
{
	if (opticsType == CreateBeamType)
		return tr("Input beam");
	else if (opticsType == LensType)
		return tr("Lens");
	else if (opticsType == FlatMirrorType)
		return tr("Flat Mirror");
	else if (opticsType == CurvedMirrorType)
		return tr("Curved Mirror");
	else if (opticsType == FlatInterfaceType)
		return tr("Flat interface");
	else if (opticsType == CurvedInterfaceType)
		return tr("Curved interface");
	else if (opticsType == GenericABCDType)
		return tr("Generic ABCD");

	return QString();
}
