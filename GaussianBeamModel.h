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

#ifndef GAUSSIANBEAMMODEL_H
#define GAUSSIANBEAMMODEL_H

#define COL_OPTICS 0
#define COL_POSITION 1
#define COL_RELATIVE_POSITION 2
#define COL_PROPERTIES 3
#define COL_WAIST 4
#define COL_WAIST_POSITION 5
#define COL_RAYLEIGH 6
#define COL_DIVERGENCE 7
#define COL_NAME 8
#define COL_LOCK 9

#include "GaussianBeam.h"

#include <QAbstractTableModel>
#include <QList>

class GaussianBeamModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	GaussianBeamModel(QObject* parent = 0);
	~GaussianBeamModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex()) const;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags ( const QModelIndex & index ) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());

public:
	const Optics& optics(int row) const { return *(m_optics[row]); }
	const Optics* opticsPtr(int row) const { return m_optics[row]; }
	const Optics& optics(const QModelIndex& index) const { return optics(index.row()); }
	const Beam& beam(int row) const { return m_beams[row]; }
	double wavelength() { return m_wavelength; }
	void setWavelength(double wavelength);
	/**
	* Adds a new optics
	* @p optics pointer to the optics to add. GaussianBeamModel takes ownership on the pointer
	* @p row the given optics will be inserted before this row
	*/
	void addOptics(Optics* optics, int row);
	void setOpticsPosition(int row, double position);
	void setInputBeam(const Beam& beam) { setInputBeam(beam, true); }

/// Cavity stuff
public:
	bool isCavityStable() const;
	const Beam cavityEigenBeam(int row) const;

private:
	QString opticsName(OpticsType opticsType) const;
	void computeBeams(int changedRow = 0, bool backward = false);
	void setInputBeam(const Beam& beam, bool update);

private:
	QList<Optics*> m_optics;
	QList<Beam> m_beams;

	double m_wavelength;

	GenericABCD m_cavity;
	int m_first_cavity_row;
	int m_last_cavity_row;
	bool m_ring_cavity;
};

#endif
