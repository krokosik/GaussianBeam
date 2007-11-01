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

GaussianBeamModel::GaussianBeamModel(QObject* parent)
	: QAbstractTableModel(parent)
	, m_cavity(1., 0., 1., 0., 0., 0.)
{
	m_first_cavity_row = 0;
	m_last_cavity_row = 0;
	m_ring_cavity = true;
	m_optics.clear();

	addOptics(new CreateBeam(180e-6, 10e-3, "w0"), rowCount());
	m_optics[0]->setAbsoluteLock(true);

	/// @todo remove this later
	m_first_cavity_row = 1;
	m_last_cavity_row = 2;

}

GaussianBeamModel::~GaussianBeamModel()
{
	for (int i = 0; i < m_optics.size(); i++)
		delete m_optics[i];
}

int GaussianBeamModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_optics.size();
}

int GaussianBeamModel::columnCount(const QModelIndex& /*parent*/) const
{
	return 10;
}

QVariant GaussianBeamModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || role != Qt::DisplayRole)
		return QVariant();

	if (index.column() == COL_OPTICS)
		return opticsName(m_optics[index.row()]->type());
	else if (index.column() == COL_POSITION)
		return m_optics[index.row()]->position()*Units::getUnit(UnitPosition).divider();
	else if ((index.column() == COL_RELATIVE_POSITION) && (index.row() > 0))
		return (m_optics[index.row()]->position() - m_optics[index.row()-1]->position())*Units::getUnit(UnitPosition).divider();
	else if (index.column() == COL_PROPERTIES)
	{
		if (m_optics[index.row()]->type() == LensType)
		{
			return QString("f = ") + QString::number(dynamic_cast<Lens*>(m_optics[index.row()])->focal()*Units::getUnit(UnitFocal).divider())
			                       + Units::getUnit(UnitFocal).string("m");
		}
		else if (m_optics[index.row()]->type() == CurvedMirrorType)
		{
			return QString("R = ") + QString::number(dynamic_cast<CurvedMirror*>(m_optics[index.row()])->curvatureRadius()*Units::getUnit(UnitCurvature).divider())
			                       + Units::getUnit(UnitCurvature).string("m");
		}
		else if (m_optics[index.row()]->type() == FlatInterfaceType)
		{
			return QString("n2/n1 = ") + QString::number(dynamic_cast<FlatInterface*>(m_optics[index.row()])->indexRatio());
		}
		else if (m_optics[index.row()]->type() == CurvedInterfaceType)
		{
			return QString("n2/n1 = ") + QString::number(dynamic_cast<CurvedInterface*>(m_optics[index.row()])->indexRatio()) +
			       QString("\nR = ") + QString::number(dynamic_cast<CurvedInterface*>(m_optics[index.row()])->surfaceRadius()*Units::getUnit(UnitCurvature).divider())
			                       + Units::getUnit(UnitCurvature).string("m");
		}
		else if (m_optics[index.row()]->type() == GenericABCDType)
		{
			return QString("A = ") + QString::number(dynamic_cast<ABCD*>(m_optics[index.row()])->A()) +
			       QString("\nB = ") + QString::number(dynamic_cast<ABCD*>(m_optics[index.row()])->B()*Units::getUnit(UnitABCD).divider())
			                         + Units::getUnit(UnitABCD).string("m") +
			       QString("\nC = ") + QString::number(dynamic_cast<ABCD*>(m_optics[index.row()])->C()/Units::getUnit(UnitABCD).divider())
			                         + " /" + Units::getUnit(UnitABCD).string("m", false) +
			       QString("\nD = ") + QString::number(dynamic_cast<ABCD*>(m_optics[index.row()])->D()) +
			       QString("\nwidth = ") + QString::number(m_optics[index.row()]->width()*Units::getUnit(UnitWidth).divider())
			                       + Units::getUnit(UnitWidth).string("m");
		}
	}
	else if (index.column() == COL_WAIST)
		return m_beams[index.row()].waist()*Units::getUnit(UnitWaist).divider();
	else if (index.column() == COL_WAIST_POSITION)
		return m_beams[index.row()].waistPosition()*Units::getUnit(UnitPosition).divider();
	else if (index.column() == COL_RAYLEIGH)
		return m_beams[index.row()].rayleigh()*Units::getUnit(UnitRayleigh).divider();
	else if (index.column() == COL_DIVERGENCE)
		return m_beams[index.row()].divergence()*Units::getUnit(UnitDivergence).divider();
	else if (index.column() == COL_NAME)
		return QString::fromUtf8(m_optics[index.row()]->name().c_str());
	else if (index.column() == COL_LOCK)
	{
		if (m_optics[index.row()]->absoluteLock())
			return tr("absolute");
		else if (m_optics[index.row()]->relativeLockParent())
			return QString::fromUtf8(m_optics[index.row()]->relativeLockParent()->name().c_str());
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

	if (index.column() == COL_POSITION)
	{
		m_optics[index.row()]->setPosition(value.toDouble()*Units::getUnit(UnitPosition).multiplier());
		computeBeams();
	}
	else if (index.column() == COL_PROPERTIES)
	{
		if (m_optics[index.row()]->type() == LensType)
			dynamic_cast<Lens*>(m_optics[index.row()])->setFocal(value.toDouble()*Units::getUnit(UnitFocal).multiplier());
		else if (m_optics[index.row()]->type() == CurvedMirrorType)
			dynamic_cast<CurvedMirror*>(m_optics[index.row()])->setCurvatureRadius(value.toDouble()*Units::getUnit(UnitCurvature).multiplier());
		else if (m_optics[index.row()]->type() == FlatInterfaceType)
			dynamic_cast<FlatInterface*>(m_optics[index.row()])->setIndexRatio(value.toDouble());
//		else if (m_optics[index.row()]->type() == CurvedInterfaceType)
//			dynamic_cast<CurvedInterface*>(m_optics[index.row()])->setSurfaceRadius(value.toDouble()*Units::getUnit(UnitCurvature).multiplier());
		else if (m_optics[index.row()]->type() == CurvedInterfaceType)
		{
			CurvedInterface* optics = dynamic_cast<CurvedInterface*>(m_optics[index.row()]);
			optics->setSurfaceRadius(value.toList()[0].toDouble()*Units::getUnit(UnitCurvature).multiplier());
			optics->setIndexRatio(value.toList()[1].toDouble());
		}
		else if (m_optics[index.row()]->type() == GenericABCDType)
		{
			/// @todo check that the ABCD matrix is valid, e.g. by introducing bool GenericABCD::isValid()
			GenericABCD* ABCDOptics = dynamic_cast<GenericABCD*>(m_optics[index.row()]);
			ABCDOptics->setA(value.toList()[0].toDouble());
			ABCDOptics->setB(value.toList()[1].toDouble()*Units::getUnit(UnitABCD).multiplier());
			ABCDOptics->setC(value.toList()[2].toDouble()/Units::getUnit(UnitABCD).multiplier());
			ABCDOptics->setD(value.toList()[3].toDouble());
			ABCDOptics->setWidth(value.toList()[4].toDouble()*Units::getUnit(UnitWidth).multiplier());
		}
		computeBeams();
	}
	else if (index.column() == COL_WAIST)
	{
		m_beams[index.row()].setWaist(value.toDouble()*Units::getUnit(UnitWaist).multiplier());
		computeBeams(index.row(), true);
	}
	else if (index.column() == COL_WAIST_POSITION)
	{
		m_beams[index.row()].setWaistPosition(value.toDouble()*Units::getUnit(UnitPosition).multiplier());
		computeBeams(index.row(), true);
	}
	else if (index.column() == COL_RAYLEIGH)
	{
		m_beams[index.row()].setRayleigh(value.toDouble()*Units::getUnit(UnitRayleigh).multiplier());
		computeBeams(index.row(), true);
	}
	else if (index.column() == COL_DIVERGENCE)
	{
		m_beams[index.row()].setDivergence(value.toDouble()*Units::getUnit(UnitDivergence).multiplier());
		computeBeams(index.row(), true);
	}
	else if (index.column() == COL_NAME)
	{
		std::string name = value.toString().toUtf8().data();
		for (int i = 0; i < rowCount(); i++)
			if (m_optics[i]->name() == name)
				return false;
		m_optics[index.row()]->setName(name);
	}
 	else if (index.column() == COL_LOCK)
	{
		QString string = value.toString();

		qDebug() << "Model : setData : lock :" << string;

		if (string == tr("absolute"))
			m_optics[index.row()]->setAbsoluteLock(true);
		else if (string == tr("none"))
		{
			m_optics[index.row()]->setAbsoluteLock(false);
			m_optics[index.row()]->relativeUnlock();
		}
		else
		{
			for (int i = 0; i < rowCount(); i++)
				if (QString::fromUtf8(m_optics[i]->name().c_str()) == string)
				{
					m_optics[index.row()]->relativeLockTo(m_optics[i]);
					break;
				}
		}
	}

	return true;
}

Qt::ItemFlags GaussianBeamModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

	if ((index.column() == COL_POSITION) ||
		(index.column() == COL_NAME) ||
		(index.column() == COL_LOCK) ||
		(index.column() == COL_WAIST) ||
		(index.column() == COL_WAIST_POSITION) ||
		(index.column() == COL_RAYLEIGH) ||
		(index.column() == COL_DIVERGENCE) ||
		(index.column() == COL_PROPERTIES) && (m_optics[index.row()]->type() != FlatMirrorType))
		flags |= Qt::ItemIsEditable;

	return flags;
}

bool GaussianBeamModel::insertRows(int row, int count, const QModelIndex& parent)
{
	beginInsertRows(parent, row, row + count -1);
	for (int i = row; i < row + count; i++)
	{
		m_optics.insert(m_optics.begin() + row,  0); /// @todo is this 0 harmfull ?
		m_beams.insert(m_beams.begin() + row, Beam());
	}
	endInsertRows();
	return true;
}

bool GaussianBeamModel::removeRows(int row, int count, const QModelIndex& parent)
{
	beginRemoveRows(parent, row, row + count -1);
	for (int i = row + count - 1; i >= row; i--)
	{
		qDebug() << "deleting optics" << i;
		delete m_optics[i];
		qDebug() << "removing optics from list " << i;
		m_optics.removeAt(i);
		m_beams.removeAt(i);
	}
	computeBeams(row);
	endRemoveRows();
	return true;
}

//////////////////////////////////////////
// Cavity stuff
bool GaussianBeamModel::isCavityStable() const
{
	if (m_first_cavity_row == 0)
		return false;

	if (m_cavity.stabilityCriterion1())
	{
		if (m_cavity.stabilityCriterion2())
			return true;
		else
			qDebug() << "Cavity stable for 1 and not 2 !!!!";
	}
	else if (m_cavity.stabilityCriterion2())
		qDebug() << "Cavity stable for 2 and not 1 !!!!";

	return false;
}

const Beam GaussianBeamModel::cavityEigenBeam(int row) const
{
	if (!isCavityStable() ||
	   (row < m_first_cavity_row) ||
	   (m_ring_cavity && (row >= m_last_cavity_row)) ||
	   (!m_ring_cavity && (row > m_last_cavity_row)))
		return Beam();

	Beam beam = m_cavity.eigenMode(m_wavelength);

	for (int i = m_first_cavity_row; i <= row; i++)
		beam = optics(i).image(beam);

	return beam;
}

//////////////////////////////////////////

void GaussianBeamModel::setWavelength(double wavelength)
{
	m_wavelength = wavelength;
	computeBeams();
}

void GaussianBeamModel::addOptics(Optics* optics, int row)
{
	insertRow(row);
	m_optics[row] = optics;
	computeBeams(row);
}

void GaussianBeamModel::setOpticsPosition(int row, double position)
{
	m_optics[row]->setPositionCheckLock(position);
	computeBeams();
}

void GaussianBeamModel::setInputBeam(const Beam& beam, bool update)
{
	CreateBeam* createBeam = dynamic_cast<CreateBeam*>(m_optics[0]);
	createBeam->setPosition(beam.waistPosition());
	createBeam->setWaist(beam.waist());
	if (update)
		computeBeams();
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

void GaussianBeamModel::computeBeams(int changedRow, bool backward)
{
	qDebug() << "computeBeams" << m_wavelength;

	Beam beam;

	if (backward)
	{
		beam = m_beams[changedRow];
		for (int row = changedRow + 1; row < rowCount(); row++)
			m_beams[row] = beam = m_optics[row]->image(beam);
		beam = m_beams[changedRow];
		for (int row = changedRow - 1; row >= 0; row--)
			m_beams[row] = beam = m_optics[row + 1]->antecedent(beam);
		setInputBeam(beam, false);
		emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));
	}
	else
	{
		if (changedRow == 0)
			beam.setWavelength(m_wavelength);
		else
			beam = m_beams[changedRow - 1];

		for (int row = changedRow; row < rowCount(); row++)
			m_beams[row] = beam = m_optics[row]->image(beam);
		emit dataChanged(index(changedRow, 0), index(rowCount()-1, columnCount()-1));
	}
/*
	// Compute the cavity
	if (m_first_cavity_row > 0)
	{
		const Optics& first_cavity_optics = optics(m_first_cavity_row);
		const Optics& last_cavity_optics  = optics(m_last_cavity_row);

		// Test coherence
		if (!first_cavity_optics.isABCD() || ! last_cavity_optics.isABCD())
			qDebug() << "Warning : cavity boundaries are not of ABCD type";
		if (!(m_last_cavity_row > 0) || !(m_last_cavity_row >= m_first_cavity_row))
			qDebug() << "Warning m_last_cavity_row seems to be wrong !";

		// Compute cavity
		m_cavity = dynamic_cast<const ABCD&>(first_cavity_optics);
		for (int i = m_first_cavity_row + 1; i < m_last_cavity_row; i++)
		{
			if (optics(i).isABCD())
				m_cavity *= dynamic_cast<const ABCD&>(optics(i));
			FreeSpace freeSpace(optics(i+1).position() - optics(i).endPosition(), optics(i).endPosition());
			m_cavity*= freeSpace;
		}
		if (m_ring_cavity)
			for (int i = m_last_cavity_row; i > m_first_cavity_row; i--)
			{
				if (optics(i).isABCD())
					m_cavity *= dynamic_cast<const ABCD&>(optics(i));
				FreeSpace freeSpace(optics(i).endPosition() - optics(i-1).endPosition(), optics(i-1).endPosition());
				m_cavity*= freeSpace;
			}
		else
		{
			m_cavity *= dynamic_cast<const ABCD&>(last_cavity_optics);
			/// @todo add a user defined free space between the last and the first optics for linear cavities.
		}

	}*/
}
