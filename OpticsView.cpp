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

#include "OpticsView.h"
#include "GaussianBeamModel.h"
#include "GaussianBeam.h"
#include "Unit.h"

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

OpticsView::OpticsView(QWidget *parent)
	: QAbstractItemView(parent)
{
	m_statusLabel = 0;
	m_fitModel = 0;

	m_hOffset = -0.1;
	m_hRange = 0.7;
	m_vRange = 5e-3;

	computeTranformMatrix();
	computePaths();

	m_active_object = -1;
	m_active_object_offset = 0.;
	m_statusLabel = 0;

	m_showTargetWaist = false;

	setMouseTracking(true);
}

void OpticsView::computePaths()
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

void OpticsView::computeTranformMatrix()
{
	m_abs2view = QMatrix(hScale(), 0., 0., vScale(), -m_hOffset*hScale(), m_vRange/2.*vScale());
	m_view2abs = m_abs2view.inverted();
}

QRectF OpticsView::objectRect() const
{
	double h = double(height());
	return QRectF(QPointF(-h*0.03, -h*0.2), QSizeF(2.*h*0.03, 2.*h*0.2));
}

double OpticsView::vScale() const
{
	return double(height())/m_vRange;
}

double OpticsView::hScale() const
{
	return double(width())/m_hRange;
}

void OpticsView::setHRange(double hRange)
{
	m_hRange = hRange;
	computeTranformMatrix();
	computePaths();
	viewport()->update();
}

void OpticsView::setVRange(double vRange)
{
	m_vRange = vRange;
	computeTranformMatrix();
	computePaths();
	viewport()->update();
}

void OpticsView::setHOffset(double hOffset)
{
	m_hOffset = hOffset;
	computeTranformMatrix();
	computePaths();
	viewport()->update();
}

void OpticsView::setTargetWaist(const Beam& targetBeam, bool showTargetWaist)
{
	m_showTargetWaist =  showTargetWaist;
	m_targetBeam = targetBeam;
	viewport()->update();
}

///////////////////////

void OpticsView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	QAbstractItemView::dataChanged(topLeft, bottomRight);
	viewport()->update();
}

QRect OpticsView::visualRect(const QModelIndex& /*index*/) const
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

void OpticsView::scrollTo(const QModelIndex& /*index*/, ScrollHint /*hint*/)
{
}

QModelIndex OpticsView::indexAt(const QPoint& point) const
{
	for (int row = 0; row < model()->rowCount(); ++row)
	{
		const Optics& currentOptics = dynamic_cast<GaussianBeamModel*>(model())->optics(row);

		QPointF abs_ObjectLeft = QPointF(currentOptics.position(), 0.);
		QPointF abs_ObjectRight = QPointF(currentOptics.position() + currentOptics.width(), 0.);
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

		QPointF abs_objCenter = QPointF(currentOptics.position(), 0.);
		QPointF distFromObjCenter = QPointF(point) - abs_objCenter*m_abs2view;
		if ((currentOptics.type() != CreateBeamType) &&
		    view_ObjectRect.contains(QPointF(point)))
			return model()->index(row, 0);
	}

	return QModelIndex();
}

QModelIndex OpticsView::moveCursor(QAbstractItemView::CursorAction /*cursorAction*/,
						Qt::KeyboardModifiers /*modifiers*/)
{
	return QModelIndex();
}

int OpticsView::horizontalOffset() const
{
	return 0;
}

int OpticsView::verticalOffset() const
{
	return 0;
}

bool OpticsView::isIndexHidden(const QModelIndex& /*index*/) const
{
	return false;
}

void OpticsView::setSelection(const QRect&, QItemSelectionModel::SelectionFlags /*command*/)
{
}

QRegion OpticsView::visualRegionForSelection(const QItemSelection& selection) const
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

void OpticsView::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
	computeTranformMatrix();
	computePaths();
}

void OpticsView::mousePressEvent(QMouseEvent* event)
{
	QAbstractItemView::mousePressEvent(event);

	QModelIndex index = indexAt(event->pos());
	if (index.isValid())
	{
		const Optics& currentOptics = dynamic_cast<GaussianBeamModel*>(model())->optics(index.row());
		QPointF abs_ObjectLeft = QPointF(currentOptics.position(), 0.);
		QPointF view_ObjectLeft = abs_ObjectLeft*m_abs2view;

		m_active_object_offset = double(event->pos().x()) - view_ObjectLeft.x();
		m_active_object = index.row();
	}

	if (m_active_object != -1)
		mouseMoveEvent(event);
}

void OpticsView::mouseMoveEvent(QMouseEvent* event)
{
	if (m_active_object >= 0)
	{
		QPointF pos = QPointF(event->pos()) - QPointF(m_active_object_offset, 0.);
		dynamic_cast<GaussianBeamModel*>(model())->setOpticsPosition(m_active_object, (pos*m_view2abs).x());
	}
	else if (m_statusLabel)
	{
		double abs_position = (QPointF(event->pos())*m_view2abs).x();
		QString text = tr("Position: ") + QString::number(abs_position*Units::getUnit(UnitPosition).divider(), 'f', 2) + " " + tr("mm") + "    ";

		for (int row = model()->rowCount() - 1; row >= 0; row--)
		{
			const Optics& currentOptics = dynamic_cast<GaussianBeamModel*>(model())->optics(row);

			QPointF abs_objectLeft = QPointF(currentOptics.position(), 0.);
			QPointF abs_objectRight = QPointF(currentOptics.position() + currentOptics.width(), 0.);
			// Check if we are inside the optics
			if ((currentOptics.width() > 0.) && (abs_objectLeft.x() < abs_position) && (abs_objectRight.x() > abs_position))
				break;
			// Check if we are lookgin at the mode created by this optics
			if ((currentOptics.type() != CreateBeamType) && (abs_objectRight.x() < abs_position) ||
				(currentOptics.type() == CreateBeamType))
			{
				const Beam& beam = dynamic_cast<GaussianBeamModel*>(model())->beam(row);
				text += tr("Beam radius: ") + QString::number(beam.radius(abs_position)*Units::getUnit(UnitWaist).divider(), 'f', 2) + " " +  tr("µm") + "    " +
				        tr("Beam curvature: ") + QString::number(beam.curvature(abs_position)*Units::getUnit(UnitCurvature).divider(), 'f', 2) +  " " + tr("mm") + "    ";
				break;
			}
		}
		m_statusLabel->setText(text);
	}
	QAbstractItemView::mouseMoveEvent(event);
}

void OpticsView::mouseReleaseEvent(QMouseEvent* event)
{
	QAbstractItemView::mouseReleaseEvent(event);
	m_active_object = -1;
}

void OpticsView::drawBeam(QPainter& painter, const Beam& beam, const QRectF& abs_beamRange, bool drawText)
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
		Approximation approximation;
		approximation.minZ = abs_beamRange.left();
		approximation.maxZ = abs_beamRange.right();
		approximation.resolution = 1./vScale();

		if (approximation.minZ < approximation.maxZ)
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
		painter.setPen(textPen);
		QPointF view_waistTop(view_waistPos.x(), view_waistPos.y() - beam.waist()*vScale());
		painter.drawLine(view_waistPos, view_waistTop);
		QString text; text.setNum(round(beam.waist()*Units::getUnit(UnitWaist).divider()));
		QRectF view_textRect(0., 0., 100., 15.);
		view_textRect.moveCenter(view_waistTop - QPointF(0., 15.));
		painter.drawText(view_textRect, Qt::AlignHCenter | Qt::AlignBottom, text);
	}
}


void OpticsView::paintEvent(QPaintEvent* event)
{
	//qDebug() << "Repaint" << property("Wavelength").toDouble();

	GaussianBeamModel* GBModel = dynamic_cast<GaussianBeamModel*>(model());

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

	// Rullers


	QPointF view_lastObjectLeft = view_right;
	QPointF abs_lastObjectLeft = abs_right;

	for (int row = model()->rowCount()-1; row >= 0; row--)
	{
		const Optics& currentOptics = GBModel->optics(row);

		//qDebug() << "Drawing element" << row << selections->isRowSelected(row, rootIndex());

		QPointF abs_ObjectLeft = QPointF(currentOptics.position(), 0.);
		QPointF abs_ObjectRight = QPointF(currentOptics.position() + currentOptics.width(), 0.);
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

			if (currentOptics.type() == LensType)
			{
				const Lens& lens = dynamic_cast<const Lens&>(currentOptics);
				if (lens.focal() < 0.)
					painter.drawPath(m_concaveLensPath*objectCenterMatrix);
				else
					painter.drawPath(m_convexLensPath*objectCenterMatrix);
			}
			else if (currentOptics.type() == FlatInterfaceType)
			{
				painter.drawPath(m_flatInterfacePath*objectCenterMatrix);
			}
			else if (currentOptics.type() == FlatMirrorType)
			{
				QRectF view_ObjectRect = objectRect();
				view_ObjectRect.moveCenter(view_ObjectCenter);
				painter.setBrush(ABCDBrush);
				painter.drawRect(view_ObjectRect);
			}
			else if (currentOptics.type() == CurvedMirrorType)
			{
				QRectF view_ObjectRect = objectRect();
				view_ObjectRect.moveCenter(view_ObjectCenter);
				painter.setBrush(ABCDBrush);
				painter.drawRect(view_ObjectRect);
			}
			else if (currentOptics.type() == CurvedInterfaceType)
			{
				const CurvedInterface& interface = dynamic_cast<const CurvedInterface&>(currentOptics);
				if (interface.surfaceRadius() < 0.)
					painter.drawPath(m_concaveInterfacePath*objectCenterMatrix);
				else
					painter.drawPath(m_convexInterfacePath*objectCenterMatrix);
			}
			else if (currentOptics.type() == GenericABCDType)
			{
				QRectF view_ObjectRect = objectRect();
				view_ObjectRect.moveCenter(view_ObjectCenter);
				view_ObjectRect.setLeft(view_ObjectLeft.x());
				view_ObjectRect.setRight(view_ObjectRight.x());
				painter.setBrush(ABCDBrush);
				painter.drawRect(view_ObjectRect);
			}

			if (currentOptics.type() != CreateBeamType)
			{
				painter.setPen(textPen);
				QString text = QString::fromUtf8(currentOptics.name().c_str());
				QRectF view_textRect(0., 0., 0., 0.);
				view_textRect.moveCenter(view_ObjectCenter - QPointF(0., view_defaultObjectHeight*0.7));
				view_textRect = painter.boundingRect(view_textRect, Qt::AlignCenter, text);
				painter.drawText(view_textRect, Qt::AlignCenter, text);
			}
		}

		// Input beam
		if (currentOptics.type() == CreateBeamType)
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
		drawBeam(painter, GBModel->beam(row), abs_beamRange, row == model()->rowCount()-1);

		Beam cavityBeam = GBModel->cavityEigenBeam(row);
		if (GBModel->isCavityStable() && cavityBeam.isValid())
		{
			painter.setPen(cavityBeamPen);
			painter.setBrush(cavityBeamBrush);
			drawBeam(painter, cavityBeam, abs_beamRange);
		}


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
	qDebug() << m_showTargetWaist << m_targetBeam.isValid() << m_targetBeam.waist() <<  m_targetBeam.waistPosition();
	if (m_showTargetWaist && m_targetBeam.isValid())
	{
		m_targetBeam.setWavelength(dynamic_cast<GaussianBeamModel*>(model())->wavelength());
		QRectF abs_beamRange = abs_paintArea;
		painter.setPen(targetBeamPen);
		painter.setBrush(targetBeamBrush);
		drawBeam(painter, m_targetBeam, abs_beamRange);
	}

/*	for (int i = 0; i < 550; i++)
	{
		painter.setPen(QPen(wavelengthColor(double((i+300)*1e-9)), 1));
		painter.drawLine(i, 10, i, 100);
	}
*/

}
