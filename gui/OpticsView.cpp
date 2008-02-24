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

#include "gui/OpticsView.h"
#include "gui/GaussianBeamModel.h"
#include "gui/Unit.h"
#include "src/GaussianBeam.h"

#include <QtGui>
#include <QtDebug>

#include <cmath>

QColor wavelengthColor(double wavelength)
{
	wavelength *= 1e9;

	if (wavelength < 380.)
		return QColor::fromRgbF(0., 0., 1.);
	if ((wavelength >= 380.) && (wavelength < 440.))
		return QColor::fromRgbF(0., 0., 1.);
	if ((wavelength >= 440.) && (wavelength < 490.))
		return QColor::fromRgbF(0., (wavelength-440.)/50., 1.);
	if ((wavelength >= 490.) && (wavelength < 510.))
		return QColor::fromRgbF(0., 1. , (510.-wavelength)/40.);
	if ((wavelength >= 510.) && (wavelength < 580.))
		return QColor::fromRgbF((wavelength-510.)/70., 1., 0.);
	if ((wavelength >= 580.) && (wavelength < 645.))
		return QColor::fromRgbF(1., (645.-wavelength)/85., 0.);
	if ((wavelength >= 645.) && (wavelength < 780.))
		return QColor::fromRgbF(1., 0., 0.);
	if ((wavelength >= 780.) && (wavelength < 800.))
		return QColor::fromRgbF((900.-wavelength)/120., 0., 0.);
	if (wavelength >= 800.)
		return QColor::fromRgbF(.833, 0., 0.);

	return Qt::black;
}

/////////////////////////////////////////////////
// OpticsScene class

OpticsScene::OpticsScene(OpticsBench& bench, QObject* parent)
	: QGraphicsScene(parent)
	, OpticsBenchNotify(bench)
{
	setItemIndexMethod(QGraphicsScene::NoIndex);
	m_bench.registerNotify(this);

	m_targetBeamItem = new BeamItem(m_bench.targetBeam());
	m_targetBeamItem->setPlainStyle(false);
	addItem(m_targetBeamItem);

	QGraphicsEllipseItem* fitItem = new QGraphicsEllipseItem(-3., -3., 3., 3.);
	fitItem->setFlags(fitItem->flags() | QGraphicsItem::ItemIgnoresTransformations);
	fitItem->setPen(QPen(Qt::black));
	fitItem->setBrush(QBrush(Qt::black));
	fitItem->setZValue(2.);
	m_fitItems.push_back(fitItem);
	addItem(fitItem);
}

void OpticsScene::showTargetBeam(bool show)
{
	m_targetBeamItem->setVisible(show);
}

void OpticsScene::OpticsBenchDataChanged(int startOptics, int endOptics)
{
	setSceneRect(m_bench.leftBoundary(), -0.005, m_bench.rightBoundary() - m_bench.leftBoundary(), 0.01);

	foreach (QGraphicsItem* graphicsItem, items())
	{
		if (OpticsItem* opticsItem = dynamic_cast<OpticsItem*>(graphicsItem))
		{
			int opticsIndex = m_bench.opticsIndex(opticsItem->optics());
			if ((opticsIndex >= startOptics) && (opticsIndex <= endOptics))
			{
				opticsItem->setUpdate(false);
				opticsItem->setPos(opticsItem->optics()->position(), 0.);
				opticsItem->setUpdate(true);
			}
		}
	}

	for (int i = qMax(0, startOptics-1); i <= endOptics; i++)
	{
		const Optics* optics = m_bench.optics(i);
		m_beamItems[i]->setPos(0., 0.);
		if (i == 0)
			m_beamItems[i]->setLeftBound(m_bench.leftBoundary());
		else
			m_beamItems[i]->setLeftBound(optics->position() + optics->width());
		if (i == m_bench.nOptics()-1)
			m_beamItems[i]->setRightBound(m_bench.rightBoundary());
		else
			m_beamItems[i]->setRightBound(m_bench.optics(i+1)->position());
		//qDebug() << i << m_beamItems[i]->leftBound() <<  m_beamItems[i]->rightBound();
	}

	/// @todo this should go to a "bound changed" bench event
	m_targetBeamItem->setPos(0., 0.);
	m_targetBeamItem->setLeftBound(m_bench.leftBoundary());
	m_targetBeamItem->setRightBound(m_bench.rightBoundary());

	/// @todo this should go to a "fit changed" bench event
}

void OpticsScene::OpticsBenchTargetBeamChanged()
{
	m_targetBeamItem->update();
}

void OpticsScene::OpticsBenchOpticsAdded(int index)
{
	OpticsItem* opticsItem = new OpticsItem(m_bench.optics(index), m_bench);
	//qDebug() << "WAIST OpticsBenchOpticsAdded" <<  m_bench.beam(index).waist();
	addItem(opticsItem);

	// recreate BeamItem list (reference to list element don't survive a list resize !)
	while (!m_beamItems.isEmpty())
	{
		removeItem(m_beamItems.last());
		m_beamItems.removeLast();
	}

	for (int i = 0; i < m_bench.nOptics(); i++)
	{
		BeamItem* beamItem = new BeamItem(m_bench.beam(i));
		m_beamItems.append(beamItem);
		addItem(beamItem);
	}
}

void OpticsScene::OpticsBenchOpticsRemoved(int index, int count)
{
	foreach (QGraphicsItem* graphicsItem, items())
		if (OpticsItem* opticsItem = dynamic_cast<OpticsItem*>(graphicsItem))
			if (m_bench.opticsIndex(opticsItem->optics()) == -1)
				removeItem(graphicsItem);

	for (int i = index; i < index + count; i++)
	{
		removeItem(m_beamItems[i]);
		m_beamItems.removeAt(i);
	}
}

/////////////////////////////////////////////////
// Ruller slider
class RullerSlider : public QScrollBar
{
public:
	RullerSlider(QGraphicsScene* scene, QWidget* parent = 0);

protected:
	virtual void sliderChange(SliderChange change);
	virtual void paintEvent(QPaintEvent* event);

public:
	void setScale(double scale) { m_scale = scale; }

private:
	QGraphicsScene* m_scene;
	double m_scale;
};

RullerSlider::RullerSlider(QGraphicsScene* scene, QWidget* parent)
	: QScrollBar(parent)
{
	m_scene = scene;
	m_scale = 1.;
}

void RullerSlider::sliderChange(SliderChange change)
{
	QScrollBar::sliderChange(change);
}

void RullerSlider::paintEvent(QPaintEvent* event)
{
	//qDebug() << "SCALE : " << m_scale << minimum() << value() << maximum() << pageStep();

	QPainter painter(this);
	QColor backgroundColor(245, 245, 200);
	QBrush backgroundBrush(backgroundColor);
	QPen backgroundPen(backgroundColor);
	QPen mainTickPen(Qt::black);
	QPen secondTickPen(Qt::lightGray);

	int length = maximum() + pageStep() - minimum();
	double scale = double(length)/m_scene->width();
	double range = double(pageStep())/scale;
	double spacing = 0.01;
	int lastStep = int(double(value() + pageStep())/(scale*spacing)) + 1;
	int firstStep = int(double(value())/(scale*spacing)) - 1;

	painter.setBrush(backgroundBrush);
	painter.setPen(backgroundPen);
	painter.drawRect(rect());
	painter.setPen(secondTickPen);
	for (double x = double(lastStep)*spacing; x >= double(firstStep)*spacing; x -= spacing)
	{
		double pos = x*scale - double(value());
		painter.drawLine(pos, 0., pos, height()*0.4);
		if ((int(round(x/spacing)) % 10) == 5)
			painter.drawLine(pos, 0., pos, height()*0.65);
		else if ((int(round(x/spacing)) % 10) == 0)
		{
			painter.setPen(mainTickPen);
			painter.drawLine(pos, 0., pos, height()*0.8);
			QString text;  text.setNum(round(x*1000.));
			QRectF textRect(0., 0., 0., 0.);
			textRect.moveCenter(QPointF(pos + 2., height()/2.));
			textRect = painter.boundingRect(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
			painter.drawText(textRect, Qt::AlignCenter, text);
			painter.setPen(secondTickPen);
		}
	}
}

/////////////////////////////////////////////////
// OpticsView class

OpticsView::OpticsView(QGraphicsScene* scene)
	: QGraphicsView(scene)
{
	setRenderHint(QPainter::Antialiasing);
	setBackgroundBrush(Qt::white);
	m_horizontalRange = 0.;
	m_verticalRange = 0.;

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	m_horizontalRuller = new RullerSlider(scene, this);

	setResizeAnchor(QGraphicsView::AnchorViewCenter);
	centerOn(0., 0.);
}

void OpticsView::showEvent(QShowEvent* event)
{
	QGraphicsView::showEvent(event);
	/// @bug putting this instruction in the constructor (i.e. befor show) triggers a qt bug to be fixed in 4.4.0
	setHorizontalScrollBar(m_horizontalRuller);
}

void OpticsView::adjustRange()
{
	if ((m_horizontalRange == 0.) || (m_verticalRange == 0.) || (width() == 0.) || (height() == 0.))
		return;

	QMatrix scaling = matrix();
	scaling.setMatrix(width()/m_horizontalRange, scaling.m12(), scaling.m21(), height()/m_verticalRange, scaling.dx(), scaling.dy());
	setMatrix(scaling);

	foreach (QGraphicsItem* graphicsItem, items())
		if (OpticsItem* opticsItem = dynamic_cast<OpticsItem*>(graphicsItem))
			opticsItem->adjustScale(m_horizontalRange*height()/width(), m_verticalRange);

	m_horizontalRuller->setScale(m_horizontalRange/width());
}

void OpticsView::setHorizontalRange(double horizontalRange)
{
	m_horizontalRange = horizontalRange;
	adjustRange();
}

void OpticsView::setVerticalRange(double verticalRange)
{
	m_verticalRange = verticalRange;
	adjustRange();
}

void OpticsView::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);
	adjustRange();
}

void OpticsView::wheelEvent(QWheelEvent* event)
{
	QApplication::sendEvent(m_horizontalRuller, event);
}

void OpticsView::mouseMoveEvent(QMouseEvent* event)
{
	if (m_statusBar)
	{
		QPointF position = mapToScene(event->pos());
		position.setY(0.);
		QString text = tr("Position: ") + QString::number(position.x()*Units::getUnit(UnitPosition).divider(), 'f', 2) + " " + tr("mm") + "    ";

		foreach (QGraphicsItem* graphicsItem, items())
			if (BeamItem* beamItem = dynamic_cast<BeamItem*>(graphicsItem))
				if (beamItem->boundingRect().contains(position))
				{
					text += tr("Beam radius: ") + QString::number(beamItem->beam().radius(position.x())*Units::getUnit(UnitWaist).divider(), 'f', 2) + " " +  tr("µm") + "    " +
							tr("Beam curvature: ") + QString::number(beamItem->beam().curvature(position.x())*Units::getUnit(UnitCurvature).divider(), 'f', 2) +  " " + tr("mm") + "    ";
					break;
				}

		m_statusBar->showMessage(text);
	}

	QGraphicsView::mouseMoveEvent(event);
}

void OpticsView::drawBackground(QPainter* painter, const QRectF& rect)
{
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

OpticsItem::OpticsItem(const Optics* optics, OpticsBench& bench)
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

	m_update = true;

}

void OpticsItem::adjustScale(double horizontalScale, double verticalScale)
{
	double fac = 300.;
	double m11 = horizontalScale/fac;
	double m22 = verticalScale/fac;

	if (m_optics->width() > 0.)
		m11 = 1.0;

	QTransform scaling = transform();
	scaling.setMatrix(m11, scaling.m12(),  scaling.m13(), scaling.m21(), m22,
	                  scaling.m23(), scaling.m31(), scaling.m32(), scaling.m33());
	setTransform(scaling);
}

QRectF OpticsItem::boundingRect() const
{
	QRectF bounding = QRectF(QPointF(-10., -66.), QSizeF(2.*10., 2.*66.));

	if (m_optics->width() > 0.)
	{
		bounding.setLeft(0.);
		bounding.setRight(m_optics->width());
	}

	return bounding;
}

QVariant OpticsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
	if ((change == ItemPositionChange && scene()) && m_update)
	{
		QPointF newPos = value.toPointF();
		QRectF rect = scene()->sceneRect();
		if (!rect.contains(newPos))
			newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
		newPos.setY(0.);
		m_bench.setOpticsPosition(m_bench.opticsIndex(m_optics), newPos.x());
		return newPos;
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
	else if (m_optics->type() == FlatMirrorType)
	{
		painter->drawRect(rect);
		QBrush ABCDBrush(Qt::black, Qt::BDiagPattern);
		painter->setBrush(ABCDBrush);
		painter->drawRect(rect);

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
		painter->drawRoundRect(rect);
	}

	if (m_optics->type() != CreateBeamType)
	{
		painter->setPen(textPen);
		QString text = QString::fromUtf8(m_optics->name().c_str());
		QRectF textRect(0., 0., 0., 0.);
		textRect.moveCenter(rect.center() - QPointF(0., rect.height()*0.7));
		textRect = painter->boundingRect(textRect, Qt::AlignCenter, text);
		painter->drawText(textRect, Qt::AlignCenter, text);
	}
}

/////////////////////////////////////////////////
// BeamView class

BeamItem::BeamItem(const Beam& beam)
	: QGraphicsItem()
	, m_beam(beam)
{
	m_drawText = true;
	m_style = true;
	setZValue(0.);
}

void BeamItem::setLeftBound(double leftBound)
{
	prepareGeometryChange();
	m_leftBound = leftBound;
}

void BeamItem::setRightBound(double rightBound)
{
	prepareGeometryChange();
	m_rightBound = rightBound;
}

QRectF BeamItem::boundingRect() const
{
	QRectF result = scene()->sceneRect();
	result.setLeft(m_leftBound);
	result.setRight(m_rightBound);
	return result;
}

void BeamItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QColor beamColor = wavelengthColor(m_beam.wavelength());
	beamColor.setAlpha(200);

	QPen beamPen(beamColor);
	painter->setPen(beamPen);

	QBrush beamBrush(beamColor, m_style ? Qt::SolidPattern : Qt::NoBrush);
	painter->setBrush(beamBrush);

	QPen textPen(Qt::black);

	double waistPosition = m_beam.waistPosition();
	double magnification = 1.;//scene()->height()/dynamic_cast<OpticsScene*>(scene())->verticalRange();

	/// @todo check degree of details
//	if (m_beam.waist() > 0.0000001/* *vScale() > 1.*/)
	{
		/** @bug this prevents a lockup for double roundup errors.
		* Test case : remove all lenses, add two lenses. The program crashed becaus maxZ-minZ = 1e-18
		* In future version of hyperbolic drwaing, check that this does not appear.
		*/
		double epsilon = 1e-6;
		Approximation approximation;
		approximation.minZ = m_leftBound;
		approximation.maxZ = m_rightBound;
		/// @todo approximation.resolution = 1./vScale();

		if (approximation.minZ + epsilon < approximation.maxZ)
		{
			//qDebug() << " Drawing waist" << approximation.minZ << approximation.maxZ;
			QPolygonF beamPolygonUp, beamPolygonDown;
			for (double z = approximation.minZ; z <= approximation.maxZ; z = m_beam.approxNextPosition(z, approximation))
			{
				//qDebug() << z;
				beamPolygonUp.append(QPointF(z, m_beam.radius(z)*magnification));
				beamPolygonDown.prepend(QPointF(z, -m_beam.radius(z)*magnification));
				if (z == approximation.maxZ)
					break;
			}
			beamPolygonUp.append(QPointF(approximation.maxZ, m_beam.radius(approximation.maxZ)*magnification));
			beamPolygonDown.prepend(QPointF(approximation.maxZ, -m_beam.radius(approximation.maxZ)*magnification));
			QPainterPath path;
			path.moveTo(beamPolygonUp[0]);
			path.addPolygon(beamPolygonUp);
			path.lineTo(beamPolygonDown[0]);
			path.addPolygon(beamPolygonDown);
			path.lineTo(beamPolygonUp[0]);

			painter->drawPath(path);
		}
	}
//	else
/*	{
		double sgn = sign((m_rightBound - waistPosition)*(m_leftBound - waistPosition));
		double rightRadius = m_beam.radius(m_rightBound);
		double leftRadius = m_beam.radius(m_leftBound);

		QPolygonF ray;
		ray << QPointF(m_leftBound, leftRadius)
			<< QPointF(m_rightBound, sgn*rightRadius)
			<< QPointF(m_rightBound, -sgn*rightRadius)
			<< QPointF(m_leftBound, -leftRadius);
		painter->drawConvexPolygon(ray);
	}
*/
	// Waist label
	/// @todo check if in view
	if (m_drawText)
	{
		painter->setPen(textPen);
		QPointF waistTop(waistPosition, - m_beam.waist()*magnification);
		painter->drawLine(QPointF(waistPosition, 0.), waistTop);
		QString text; text.setNum(round(m_beam.waist()*Units::getUnit(UnitWaist).divider()));
		QRectF textRect(0., 0., 100., 15.);
		textRect.moveCenter(waistTop - QPointF(0., 15.));
		painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, text);
	}
}

/////////////////////////////////////////////////
// OpticsItemView class


OpticsItemView::OpticsItemView(OpticsBench& bench, QWidget *parent)
	: QAbstractItemView(parent)
	, m_bench(bench)
{
	m_statusBar = 0;
	m_fitModel = 0;

	m_hOffset = -0.1;
	m_hRange = 0.7;
	m_vRange = 5e-3;

	computeTranformMatrix();
	computePaths();

	m_active_object = -1;
	m_active_object_offset = 0.;
	m_statusBar = 0;

	m_showTargetBeam = false;

	setMouseTracking(true);
}

void OpticsItemView::computePaths()
{
	QRectF lens = objectRect();
	QRectF rightLens = lens;
	rightLens.moveLeft(lens.width()/4.);
	QRectF leftLens = lens;
	leftLens.moveRight(-lens.width()/4.);

	m_convexLensPath = QPainterPath();
	m_convexLensPath.moveTo(0., lens.top());
	m_convexLensPath.arcTo(lens, 90., 180.);
	m_convexLensPath.arcTo(lens, 270., 180.);

	m_concaveLensPath = QPainterPath();
	m_concaveLensPath.moveTo(leftLens.center().x(), lens.top());
	m_concaveLensPath.arcTo(rightLens, 90., 180.);
	m_concaveLensPath.arcTo(leftLens, 270., 180.);

	m_flatInterfacePath = QPainterPath();
	m_flatInterfacePath.moveTo(lens.right(), lens.top());
	m_flatInterfacePath.lineTo(lens.center().x(), lens.top());
	m_flatInterfacePath.lineTo(lens.center().x(), lens.bottom());
	m_flatInterfacePath.lineTo(lens.right(), lens.bottom());

	/// @todo chech the correct relation between the radius of curvature and the concavity

	m_convexInterfacePath = QPainterPath();
	m_convexInterfacePath.moveTo(0., lens.top());
	m_convexInterfacePath.arcTo(lens, 90., 180.);
//
	m_concaveInterfacePath = QPainterPath();
	m_concaveInterfacePath.moveTo(0., lens.bottom());
	m_concaveInterfacePath.arcTo(lens, 270., 180.);
}

void OpticsItemView::computeTranformMatrix()
{
	m_abs2view = QMatrix(hScale(), 0., 0., vScale(), -m_hOffset*hScale(), m_vRange/2.*vScale());
	m_view2abs = m_abs2view.inverted();
}

QRectF OpticsItemView::objectRect() const
{
	double h = double(height());
	return QRectF(QPointF(-h*0.03, -h*0.2), QSizeF(2.*h*0.03, 2.*h*0.2));
}

double OpticsItemView::vScale() const
{
	return double(height())/m_vRange;
}

double OpticsItemView::hScale() const
{
	return double(width())/m_hRange;
}

void OpticsItemView::setHRange(double hRange)
{
	m_hRange = hRange;
	computeTranformMatrix();
	computePaths();
	viewport()->update();
}

void OpticsItemView::setVRange(double vRange)
{
	m_vRange = vRange;
	computeTranformMatrix();
	computePaths();
	viewport()->update();
}

void OpticsItemView::setHOffset(double hOffset)
{
	m_hOffset = hOffset;
	computeTranformMatrix();
	computePaths();
	viewport()->update();
}

void OpticsItemView::setShowTargetBeam(bool showTargetBeam)
{
	m_showTargetBeam =  showTargetBeam;
	viewport()->update();
}

///////////////////////

void OpticsItemView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	QAbstractItemView::dataChanged(topLeft, bottomRight);
	viewport()->update();
}

QRect OpticsItemView::visualRect(const QModelIndex& /*index*/) const
{
	return viewport()->rect();

	/// @todo find a way to express the visual rect including the beam
/*	Optics* optics = dynamic_cast<GaussianBeamModel*>(model())->optics(index.row());

	QPointF abs_objCenter = QPointF(optics->position(), 0.);
	QPointF view_objCenter = abs_objCenter*m_abs2view;
	QRectF rect = objectRect();
	rect.moveCenter(view_objCenter);

	return rect.toRect();*/
}

void OpticsItemView::scrollTo(const QModelIndex& /*index*/, ScrollHint /*hint*/)
{
}

QModelIndex OpticsItemView::indexAt(const QPoint& point) const
{
	for (int row = 0; row < model()->rowCount(); ++row)
	{
		const Optics* currentOptics = m_bench.optics(row);

		QPointF abs_ObjectLeft = QPointF(currentOptics->position(), 0.);
		QPointF abs_ObjectRight = QPointF(currentOptics->position() + currentOptics->width(), 0.);
		QPointF view_ObjectLeft = abs_ObjectLeft*m_abs2view;
		QPointF view_ObjectRight = abs_ObjectRight*m_abs2view;
		QPointF view_ObjectCenter = (view_ObjectLeft + view_ObjectRight)/2.;

		QRectF view_ObjectRect = objectRect();
		view_ObjectRect.moveCenter(view_ObjectCenter);
		if (view_ObjectRight.x() - view_ObjectLeft.x() > view_ObjectRect.width())
		{
			view_ObjectRect.setLeft(view_ObjectLeft.x());
			view_ObjectRect.setRight(view_ObjectRight.x());
		}

		QPointF abs_objCenter = QPointF(currentOptics->position(), 0.);
		QPointF distFromObjCenter = QPointF(point) - abs_objCenter*m_abs2view;
		if ((currentOptics->type() != CreateBeamType) &&
		    view_ObjectRect.contains(QPointF(point)))
			return model()->index(row, 0);
	}

	return QModelIndex();
}

QModelIndex OpticsItemView::moveCursor(QAbstractItemView::CursorAction /*cursorAction*/,
						Qt::KeyboardModifiers /*modifiers*/)
{
	return QModelIndex();
}

int OpticsItemView::horizontalOffset() const
{
	return 0;
}

int OpticsItemView::verticalOffset() const
{
	return 0;
}

bool OpticsItemView::isIndexHidden(const QModelIndex& /*index*/) const
{
	return false;
}

void OpticsItemView::setSelection(const QRect&, QItemSelectionModel::SelectionFlags /*command*/)
{
}

QRegion OpticsItemView::visualRegionForSelection(const QItemSelection& selection) const
{
	int ranges = selection.count();
	if (ranges == 0)
		return QRect();

	QRegion region;
	for (int i = 0; i < ranges; i++)
	{
		QItemSelectionRange range = selection.at(i);
		for (int row = range.top(); row <= range.bottom(); row++)
			region += visualRect(model()->index(row, 0));
	}
	return region;
}

void OpticsItemView::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
	computeTranformMatrix();
	computePaths();
}

void OpticsItemView::mousePressEvent(QMouseEvent* event)
{
	QAbstractItemView::mousePressEvent(event);

	QModelIndex index = indexAt(event->pos());
	if (index.isValid())
	{
		const Optics* currentOptics = m_bench.optics(index.row());

		QPointF abs_ObjectLeft = QPointF(currentOptics->position(), 0.);
		QPointF view_ObjectLeft = abs_ObjectLeft*m_abs2view;

		m_active_object_offset = double(event->pos().x()) - view_ObjectLeft.x();
		m_active_object = index.row();
	}

	if (m_active_object != -1)
		mouseMoveEvent(event);
}

void OpticsItemView::mouseMoveEvent(QMouseEvent* event)
{
	if (m_active_object >= 0)
	{
		QPointF pos = QPointF(event->pos()) - QPointF(m_active_object_offset, 0.);
		m_active_object = m_bench.setOpticsPosition(m_active_object, (pos*m_view2abs).x());
	}
	else if (m_statusBar)
	{
		double abs_position = (QPointF(event->pos())*m_view2abs).x();
		QString text = tr("Position: ") + QString::number(abs_position*Units::getUnit(UnitPosition).divider(), 'f', 2) + " " + tr("mm") + "    ";

		for (int row = model()->rowCount() - 1; row >= 0; row--)
		{
			const Optics* currentOptics = m_bench.optics(row);

			QPointF abs_objectLeft = QPointF(currentOptics->position(), 0.);
			QPointF abs_objectRight = QPointF(currentOptics->position() + currentOptics->width(), 0.);
			// Check if we are inside the optics
			if ((currentOptics->width() > 0.) && (abs_objectLeft.x() < abs_position) && (abs_objectRight.x() > abs_position))
				break;
			// Check if we are lookgin at the mode created by this optics
			if ((currentOptics->type() != CreateBeamType) && (abs_objectRight.x() < abs_position) ||
				(currentOptics->type() == CreateBeamType))
			{
				const Beam& beam = m_bench.beam(row);
				text += tr("Beam radius: ") + QString::number(beam.radius(abs_position)*Units::getUnit(UnitWaist).divider(), 'f', 2) + " " +  tr("µm") + "    " +
				        tr("Beam curvature: ") + QString::number(beam.curvature(abs_position)*Units::getUnit(UnitCurvature).divider(), 'f', 2) +  " " + tr("mm") + "    ";
				break;
			}
		}
		m_statusBar->showMessage(text);
	}
	QAbstractItemView::mouseMoveEvent(event);
}

void OpticsItemView::mouseReleaseEvent(QMouseEvent* event)
{
	QAbstractItemView::mouseReleaseEvent(event);
	m_active_object = -1;
}

void OpticsItemView::drawBeam(QPainter& painter, const Beam& beam, const QRectF& abs_beamRange, bool drawText)
{
	QColor beamColor = wavelengthColor(beam.wavelength());
	QPen beamPen = painter.pen();
	beamColor.setAlpha(beamPen.color().alpha());
	beamPen.setColor(beamColor);
	painter.setPen(beamPen);
	QBrush beamBrush = painter.brush();
	beamColor.setAlpha(beamBrush.color().alpha());
	beamBrush.setColor(beamColor);
	painter.setBrush(beamBrush);
	QPen textPen(Qt::black, 1);

	QRectF view_beamRange = m_abs2view.mapRect(abs_beamRange).normalized();
	QPointF abs_waistPos = QPointF(beam.waistPosition(), 0.);
	QPointF view_waistPos = abs_waistPos*m_abs2view;

	QPointF abs_left(m_hOffset, 0.);
	QPointF view_left = abs_left*m_abs2view;

//	double max_ray = 4.;
//	double max_w0  = sqrt(1. + sqr(max_ray));

	//QRectF abs_waistRect(0., 0., 2.*beam.rayleigh()*max_ray, 2.*beam.waist()*max_w0);
	//abs_waistRect.moveCenter(abs_waistPos);
	/// @todo check if the beam intersects the view. If not, quit.

	if (beam.waist()*vScale() > 1.)
	{
		/** @bug this prevents a lockup for double roundup errors.
		* Test case : remove all lenses, add two lenses. The program crashed becaus maxZ-minZ = 1e-18
		* In future version of hyperbolic drwaing, check that this does not appear.
		*/
		double epsilon = 1e-6;
		Approximation approximation;
		approximation.minZ = abs_beamRange.left();
		approximation.maxZ = abs_beamRange.right();
		approximation.resolution = 1./vScale();

		if (approximation.minZ + epsilon < approximation.maxZ)
		{
			//qDebug() << " Drawing waist" << approximation.minZ << approximation.maxZ;
			QPolygonF beamPolygonUp, beamPolygonDown;
			for (double z = approximation.minZ; z <= approximation.maxZ; z = beam.approxNextPosition(z, approximation))
			{
				//qDebug() << z;
				beamPolygonUp.append(QPointF(z, beam.radius(z)));
				beamPolygonDown.prepend(QPointF(z, -beam.radius(z)));
				if (z == approximation.maxZ)
					break;
			}
			beamPolygonUp.append(QPointF(approximation.maxZ, beam.radius(approximation.maxZ)));
			beamPolygonDown.prepend(QPointF(approximation.maxZ, -beam.radius(approximation.maxZ)));
			QPainterPath path;
			path.moveTo(beamPolygonUp[0]);
			path.addPolygon(beamPolygonUp);
			path.lineTo(beamPolygonDown[0]);
			path.addPolygon(beamPolygonDown);
			path.lineTo(beamPolygonUp[0]);

			path = path*m_abs2view;
			painter.drawPath(path);
		}
	}
	else
	{
		double sgn = sign((abs_beamRange.right() - abs_waistPos.x())*(abs_beamRange.left() - abs_waistPos.x()));
		double view_rightRadius = beam.radius(abs_beamRange.right())*vScale();
		double view_leftRadius = beam.radius(abs_beamRange.left())*vScale();

		QPolygonF ray;
		ray << QPointF(view_beamRange.left(), view_left.y() + view_leftRadius)
			<< QPointF(view_beamRange.right(), view_left.y() + sgn*view_rightRadius)
			<< QPointF(view_beamRange.right(), view_left.y() - sgn*view_rightRadius)
			<< QPointF(view_beamRange.left(), view_left.y() - view_leftRadius);
		painter.drawConvexPolygon(ray);
	}

	// Waist
	if (drawText)
	{
/*		painter.setPen(textPen);
		QPointF view_waistTop(view_waistPos.x(), view_waistPos.y() - beam.waist()*vScale());
		painter.drawLine(view_waistPos, view_waistTop);
		QString text; text.setNum(round(beam.waist()*Units::getUnit(UnitWaist).divider()));
		QRectF view_textRect(0., 0., 100., 15.);
		view_textRect.moveCenter(view_waistTop - QPointF(0., 15.));
		painter.drawText(view_textRect, Qt::AlignHCenter | Qt::AlignBottom, text);*/
	}
}


void OpticsItemView::paintEvent(QPaintEvent* event)
{
	// View properties
	QItemSelectionModel* selections = selectionModel();
	QStyleOptionViewItem option = viewOptions();
	QStyle::State state = option.state;

	// Drawing style
	QBrush background = option.palette.base();
	QPen axisPen(Qt::lightGray, 2, Qt::DashDotLine, Qt::FlatCap, Qt::RoundJoin);
	QPen rullerPen(Qt::gray, 1, Qt::DotLine, Qt::FlatCap, Qt::RoundJoin);
	QColor lensColor = QColor(153, 209, 247, 150);
	QBrush lensBrush(lensColor);
	QBrush currentLensBrush(lensColor, Qt::Dense4Pattern);
	QBrush selectedLensBrush(lensColor, Qt::Dense3Pattern);
	QPen lensPen(Qt::black, 1);
	QPen textPen(Qt::black, 1);
	QPen dataPen(Qt::black, 4, Qt::SolidLine, Qt::RoundCap);
	QColor beamColor = Qt::black;
	beamColor.setAlpha(200);
	QPen beamPen(QColor(0,0,0,0), 0);
	QBrush beamBrush(beamColor);
	QPen cavityBeamPen(beamColor, 2);
	cavityBeamPen.setStyle(Qt::DashLine);
	QBrush cavityBeamBrush;
	QPen targetBeamPen(beamColor, 2);
	targetBeamPen.setStyle(Qt::DotLine);
	QBrush targetBeamBrush;
	QBrush ABCDBrush(Qt::black, Qt::BDiagPattern);

	// Painter
	QPainter painter(viewport());
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(event->rect(), background);

	// Dimensions
	QRectF view_paintArea(0, 0, width(), height());
	QRectF abs_paintArea = m_view2abs.mapRect(view_paintArea);
	QPointF abs_center = QPointF(0., 0.);
	QPointF view_center = abs_center*m_abs2view;
	QPointF abs_left(m_hOffset, 0.);
	QPointF view_left = abs_left*m_abs2view;
	QPointF abs_right(m_hOffset + m_hRange, 0.);
	QPointF view_right = abs_right*m_abs2view;

	// Optical axis
	painter.setPen(axisPen);
	painter.drawLine(view_left, view_right);
	painter.setPen(rullerPen);
	painter.drawLine(int(view_center.x()), 0, int(view_center.x()), height());

	// Fit points
	if (m_fitModel)
	{
		painter.setPen(dataPen);
		double factor = 1.;
		if (m_measureCombo->currentIndex() == 1)
			factor = 0.5;
		for (int row = 0; row < m_fitModel->rowCount(); row++)
		{
			double position = m_fitModel->data(m_fitModel->index(row, 0)).toDouble()*Units::getUnit(UnitPosition).multiplier();
			double radius = factor*m_fitModel->data(m_fitModel->index(row, 1)).toDouble()*Units::getUnit(UnitWaist).multiplier();
			if ((position != 0.) && (radius > 0.))
			{
				QPointF point = QPointF(position, radius)*m_abs2view;
				painter.drawPoint(point);
				point = QPointF(position, -radius)*m_abs2view;
				painter.drawPoint(point);
			}
		}
	}
	for (int i = 0; i < m_bench.nFit(); i++)
	{
		Fit& fit = m_bench.fit(i);

	}


	// Rullers


	QPointF view_lastObjectLeft = view_right;
	QPointF abs_lastObjectLeft = abs_right;

	for (int row = model()->rowCount()-1; row >= 0; row--)
	{
		const Optics* currentOptics = m_bench.optics(row);

		//qDebug() << "Drawing element" << row << currentOptics->name().c_str() << currentOptics->position();

		QPointF abs_ObjectLeft = QPointF(currentOptics->position(), 0.);
		QPointF abs_ObjectRight = QPointF(currentOptics->position() + currentOptics->width(), 0.);
		QPointF view_ObjectLeft = abs_ObjectLeft*m_abs2view;
		QPointF view_ObjectRight = abs_ObjectRight*m_abs2view;
		QPointF view_ObjectCenter = (view_ObjectLeft + view_ObjectRight)/2.;

		QMatrix objectCenterMatrix(1., 0., 0., 1., view_ObjectCenter.x(), view_ObjectCenter.y());

		double view_defaultObjectHeight = objectRect().height();

		// Optics
//		if (view_ObjRect.intersects(view_paintArea))
		{
			if (selections->isRowSelected(row, rootIndex()))
				painter.setBrush(selectedLensBrush);
			else
				painter.setBrush(lensBrush);
			painter.setPen(lensPen);

			if (currentOptics->type() == LensType)
			{
				const Lens* lens = dynamic_cast<const Lens*>(currentOptics);
				if (lens->focal() < 0.)
					painter.drawPath(m_concaveLensPath*objectCenterMatrix);
				else
					painter.drawPath(m_convexLensPath*objectCenterMatrix);
			}
			else if (currentOptics->type() == FlatInterfaceType)
			{
				painter.drawPath(m_flatInterfacePath*objectCenterMatrix);
			}
			else if (currentOptics->type() == FlatMirrorType)
			{
				QRectF view_ObjectRect = objectRect();
				view_ObjectRect.moveCenter(view_ObjectCenter);
				painter.setBrush(ABCDBrush);
				painter.drawRect(view_ObjectRect);
			}
			else if (currentOptics->type() == CurvedMirrorType)
			{
				QRectF view_ObjectRect = objectRect();
				view_ObjectRect.moveCenter(view_ObjectCenter);
				painter.setBrush(ABCDBrush);
				painter.drawRect(view_ObjectRect);
			}
			else if (currentOptics->type() == CurvedInterfaceType)
			{
				const CurvedInterface* interface = dynamic_cast<const CurvedInterface*>(currentOptics);
				if (interface->surfaceRadius() < 0.)
					painter.drawPath(m_concaveInterfacePath*objectCenterMatrix);
				else
					painter.drawPath(m_convexInterfacePath*objectCenterMatrix);
			}
			else if (currentOptics->type() == GenericABCDType)
			{
				QRectF view_ObjectRect = objectRect();
				view_ObjectRect.moveCenter(view_ObjectCenter);
				view_ObjectRect.setLeft(view_ObjectLeft.x());
				view_ObjectRect.setRight(view_ObjectRight.x());
				painter.setBrush(ABCDBrush);
				painter.drawRect(view_ObjectRect);
			}

			if (currentOptics->type() != CreateBeamType)
			{
				painter.setPen(textPen);
				QString text = QString::fromUtf8(currentOptics->name().c_str());
				QRectF view_textRect(0., 0., 0., 0.);
				view_textRect.moveCenter(view_ObjectCenter - QPointF(0., view_defaultObjectHeight*0.7));
				view_textRect = painter.boundingRect(view_textRect, Qt::AlignCenter, text);
				painter.drawText(view_textRect, Qt::AlignCenter, text);
			}
		}

		// Input beam
		if (currentOptics->type() == CreateBeamType)
		{
			view_ObjectRight = view_left;
			abs_ObjectRight = abs_left;
		}

		// Beam
		QRectF abs_beamRange = abs_paintArea;
		abs_beamRange.setLeft(abs_ObjectRight.x());
		abs_beamRange.setRight(abs_lastObjectLeft.x());
		abs_beamRange = abs_beamRange.normalized();
		painter.setPen(beamPen);
		painter.setBrush(beamBrush);
		drawBeam(painter, m_bench.beam(row), abs_beamRange, row == model()->rowCount()-1);

/*		Beam cavityBeam = GBModel->cavityEigenBeam(row);
		if (GBModel->isCavityStable() && cavityBeam.isValid())
		{
			painter.setPen(cavityBeamPen);
			painter.setBrush(cavityBeamBrush);
			drawBeam(painter, cavityBeam, abs_beamRange);
		}
*/

		// Relative position
/*		if (row != 0)
		{
			double view_lastObjCenterX, abs_lastObjCenterX;
			if (row == model()->rowCount()-1)
			{
				view_lastObjCenterX = view_waistPos.x();
				abs_lastObjCenterX = currentBeam->waistPosition();
			}
			else
			{
				view_lastObjCenterX = view_lastObjCenter.x();
				abs_lastObjCenterX = abs_lastObjCenter.x();
			}
			qDebug() << "FootNote !!!";
			painter.setPen(textPen);
			QRectF view_textRect(0., 0., 3.*view_ObjRect.width(), view_ObjRect.width());
			view_textRect.moveCenter(QPointF((view_lastObjCenterX + view_ObjCenter.x())/2., view_ObjCenter.y() + view_ObjRect.height()*0.8));
			painter.drawText(view_textRect, Qt::AlignCenter, QString::number(round((abs_lastObjCenterX - abs_ObjCenter.x())*Units::getUnit(UnitPosition).divider())));
			painter.setPen(rullerPen);
			painter.drawLine(int(view_ObjCenter.x()), view_ObjCenter.y() + view_ObjRect.height()*0.7,
			                 int(view_ObjCenter.x()), view_ObjCenter.y() + view_ObjRect.height()*0.9);
		}
*/
		abs_lastObjectLeft = abs_ObjectLeft;
		view_lastObjectLeft = view_ObjectLeft;

	} // End optics for loop

	// Target Beam
	const Beam& targetBeam = m_bench.targetBeam();
	if (m_showTargetBeam && targetBeam.isValid())
	{
		QRectF abs_beamRange = abs_paintArea;
		painter.setPen(targetBeamPen);
		painter.setBrush(targetBeamBrush);
		drawBeam(painter, targetBeam, abs_beamRange);
	}

/*	for (int i = 0; i < 550; i++)
	{
		painter.setPen(QPen(wavelengthColor(double((i+300)*1e-9)), 1));
		painter.drawLine(i, 10, i, 100);
	}
*/
}
