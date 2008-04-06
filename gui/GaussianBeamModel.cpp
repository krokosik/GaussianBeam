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

#include "gui/GaussianBeamModel.h"
#include "gui/Unit.h"

#include <QtGui>
#include <QDebug>

GaussianBeamModel::GaussianBeamModel(OpticsBench& bench, QObject* parent)
	: QAbstractTableModel(parent)
	, OpticsBenchNotify(bench)

{
	m_columns << OpticsColumn << PositionColumn << RelativePositionColumn << PropertiesColumn
	          << WaistColumn << WaistPositionColumn << RayleighColumn << DivergenceColumn
	          << SensitivityColumn << NameColumn << LockColumn;

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
	return m_columns.size();
}

QVariant GaussianBeamModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || role != Qt::DisplayRole)
		return QVariant();

	int row = index.row();
	ColumnContent column = m_columns[index.column()];

	if (column == OpticsColumn)
		return opticsName(m_bench.optics(row)->type());
	else if (column == PositionColumn)
		return m_bench.optics(row)->position()*Units::getUnit(UnitPosition).divider();
	else if ((column == RelativePositionColumn) && (row > 0))
		return (m_bench.optics(row)->position() - m_bench.optics(row-1)->position())*Units::getUnit(UnitPosition).divider();
	else if (column == PropertiesColumn)
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
		else if (m_bench.optics(row)->type() == DielectricSlabType)
		{
			const DielectricSlab* optics = dynamic_cast<const DielectricSlab*>(m_bench.optics(row));
			return QString("n2/n1 = ") + QString::number(optics->indexRatio()) +
			       QString("\n") + tr("width") + " = " + QString::number(m_bench.optics(row)->width()*Units::getUnit(UnitWidth).divider())
			                       + Units::getUnit(UnitWidth).string("m");
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
			       QString("\n") + tr("width") + " = " + QString::number(m_bench.optics(row)->width()*Units::getUnit(UnitWidth).divider())
			                       + Units::getUnit(UnitWidth).string("m");
		}
	}
	else if (column == WaistColumn)
		return m_bench.beam(row).waist()*Units::getUnit(UnitWaist).divider();
	else if (column == WaistPositionColumn)
		return m_bench.beam(row).waistPosition()*Units::getUnit(UnitPosition).divider();
	else if (column == RayleighColumn)
		return m_bench.beam(row).rayleigh()*Units::getUnit(UnitRayleigh).divider();
	else if (column == DivergenceColumn)
		return m_bench.beam(row).divergence()*Units::getUnit(UnitDivergence).divider();
	else if (column == SensitivityColumn)
		return fabs(m_bench.sensitivity(row))*100./sqr(Units::getUnit(UnitPosition).divider());
		//return QString::number(fabs(m_bench.sensitivity(row))*100./sqr(Units::getUnit(UnitPosition).divider()), 'f', 2);
	else if (column == NameColumn)
		return QString::fromUtf8(m_bench.optics(row)->name().c_str());
	else if (column == LockColumn)
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
			switch (m_columns[section])
			{
				case OpticsColumn:
					return tr("Optics");
				case PositionColumn:
					return tr("Position") + "\n(" + Units::getUnit(UnitPosition).prefix() + "m)";
				case RelativePositionColumn:
					return tr("Relative\nposition");
				case PropertiesColumn:
					return tr("Properties");
				case WaistColumn:
					return tr("Waist") + " (" + Units::getUnit(UnitWaist).prefix() + "m)";
				case WaistPositionColumn:
					return tr("Waist\nPosition") + " (" + Units::getUnit(UnitPosition).prefix() + "m)";
				case RayleighColumn:
					return tr("Rayleigh\nlength") + " (" + Units::getUnit(UnitRayleigh).prefix() + "m)";
				case DivergenceColumn:
					return tr("Divergence") + "\n(" + Units::getUnit(UnitDivergence).prefix() + "rad)";
				case SensitivityColumn:
					return tr("Sensitivity") + "\n(%/" + Units::getUnit(UnitPosition).prefix() + tr("m²") + ")";
				case NameColumn:
					return tr("Name");
				case LockColumn:
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
	ColumnContent column = m_columns[index.column()];

	if (column == PositionColumn)
		m_bench.setOpticsPosition(row, value.toDouble()*Units::getUnit(UnitPosition).multiplier(), row != 0);
	else if (column == RelativePositionColumn)
	{
		if (row == 0)
			return false;

		const Optics* optics = m_bench.optics(row);
		const Optics* preceedingOptics = m_bench.optics(row-1);
		if (optics->relativeLockedTo(preceedingOptics))
			return false;
		m_bench.setOpticsPosition(row, value.toDouble()*Units::getUnit(UnitPosition).multiplier() + preceedingOptics->position());
	}
	else if (column == PropertiesColumn)
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
			qDebug() << "setData for CurvedInterfaceType" << value.toList()[0].toDouble() << value.toList()[1].toDouble();
			CurvedInterface* curvedInterfaceOptics = dynamic_cast<CurvedInterface*>(optics);
			curvedInterfaceOptics->setSurfaceRadius(value.toList()[0].toDouble()*Units::getUnit(UnitCurvature).multiplier());
			curvedInterfaceOptics->setIndexRatio(value.toList()[1].toDouble());
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
	else if (column == WaistColumn)
	{
		Beam beam = m_bench.beam(row);
		beam.setWaist(value.toDouble()*Units::getUnit(UnitWaist).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == WaistPositionColumn)
	{
		Beam beam = m_bench.beam(row);
		beam.setWaistPosition(value.toDouble()*Units::getUnit(UnitPosition).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == RayleighColumn)
	{
		Beam beam = m_bench.beam(row);
		beam.setRayleigh(value.toDouble()*Units::getUnit(UnitRayleigh).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == DivergenceColumn)
	{
		Beam beam = m_bench.beam(row);
		beam.setDivergence(value.toDouble()*Units::getUnit(UnitDivergence).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == NameColumn)
		m_bench.setOpticsName(row, value.toString().toUtf8().data());
 	else if (column == LockColumn)
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
	ColumnContent column = m_columns[index.column()];

	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

	if ((column == NameColumn) ||
		(column == LockColumn) ||
		(column == WaistColumn) ||
		(column == WaistPositionColumn) ||
		(column == RayleighColumn) ||
		(column == DivergenceColumn) ||
		(column == PropertiesColumn)
		 && (m_bench.optics(row)->type() != FlatMirrorType)
		 && (m_bench.optics(row)->type() != CreateBeamType))
			flags |= Qt::ItemIsEditable;

	if (column == PositionColumn)
	{
		if (row == 0)
			flags |= Qt::ItemIsEditable;

		const Optics* optics = m_bench.optics(row);
		if (!optics->relativeLockTreeAbsoluteLock())
			flags |= Qt::ItemIsEditable;
	}

	if ((column == RelativePositionColumn) && (row != 0))
	{
		const Optics* optics = m_bench.optics(row);
		const Optics* preceedingOptics = m_bench.optics(row-1);
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

void GaussianBeamModel::OpticsBenchDataChanged(int startOptics, int endOptics)
{
	emit dataChanged(index(startOptics, 0), index(endOptics, columnCount()-1));
}

void GaussianBeamModel::OpticsBenchOpticsAdded(int index)
{
	qDebug() << "GaussianBeamModel::OpticsBenchOpticsAdded";
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
	else if (opticsType == ThickLensType)
		return tr("Thick lens");
	else if (opticsType == FlatMirrorType)
		return tr("Flat Mirror");
	else if (opticsType == CurvedMirrorType)
		return tr("Curved Mirror");
	else if (opticsType == FlatInterfaceType)
		return tr("Flat interface");
	else if (opticsType == CurvedInterfaceType)
		return tr("Curved interface");
	else if (opticsType == DielectricSlabType)
		return tr("Dielectric slab");
	else if (opticsType == ThermalLensType)
		return tr("Thermal lens");
	else if (opticsType == GenericABCDType)
		return tr("Generic ABCD");

	return QString();
}
