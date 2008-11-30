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

#include "gui/OpticsView.h"
#include "gui/OpticsWidgets.h"
#include "gui/GaussianBeamModel.h"
#include "gui/Unit.h"
#include "src/GaussianBeam.h"
#include "src/Utils.h"
#include "src/OpticsBench.h"

#include <QtGui>
#include <QtDebug>
#include <QtGlobal>

#include <cmath>

QColor wavelengthColor(double wavelength)
{
	wavelength *= 1e9;

	QColor color = Qt::black;

	// Wavelength to rgb conversion
	if (wavelength < 380.)
		color = QColor::fromRgbF(1., 0., 1.);
	else if (wavelength < 440.)
		color = QColor::fromRgbF((440.-wavelength)/60., 0., 1.);
	else if (wavelength < 490.)
		color = QColor::fromRgbF(0., (wavelength-440.)/50., 1.);
	else if (wavelength < 510.)
		color = QColor::fromRgbF(0., 1. , (510.-wavelength)/40.);
	else if (wavelength < 580.)
		color = QColor::fromRgbF((wavelength-510.)/70., 1., 0.);
	else if (wavelength < 645.)
		color = QColor::fromRgbF(1., (645.-wavelength)/85., 0.);
	else
		color = QColor::fromRgbF(1., 0., 0.);

	// Attenuation
	double attenuation = 1.;
	double minAttenuation = 0.37;
	if (wavelength > 700.)
		attenuation = qMax(minAttenuation, .3 + .7*(780.-wavelength)/80.);
	else if (wavelength < 420.)
		attenuation = qMax(minAttenuation, .3 + .7*(wavelength-380.)/40.);

	// Gamma
	double gamma = 0.8;
	color.setRedF  (pow(color.redF  ()*attenuation, gamma));
	color.setGreenF(pow(color.greenF()*attenuation, gamma));
	color.setBlueF (pow(color.blueF ()*attenuation, gamma));

	return color;
}

/////////////////////////////////////////////////
// OpticsScene class

OpticsScene::OpticsScene(OpticsBench* bench, QObject* parent)
	: QGraphicsScene(parent)
{
	// Bench connections
	m_bench = bench;
	connect(m_bench, SIGNAL(dataChanged(int, int)),   this, SLOT(onOpticsBenchDataChanged(int, int)));
	connect(m_bench, SIGNAL(targetBeamChanged()),     this, SLOT(onOpticsBenchTargetBeamChanged()));
	connect(m_bench, SIGNAL(boundariesChanged()),     this, SLOT(onOpticsBenchBoundariesChanged()));
	connect(m_bench, SIGNAL(opticsAdded(int)),        this, SLOT(onOpticsBenchOpticsAdded(int)));
	connect(m_bench, SIGNAL(opticsRemoved(int, int)), this, SLOT(onOpticsBenchOpticsRemoved(int, int)));
	connect(m_bench, SIGNAL(fitsRemoved(int, int)),   this, SLOT(onOpticsBenchFitsRemoved(int, int)));
	connect(m_bench, SIGNAL(fitDataChanged(int)),     this, SLOT(onOpticsBenchFitDataChanged(int)));

	setItemIndexMethod(QGraphicsScene::NoIndex);

	m_targetBeamItem = new BeamItem(m_bench->targetBeam());
	m_targetBeamItem->setPlainStyle(false);
	m_targetBeamItem->setAuxiliary(true);
	m_targetBeamItem->setPos(0., 0.);
	addItem(m_targetBeamItem);

	m_cavityBeamItem = new BeamItem(m_bench->cavity().eigenBeam(m_bench->wavelength(), 0));
	m_cavityBeamItem->setPlainStyle(false);
	m_cavityBeamItem->setAuxiliary(true);
	m_cavityBeamItem->setPos(0., 0.);
	m_cavityBeamItem->setVisible(true);
	addItem(m_cavityBeamItem);

	// Sync with bench
	for (int i = 0; i < m_bench->nOptics(); i++)
		onOpticsBenchOpticsAdded(i);
	onOpticsBenchBoundariesChanged();
}

void OpticsScene::showTargetBeam(bool show)
{
	m_targetBeamItem->setVisible(show);
}

bool OpticsScene::targetBeamVisible()
{
	return m_targetBeamItem->isVisible();
}

/// @todo get rid of this trick
#define SCENEHALFHEIGHT 0.005

void OpticsScene::onOpticsBenchDataChanged(int startOptics, int endOptics)
{
	foreach (QGraphicsItem* graphicsItem, items())
	{
		if (OpticsItem* opticsItem = dynamic_cast<OpticsItem*>(graphicsItem))
		{
			int opticsIndex = m_bench->opticsIndex(opticsItem->optics());
			if ((opticsIndex >= startOptics) && (opticsIndex <= endOptics))
			{
				opticsItem->setUpdate(false);
				double position = opticsItem->optics()->position();
				const Beam* axis = m_bench->axis(opticsIndex);
				opticsItem->setPos(axis->origin()[0] + position*cos(axis->angle()), axis->origin()[1] - position*sin(axis->angle()));
				opticsItem->resetTransform();
				opticsItem->rotate(-(axis->angle() + opticsItem->optics()->angle())*180./M_PI);
				opticsItem->setUpdate(true);
			}
		}
	}

	for (int i = qMax(0, startOptics-1); i <= endOptics; i++)
		m_beamItems[i]->updateTransform();
}

void OpticsScene::onOpticsBenchTargetBeamChanged()
{
	m_targetBeamItem->update();
}

void OpticsScene::onOpticsBenchBoundariesChanged()
{
	setSceneRect(m_bench->leftBoundary(), -SCENEHALFHEIGHT, m_bench->rightBoundary() - m_bench->leftBoundary(), 2.*SCENEHALFHEIGHT);

	m_targetBeamItem->updateTransform();
	m_cavityBeamItem->updateTransform();

	if ((m_beamItems.size() != 0) && (m_beamItems.size() == m_bench->nOptics()))
	{
		m_beamItems[0]->updateTransform();
		m_beamItems[m_bench->nOptics()-1]->updateTransform();
	}

	foreach (QGraphicsView* view, views())
		dynamic_cast<OpticsView*>(view)->adjustRange();
}

void OpticsScene::onOpticsBenchOpticsAdded(int index)
{
	OpticsItem* opticsItem = new OpticsItem(m_bench->optics(index), m_bench);
	addItem(opticsItem);

	BeamItem* beamItem = new BeamItem(m_bench->beam(index));
	m_beamItems.insert(index, beamItem);
	beamItem->updateTransform();
	addItem(beamItem);
}

void OpticsScene::onOpticsBenchOpticsRemoved(int index, int count)
{
	foreach (QGraphicsItem* graphicsItem, items())
		if (OpticsItem* opticsItem = dynamic_cast<OpticsItem*>(graphicsItem))
			if (m_bench->opticsIndex(opticsItem->optics()) == -1)
				removeItem(graphicsItem);

	for (int i = index + count - 1; i >= index; i--)
		removeItem(m_beamItems.takeAt(i));
}

void OpticsScene::addFitPoint(double position, double radius, QRgb color)
{
	QGraphicsEllipseItem* fitItem = new QGraphicsEllipseItem(-2., -2., 4., 4.);
	fitItem->setFlags(fitItem->flags() | QGraphicsItem::ItemIgnoresTransformations);
	fitItem->setPen(QPen(color));
	fitItem->setBrush(QBrush(color));
	fitItem->setPos(position, radius);
	fitItem->setZValue(2.);
	m_fitItems.push_back(fitItem);
	addItem(fitItem);
}

void OpticsScene::onOpticsBenchFitsRemoved(int index, int count)
{
	Q_UNUSED(count);
	/// @todo with more houskeeping in OpticsScene::OpticsBenchFitDataChanged this will not work any more
	onOpticsBenchFitDataChanged(index);
}

void OpticsScene::onOpticsBenchFitDataChanged(int index)
{
	Q_UNUSED(index);

	/// @todo with a little bit of housekeeping, we could avoid recontructing all the items each time...
	while (!m_fitItems.isEmpty())
	{
		removeItem(m_fitItems.last());
		m_fitItems.removeLast();
	}

	for (int index = 0; index < m_bench->nFit(); index++)
	{
		Fit* fit = m_bench->fit(index);
		for (int i = 0; i < fit->size(); i++)
			if (fit->value(i) != 0.)
			{
				addFitPoint(fit->position(i), -fit->radius(i), fit->color());
				addFitPoint(fit->position(i),  fit->radius(i), fit->color());
			}
	}
}

/////////////////////////////////////////////////
// OpticsView class

OpticsView::OpticsView(QGraphicsScene* scene, OpticsBench* bench)
	: QGraphicsView(scene)
	, m_bench(bench)
{
	setRenderHint(QPainter::Antialiasing);
	setBackgroundBrush(Qt::white);
	m_horizontalRange = 0.;
	m_verticalRange = 0.;
	m_statusWidget = 0;

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_horizontalRuller = new RullerSlider(this);
	m_verticalRuller = new RullerSlider(this, true);
	setHorizontalScrollBar(m_horizontalRuller);
	setVerticalScrollBar(m_verticalRuller);

	m_opticsViewProperties = new OpticsViewProperties(this);
	m_opticsViewProperties->hide();

	CornerWidget* cornerWidget = new CornerWidget(QColor(245, 245, 200),
	              ":/images/zoom-best-fit.png", m_opticsViewProperties, this);
	setCornerWidget(cornerWidget);

	setResizeAnchor(QGraphicsView::AnchorViewCenter);

	connect(m_horizontalRuller, SIGNAL(valueChanged(int)), this, SLOT(scrollUpdated(int)));
}

void OpticsView::adjustRange()
{
	if ((m_horizontalRange > scene()->width()) || (m_horizontalRange == 0.))
		m_horizontalRange = scene()->width();

//	if ((m_verticalRange > scene()->height()) || (m_verticalRange == 0.))
//		m_verticalRange = scene()->height();

	if ((m_horizontalRange == 0.) /*|| (m_verticalRange == 0.)*/ || (width() == 0.) || (height() == 0.))
		return;

	double scale = width()/m_horizontalRange;
	QMatrix scaling = matrix();
	scaling.setMatrix(scale, scaling.m12(), scaling.m21(), scale, scaling.dx(), scaling.dy());
	setMatrix(scaling);

	m_opticsViewProperties->setViewWidth(m_horizontalRange);
	m_opticsViewProperties->setViewHeight(m_verticalRange);
	m_opticsViewProperties->setViewOrigin(origin());
	verticalScrollBar()->setValue((verticalScrollBar()->maximum() + verticalScrollBar()->minimum())/2);
}

void OpticsView::showProperties(bool show)
{
	m_opticsViewProperties->setVisible(show);
}

bool OpticsView::propertiesVisible()
{
	return m_opticsViewProperties->isVisible();
}

void OpticsView::scrollUpdated(int value)
{
	Q_UNUSED(value);
	m_opticsViewProperties->setViewOrigin(origin());
}

double OpticsView::origin()
{
	return mapToScene(viewport()->rect().topLeft()).x();
}

void OpticsView::setOrigin(double origin)
{
	if (origin == OpticsView::origin())
		return;

	if (origin < sceneRect().left())
		origin = sceneRect().left();

	if (origin > sceneRect().right() - m_horizontalRange)
		origin = sceneRect().right() - m_horizontalRange;

	int value = int(origin*m_horizontalRuller->rullerScale());
	horizontalScrollBar()->setValue(value);
	m_opticsViewProperties->setViewOrigin(origin);
}

void OpticsView::setHorizontalRange(double horizontalRange)
{
	m_horizontalRange = horizontalRange;
	m_opticsViewProperties->setViewWidth(m_horizontalRange);
	adjustRange();
}

void OpticsView::setVerticalRange(double verticalRange)
{
	m_verticalRange = verticalRange;
	m_opticsViewProperties->setViewHeight(m_verticalRange);
	adjustRange();
}

void OpticsView::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);
	adjustRange();
	m_opticsViewProperties->move(viewport()->width() - m_opticsViewProperties->width(), viewport()->height() - m_opticsViewProperties->height());
}

void OpticsView::wheelEvent(QWheelEvent* event)
{
	/// @todo this does not work
	QApplication::sendEvent(m_horizontalRuller, event);
}

void OpticsView::mouseMoveEvent(QMouseEvent* event)
{
	if (m_statusWidget)
	{
		QPointF position = mapToScene(event->pos());
		std::vector<double> pos(2);
		pos[0] = position.x();
		pos[1] = -position.y();
		std::pair<Beam*, double> p = m_bench->closestPosition(pos);

		m_statusWidget->showBeamInfo(p.first, p.second);
	}

	QGraphicsView::mouseMoveEvent(event);
}

void OpticsView::drawBackground(QPainter* painter, const QRectF& rect)
{
	Q_UNUSED(painter);
	Q_UNUSED(rect);

/*	/// @todo if drawing a grid and rullers, set background cache
	const double pixel = scene()->width()/width();

	QBrush backgroundBrush(Qt::white);
	painter->fillRect(rect, backgroundBrush);

	QPen opticalAxisPen(Qt::lightGray, 2.*pixel, Qt::DashDotLine, Qt::FlatCap, Qt::RoundJoin);
	painter->setPen(opticalAxisPen);
	/// @todo replace scene()->sceneRect().left() by rect.left() once dashOffset works
	painter->drawLine(QPointF(scene()->sceneRect().left(), 0.), QPointF(rect.right(), 0.));


	QPen verticalAxisPen(Qt::lightGray, pixel, Qt::DotLine, Qt::FlatCap, Qt::RoundJoin);
	/// @todo idem
	painter->setPen(verticalAxisPen);
	painter->drawLine(QPointF(0., scene()->sceneRect().top()), QPointF(0., rect.bottom()));*/
}

/////////////////////////////////////////////////
// OpticsItem class

OpticsItem::OpticsItem(const Optics* optics, OpticsBench* bench)
	: QGraphicsItem()
	, m_optics(optics)
	, m_bench(bench)
{
	if (m_optics->type() != CreateBeamType)
	{
		setCursor(Qt::OpenHandCursor);
		setFlag(ItemIsMovable);
		setZValue(1);
	}
	else
		setZValue(-1);

	if (m_optics->width() == 0.)
		setFlag(QGraphicsItem::ItemIgnoresTransformations);

	m_update = true;
}

QRectF OpticsItem::boundingRect() const
{
	QRectF bounding;

	if (m_optics->width() == 0.)
		bounding = QRectF(QPointF(-10., -66.), QSizeF(2.*10., 2.*66.));
	else
		bounding = QRectF(QPointF(0., -SCENEHALFHEIGHT), QSizeF(m_optics->width(), 2.*SCENEHALFHEIGHT));

	return bounding;
}

QVariant OpticsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
	if ((change == ItemPositionChange && scene()) && m_update)
	{
		QPointF newPos = value.toPointF();
		std::vector<double> benchPosition(2);
		benchPosition[0] = newPos.x();
		benchPosition[1] = -newPos.y();
		std::pair<Beam*, double> p = m_bench->closestPosition(benchPosition);

		// Propose the new position
		qDebug() << "Old Pos" << x() << y();
		m_bench->setOpticsPosition(m_bench->opticsIndex(m_optics), p.second);
		qDebug() << "New Pos" << x() << y();
		return pos();
	}

	return QGraphicsItem::itemChange(change, value);
 }

void OpticsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	QColor opticsColor = QColor(153, 209, 247, 150);
	QBrush opticsBrush(opticsColor);
	painter->setBrush(opticsBrush);
	QPen textPen(Qt::black);

	QPainterPath path;
	QRectF rect = boundingRect();
	QRectF rightRect = rect;
	rightRect.moveLeft(rect.width()/4.);
	QRectF leftRect = rect;
	leftRect.moveRight(-rect.width()/4.);

	if (m_optics->type() == CreateBeamType)
	{
		// no painting !
	}
	else if (m_optics->type() == LensType)
	{
		if (dynamic_cast<const Lens*>(m_optics)->focal() >= 0.)
		{
			path.moveTo(0., rect.top());
			path.arcTo(rect, 90., 180.);
			path.arcTo(rect, 270., 180.);
		}
		else
		{
			path.moveTo(leftRect.center().x(), rect.top());
			path.arcTo(rightRect, 90., 180.);
			path.arcTo(leftRect, 270., 180.);
		}
		painter->drawPath(path);
	}
	else if ((m_optics->type() == FlatInterfaceType) || (m_optics->type() == CurvedInterfaceType))
	{
		double indexRatio = dynamic_cast<const Interface*>(m_optics)->indexRatio();
		double origin = (indexRatio >= 1.) ? rect.right() : rect.left();
		path.moveTo(origin, rect.top());
		path.lineTo(rect.center().x(), rect.top());
		if (m_optics->type() == FlatInterfaceType)
			path.lineTo(rect.center().x(), rect.bottom());
		else
			path.arcTo(rect, 90., dynamic_cast<const CurvedInterface*>(m_optics)->surfaceRadius() >= 0. ? 180. : -180.);
		path.lineTo(origin, rect.bottom());
		painter->drawPath(path);
	}
	else if (m_optics->type() == DielectricSlabType)
	{
		painter->drawRect(rect);
	}
	else if (m_optics->type() == FlatMirrorType)
	{
		QRectF mirrorRect = rect;
		mirrorRect.setLeft(0.);

		painter->setPen(QPen(Qt::NoPen));
		painter->setBrush(QBrush(Qt::black, Qt::BDiagPattern));
		painter->drawRect(mirrorRect);

		mirrorRect.setRight(rect.right()/5.);
		painter->setPen(QPen());
		painter->setBrush(QBrush(Qt::black));
		painter->drawRect(mirrorRect);
	}
	else if (m_optics->type() == CurvedMirrorType)
	{
		painter->drawRect(rect);
		QBrush ABCDBrush(Qt::black, Qt::BDiagPattern);
		painter->setBrush(ABCDBrush);
		painter->drawRect(rect);
	}
	else if (m_optics->type() == GenericABCDType)
	{
		QColor color = QColor(193, 193, 193, 150);
		painter->setBrush(QBrush(color));
		painter->drawRect(rect);
	}
/*
	if (m_optics->type() != CreateBeamType)
	{
		painter->setPen(textPen);
		QString text = QString::fromUtf8(m_optics->name().c_str());
		QRectF textRect(0., 0., 0., 0.);
		textRect.moveCenter(rect.center() - QPointF(0., rect.height()*0.7));
		textRect = painter->boundingRect(textRect, Qt::AlignCenter, text);
		painter->drawText(textRect, Qt::AlignCenter, text);
	}
*/
}

/////////////////////////////////////////////////
// BeamView class

BeamItem::BeamItem(const Beam* beam)
	: QGraphicsItem()
	, m_beam(beam)
{
	m_drawText = true;
	m_style = true;
	m_auxiliary = false;
	m_scale = 100.;
	setZValue(0.);
}

void BeamItem::updateTransform()
{
	Q_ASSERT(beam()->origin().size() == 2);

	resetTransform();
	setPos(beam()->origin()[0], -beam()->origin()[1]);
	rotate(-beam()->angle()*180./M_PI);
	scale(1., m_scale);
	prepareGeometryChange();
}

QRectF BeamItem::boundingRect() const
{
	QRectF result = scene()->sceneRect();
	result.setLeft(m_beam->start());
	result.setRight(m_beam->stop());
	return result;
}

void BeamItem::drawSegment(double start, double stop, double pixel, int nStep, QPolygonF& polygon) const
{
	if (start < m_beam->start())
		start = m_beam->start();

	if (stop > m_beam->stop())
		stop = m_beam->stop();

	if (start >= stop)
		return;

	// "pixel" is added to avoid overlong iterations due to very small steps caused by rounding errors
	double step = (stop - start)/double(nStep) + pixel;
	if (step < pixel)
		step = pixel;

	for (double z = start; z <= stop; z += step)
		polygon.append(QPointF(z, m_beam->radius(z)));
}

void BeamItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(widget);

	QColor beamColor = wavelengthColor(m_beam->wavelength());
	beamColor.setAlpha(200);

	QPen beamPen(beamColor);
	painter->setPen(beamPen);

	QBrush beamBrush(beamColor, m_style ? Qt::SolidPattern : Qt::NoBrush);
	painter->setBrush(beamBrush);

	QPen textPen(Qt::black);

	const double waist = m_beam->waist();
	const double waistPosition = m_beam->waistPosition();
	const double rayleigh = m_beam->rayleigh();

	double horizontalScale = 1./sqrt(sqr(option->matrix.m11()) + sqr(option->matrix.m12())); // m/Pixels
	double verticalScale   = 1./sqrt(sqr(option->matrix.m22()) + sqr(option->matrix.m21())); // m/Pixels

	// The waist is pixel reoslved
	if (waist/verticalScale > 1.)
	{
		QPolygonF beamPolygonUp, beamPolygonDown;

		// Construct the upper part of the beam
		double nextLeft, left = m_beam->start();
		drawSegment(left, nextLeft = waistPosition - 5.*rayleigh, horizontalScale, 1,  beamPolygonUp); left = nextLeft;
		drawSegment(left, nextLeft = waistPosition - rayleigh,    horizontalScale, 10, beamPolygonUp); left = nextLeft;
		drawSegment(left, nextLeft = waistPosition + rayleigh,    horizontalScale, 20, beamPolygonUp); left = nextLeft;
		drawSegment(left, nextLeft = waistPosition + 5.*rayleigh, horizontalScale, 10, beamPolygonUp); left = nextLeft;
		drawSegment(left, nextLeft = m_beam->stop(),                horizontalScale, 1,  beamPolygonUp); left = nextLeft;
		beamPolygonUp.append(QPointF(m_beam->stop(), m_beam->radius(m_beam->stop())));

		// Mirror the upper part to make the lower part
		for (int i = beamPolygonUp.size() - 1; i >= 0; i--)
			beamPolygonDown.append(QPointF(beamPolygonUp[i].x(), -beamPolygonUp[i].y()));

		// Draw the beam
		QPainterPath path;
		path.moveTo(beamPolygonUp[0]);
		path.addPolygon(beamPolygonUp);
		path.lineTo(beamPolygonDown[0]);
		path.addPolygon(beamPolygonDown);
		path.lineTo(beamPolygonUp[0]);
		painter->drawPath(path);
	}
	// The waist is not pixel resolved
	else
	{
		double sgn = sign((m_beam->stop() - waistPosition)*(m_beam->start() - waistPosition));
		double rightRadius = m_beam->radius(m_beam->stop());
		double leftRadius = m_beam->radius(m_beam->start());

		QPolygonF ray;
		ray << QPointF(m_beam->start(), leftRadius)
			<< QPointF(m_beam->stop(), sgn*rightRadius)
			<< QPointF(m_beam->stop(), -sgn*rightRadius)
			<< QPointF(m_beam->start(), -leftRadius);
		painter->drawConvexPolygon(ray);
	}


	// Waist label
	/// @todo check if in view
	if (m_drawText)
	{
		painter->setPen(textPen);
		QPointF waistTop(waistPosition, - m_beam->waist());
		painter->drawLine(QPointF(waistPosition, 0.), waistTop);
		QString text; text.setNum(round(m_beam->waist()*Units::getUnit(UnitWaist).divider()));
		QRectF textRect(0., 0., 100., 15.);
		textRect.moveCenter(waistTop - QPointF(0., 15.));
		painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, text);
	}
}
