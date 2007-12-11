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

#ifndef GAUSSIANBEAMDELEGATE_H
#define GAUSSIANBEAMDELEGATE_H

#include "src/OpticsBench.h"

#include <QItemDelegate>
#include <QModelIndex>
#include <QWidget>
#include <QDoubleSpinBox>

class GaussianBeamModel;

class ABCDWidget : public QWidget
{
	Q_OBJECT

public:
	ABCDWidget(QWidget* parent = 0);

public:
	double A() { return m_ADoubleSpinBox.value(); }
	double B() { return m_BDoubleSpinBox.value(); }
	double C() { return m_CDoubleSpinBox.value(); }
	double D() { return m_DDoubleSpinBox.value(); }
	double width() { return m_widthDoubleSpinBox.value(); }
	void setA(double A) { m_ADoubleSpinBox.setValue(A); }
	void setB(double B) { m_BDoubleSpinBox.setValue(B); }
	void setC(double C) { m_CDoubleSpinBox.setValue(C); }
	void setD(double D) { m_DDoubleSpinBox.setValue(D); }
	void setWidth(double width) { m_widthDoubleSpinBox.setValue(width); };

private:
	QDoubleSpinBox m_ADoubleSpinBox, m_BDoubleSpinBox, m_CDoubleSpinBox, m_DDoubleSpinBox;
	QDoubleSpinBox m_widthDoubleSpinBox;
};

class CurvedInterfaceWidget : public QWidget
{
	Q_OBJECT

public:
	CurvedInterfaceWidget(QWidget* parent = 0);

public:
	double surfaceRadius() { return m_surfaceRadiusDoubleSpinBox.value(); }
	double indexRatio() { return m_indexRatioDoubleSpinBox.value(); }
	void setSurfaceRadius(double surfaceRadius) { m_surfaceRadiusDoubleSpinBox.setValue(surfaceRadius); }
	void setIndexRatio(double indexRatio) { m_indexRatioDoubleSpinBox.setValue(indexRatio); }

private:
	QDoubleSpinBox m_surfaceRadiusDoubleSpinBox, m_indexRatioDoubleSpinBox;
};

class GaussianBeamDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	GaussianBeamDelegate(QObject* parent, GaussianBeamModel* model, OpticsBench& bench);

public:
	QWidget *createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;

	void setEditorData(QWidget* editor, const QModelIndex& index) const;
	void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
	void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
	GaussianBeamModel* m_model;
	const OpticsBench& m_bench;
};

#endif
