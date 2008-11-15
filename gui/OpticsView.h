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

#ifndef OPTICSVIEW_H
#define OPTICSVIEW_H

#include "src/GaussianBeam.h"

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
class OpticsViewProperties;
class StatusWidget;

class OpticsBench;
class Optics;

class OpticsScene : public QGraphicsScene
{
Q_OBJECT

public:
	OpticsScene(OpticsBench* bench, QObject* parent = 0);

public:
	void showTargetBeam(bool show = true);
	bool targetBeamVisible();

private slots:
	void onOpticsBenchDataChanged(int startOptics, int endOptics);
	void onOpticsBenchTargetBeamChanged();
	void onOpticsBenchBoundariesChanged();
	void onOpticsBenchOpticsAdded(int index);
	void onOpticsBenchOpticsRemoved(int index, int count);
	void onOpticsBenchFitDataChanged(int index);
	void onOpticsBenchFitsRemoved(int index, int count);

private:
	void addFitPoint(double position, double radius, QRgb color);

private:
	OpticsBench* m_bench;

	QList<BeamItem*> m_beamItems;
	BeamItem* m_targetBeamItem;
	BeamItem* m_cavityBeamItem;
	QList<QGraphicsEllipseItem*> m_fitItems;
};

class OpticsView : public QGraphicsView
{
Q_OBJECT

public:
	OpticsView(QGraphicsScene* scene);

public:
	void setStatusWidget(StatusWidget* statusWidget) { m_statusWidget = statusWidget; }
	double verticalRange() { return m_verticalRange; }
	void setVerticalRange(double verticalRange);
	double horizontalRange() { return m_horizontalRange; }
	void setHorizontalRange(double horizontalRange);
	double origin();
	void setOrigin(double origin);
	void showProperties(bool show = true);
	bool propertiesVisible();

/// Inherited protected functions
protected:
	virtual void resizeEvent(QResizeEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* e);
	virtual void drawBackground(QPainter* painter, const QRectF& rect);

private:
	void adjustRange();

private slots:
	void scrollUpdated(int value);

private:
	OpticsViewProperties* m_opticsViewProperties;
	RullerSlider* m_horizontalRuller;
	RullerSlider* m_verticalRuller;
	StatusWidget* m_statusWidget;
	double m_horizontalRange;
	double m_verticalRange;

friend class OpticsScene;
};

/// @todo don't forget prepareGeometryChange()
class OpticsItem : public QGraphicsItem
{
public:
	OpticsItem(const Optics* optics, OpticsBench* bench);

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

private:
	const Optics* m_optics;
	bool m_update;
	OpticsBench* m_bench;
};

class BeamItem : public QGraphicsItem
{
public:
	BeamItem(const Beam* beam);

/// Inherited public functions
public:
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

public:
	void updateTransform();
	const Beam* beam() const { return m_beam; }
	void setPlainStyle(bool style = true) { m_style = style; }
	bool auxiliary() const { return m_auxiliary; }
	void setAuxiliary(bool auxiliary) { m_auxiliary = auxiliary; }

private:
	void drawSegment(double start, double stop, double pixel, int nStep, QPolygonF& polygon) const;

private:
	const Beam* m_beam;
	bool m_drawText;
	bool m_style;
	bool m_auxiliary;
};

#endif
