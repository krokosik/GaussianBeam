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

#include "src/GaussianBeam.h"
#include "src/OpticsBench.h"

#include <QPoint>
#include <QPainterPath>

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QStatusBar>

class QAbstractItemModel;
class QComboBox;

class OpticsItem;
class BeamItem;
class RullerSlider;

class OpticsScene : public QGraphicsScene, private OpticsBenchNotify
{
public:
	OpticsScene(OpticsBench& bench, QObject* parent = 0);

public:
	void showTargetBeam(bool show = true);

private:
	void OpticsBenchDataChanged(int startOptics, int endOptics);
	void OpticsBenchOpticsAdded(int index);
	void OpticsBenchOpticsRemoved(int index, int count);

private:
	QList<BeamItem*> m_beamItems;
	BeamItem* m_targetBeamItem;
};

class OpticsView : public QGraphicsView
{
	Q_OBJECT

public:
	OpticsView(QGraphicsScene* scene);

public:
	void setStatusBar(QStatusBar* statusBar) { m_statusBar = statusBar; }
	double verticalRange() { return m_verticalRange; }
	void setVerticalRange(double verticalRange);
	double horizontalRange() { return m_horizontalRange; }
	void setHorizontalRange(double horizontalRange);

/// Inherited protected functions
protected:
	virtual void drawBackground(QPainter* painter, const QRectF& rect);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* e);
	virtual void showEvent(QShowEvent* event);

private:
	void adjustRange();

private:
	RullerSlider* m_horizontalRuller;
	QStatusBar* m_statusBar;
	double m_horizontalRange;
	double m_verticalRange;
};

/// @todo don't forget prepareGeometryChange()
class OpticsItem : public QGraphicsItem
{
public:
	OpticsItem(const Optics* optics, OpticsBench& bench);

/// Inherited public functions
public:
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

/// Inherited protected functions
protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant& value);

public:
	void setUpdate(bool update) { m_update = update; }
	const Optics* optics() const { return m_optics; }
	void setOptics(const Optics* optics) { m_optics = optics; }
	void adjustScale(double horizontalScale, double verticalScale);

private:
	const Optics* m_optics;
	bool m_update;
	OpticsBench& m_bench;

};

class BeamItem : public QGraphicsItem
{
public:
	BeamItem(const Beam& beam);

/// Inherited public functions
public:
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

public:
	double leftBound() const { return m_leftBound; }
	double rightBound() const { return m_rightBound; }
	void setLeftBound(double leftBound);
	void setRightBound(double rightBound);
	const Beam& beam() const { return m_beam; }

private:
	const Beam& m_beam;
	double m_leftBound;
	double m_rightBound;
	bool m_drawText;
};

#include <QAbstractItemView>

class OpticsItemView : public QAbstractItemView
{
	Q_OBJECT

public:
	OpticsItemView(OpticsBench& bench, QWidget* parent = 0);

	QRect visualRect(const QModelIndex &index) const;
	void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
	QModelIndex indexAt(const QPoint &point) const;

	void setHRange(double hRange);
	void setVRange(double vRange);
	void setHOffset(double hOffset);
	void setStatusBar(QStatusBar* statusBar) { m_statusBar = statusBar; }
	void setFitModel(QAbstractItemModel* fitModel) { m_fitModel = fitModel; }
	void setMeasureCombo(QComboBox* measureCombo) { m_measureCombo = measureCombo; }
	void setShowTargetBeam(bool showTargetBeam);
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

	void drawBeam(QPainter& painter, const Beam& beam, const QRectF& abs_beamRange, bool drawText = false);

private:
	QStatusBar* m_statusBar;
	/// @todo this should remove
	QAbstractItemModel* m_fitModel;
	QComboBox* m_measureCombo;
	OpticsBench& m_bench;

	QPainterPath m_convexLensPath;
	QPainterPath m_concaveLensPath;
	QPainterPath m_flatInterfacePath;
	QPainterPath m_convexInterfacePath;
	QPainterPath m_concaveInterfacePath;

	bool m_showTargetBeam;
	double m_hRange;
	double m_vRange;
	double m_hOffset;
	QMatrix m_abs2view;
	QMatrix m_view2abs;

	int m_active_object;
	double m_active_object_offset;
};

#endif
