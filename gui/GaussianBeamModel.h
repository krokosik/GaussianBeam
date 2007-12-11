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

#include "src/OpticsBench.h"
#include "src/GaussianBeam.h"
#include "src/Optics.h"

#include <QAbstractTableModel>
#include <QList>

enum ColumnContent {OpticsColumn, PositionColumn, RelativePositionColumn, PropertiesColumn,
                    WaistColumn, WaistPositionColumn, RayleighColumn, DivergenceColumn,
                    NameColumn, LockColumn, SensitivityColumn};

class GaussianBeamModel : public QAbstractTableModel, private OpticsBenchNotify
{
	Q_OBJECT

public:
	GaussianBeamModel(OpticsBench& bench, QObject* parent = 0);
	~GaussianBeamModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex()) const;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags ( const QModelIndex & index ) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
	ColumnContent columnContent(int column) const { return m_columns[column]; }

private:
	QString opticsName(OpticsType opticsType) const;
	void OpticsBenchDataChanged(int startOptics, int endOptics);
	void OpticsBenchOpticsAdded(int index);
	void OpticsBenchOpticsRemoved(int index, int count);

private:
	QList<ColumnContent> m_columns;
};

#endif
