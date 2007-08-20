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

#ifndef OPTICSVIEW_H
#define OPTICSVIEW_H

#include <QAbstractItemView>
#include <QPoint>
#include <QPainterPath>

class QLabel;
class QAbstractItemModel;
class QComboBox;

class OpticsView : public QAbstractItemView
{
	Q_OBJECT

public:
	OpticsView(QWidget* parent = 0);

	QRect visualRect(const QModelIndex &index) const;
	void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
	QModelIndex indexAt(const QPoint &point) const;

	void setHRange(double hRange);
	void setVRange(double vRange);
	void setHOffset(double hOffset);
	void setWavelength(double wavelength) { m_wavelength = wavelength; }
	void setStatusLabel(QLabel* statusLabel) { m_statusLabel = statusLabel; }
	void setFitModel(QAbstractItemModel* fitModel) { m_fitModel = fitModel; }
	void setMeasureCombo(QComboBox* measureCombo) { m_measureCombo = measureCombo; }
	void updateViewport() { viewport()->update(); }

protected slots:
	void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
//	void rowsInserted(const QModelIndex& parent, int start, int end);
//	void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);

protected:
	QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction,
							Qt::KeyboardModifiers modifiers);
	int horizontalOffset() const;
	int verticalOffset() const;
	bool isIndexHidden(const QModelIndex& index) const;
	void setSelection(const QRect&, QItemSelectionModel::SelectionFlags command);
	QRegion visualRegionForSelection(const QItemSelection &selection) const;

	void mousePressEvent(QMouseEvent* event);
	void paintEvent(QPaintEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void resizeEvent(QResizeEvent* event);

private:
	void computeTranformMatrix();
	void computePaths();
	double vScale() const;
	double hScale() const;

	QRectF objectRect() const;

private:
	QLabel* m_statusLabel;
	QAbstractItemModel* m_fitModel;
	QComboBox* m_measureCombo;

	QPainterPath m_convexLensPath;
	QPainterPath m_concaveLensPath;
	QPainterPath m_flatInterfacePath;
	QPainterPath m_convexInterfacePath;
	QPainterPath m_concaveInterfacePath;

	double m_wavelength;
	double m_hRange;
	double m_vRange;
	double m_hOffset;
	QMatrix m_abs2view;
	QMatrix m_view2abs;

	int m_active_object;
};

#endif
