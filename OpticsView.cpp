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
	m_statusLabel = 0;

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
		const Optics* currentOptics = dynamic_cast<GaussianBeamModel*>(model())->optics(row);

		QPointF abs_objCenter = QPointF(currentOptics->position(), 0.);
		QPointF distFromObjCenter = QPointF(point) - abs_objCenter*m_abs2view;
		if ((currentOptics->type() != CreateBeamType) &&
		    objectRect().contains(distFromObjCenter))
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
		m_active_object = index.row();

	if (m_active_object != -1)
		mouseMoveEvent(event);
}

void OpticsView::mouseMoveEvent(QMouseEvent* event)
{
	if (m_active_object >= 0)
		dynamic_cast<GaussianBeamModel*>(model())->setOpticsPosition(m_active_object, (QPointF(event->pos())*m_view2abs).x());
	else if (m_statusLabel)
		for (int row = model()->rowCount() - 1; row >= 0; row--)
		{
			const Optics* currentOptics = dynamic_cast<GaussianBeamModel*>(model())->optics(row);

			QPointF abs_objCenter = QPointF(currentOptics->position(), 0.);
			double abs_position = (QPointF(event->pos())*m_view2abs).x();
			if ((currentOptics->type() != CreateBeamType) && (abs_objCenter.x() < abs_position) ||
				(currentOptics->type() == CreateBeamType))
			{
				const Beam& beam = dynamic_cast<GaussianBeamModel*>(model())->beamList()[row];
				QString text = tr("Position: ") + QString::number(abs_position*Units::getUnit(UnitPosition).divider(), 'f', 2) + tr("mm") + "    " +
				               tr("Beam radius: ") + QString::number(beam.radius(abs_position)*Units::getUnit(UnitWaist).divider()) + tr("µm") + "    " +
				               tr("Beam curvature: ") + QString::number(beam.curvature(abs_position)*Units::getUnit(UnitCurvature).divider()) + tr("mm") + "    ";
				m_statusLabel->setText(text);
				break;
			}
		}

	QAbstractItemView::mouseMoveEvent(event);
}

void OpticsView::mouseReleaseEvent(QMouseEvent* event)
{
	QAbstractItemView::mouseReleaseEvent(event);
	m_active_object = -1;
}

void OpticsView::paintEvent(QPaintEvent* event)
{
	qDebug() << "Repaint" << property("Wavelength").toDouble();

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
	QColor beamColor = wavelengthColor(m_wavelength);
	beamColor.setAlpha(200);
	QPen beamPen(beamColor, 0);
	QBrush beamBrush(beamColor);

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


	QPointF view_lastObjCenter = view_right;
	QPointF abs_lastObjCenter = abs_right;

	const QList<Optics*>& optics = dynamic_cast<GaussianBeamModel*>(model())->opticsList();
	const QList<Beam>& beams = dynamic_cast<GaussianBeamModel*>(model())->beamList();

	for (int row = model()->rowCount()-1; row >= 0; row--)
	{
		const Optics* currentOptics = optics[row];
		const Beam* currentBeam = &(beams[row]);

		qDebug() << "Drawing element" << row << selections->isRowSelected(row, rootIndex());

		QPointF abs_ObjCenter = QPointF(currentOptics->position(), 0.);
		QPointF view_ObjCenter = abs_ObjCenter*m_abs2view;

		QMatrix objectCenterMatrix(1., 0., 0., 1., view_ObjCenter.x(), view_ObjCenter.y());

		// Optics
		QRectF view_ObjRect = objectRect();
		view_ObjRect.moveCenter(view_ObjCenter);
		if (view_ObjRect.intersects(view_paintArea))
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
			else if (currentOptics->type() == CurvedInterfaceType)
			{
				const CurvedInterface* interface = dynamic_cast<const CurvedInterface*>(currentOptics);
				if (interface->surfaceRadius() < 0.)
					painter.drawPath(m_concaveInterfacePath*objectCenterMatrix);
				else
					painter.drawPath(m_convexInterfacePath*objectCenterMatrix);
			}

			if (currentOptics->type() != CreateBeamType)
			{
				painter.setPen(textPen);
				QRectF view_textRect(0., 0., 3.*view_ObjRect.width(), view_ObjRect.width());
				view_textRect.moveCenter(view_ObjCenter - QPointF(0., view_ObjRect.height()*0.7));
				painter.drawText(view_textRect, Qt::AlignCenter, QString::fromUtf8(currentOptics->name().c_str()));
			}
		}

		// Input beam
		if (currentOptics->type() == CreateBeamType)
		{
			view_ObjCenter = view_left;
			abs_ObjCenter = abs_left;
		}

		// Beam
		QPointF abs_waistPos = QPointF(currentBeam->waistPosition(), 0.);
		QPointF view_waistPos = abs_waistPos*m_abs2view;
		double max_ray = 4.;
		double max_w0  = sqrt(1. + sqr(max_ray));

		painter.setPen(beamPen);
		painter.setBrush(beamBrush);
		QRectF abs_objInterRect = abs_paintArea;
		abs_objInterRect.setLeft(abs_ObjCenter.x());
		abs_objInterRect.setRight(abs_lastObjCenter.x());
		QRectF view_objInterRect = m_abs2view.mapRect(abs_objInterRect);
		QRectF abs_waistRect(0., 0., 2.*currentBeam->rayleigh()*max_ray, 2.*currentBeam->waist()*max_w0);
		abs_waistRect.moveCenter(abs_waistPos);
		if ((currentBeam->waist()*vScale() > 1.) &&
		    abs_objInterRect.intersects(abs_waistRect))
		{
			Approximation approximation;
			approximation.minZ = min(abs_ObjCenter.x(), abs_lastObjCenter.x());
			approximation.maxZ = max(abs_ObjCenter.x(), abs_lastObjCenter.x());
			approximation.resolution = 1./vScale();

			if (approximation.minZ < approximation.maxZ)
			{
				qDebug() << " Drawing waist" << approximation.minZ << approximation.maxZ;
				QPolygonF beamPolygonUp, beamPolygonDown;
				for (double z = approximation.minZ; z <= approximation.maxZ; z = currentBeam->approxNextPosition(z, approximation))
				{
					qDebug() << z;
					beamPolygonUp.append(QPointF(z, currentBeam->radius(z)));
					beamPolygonDown.prepend(QPointF(z, -currentBeam->radius(z)));
					if (z == approximation.maxZ)
						break;
				}
				beamPolygonUp.append(QPointF(approximation.maxZ, currentBeam->radius(approximation.maxZ)));
				beamPolygonDown.prepend(QPointF(approximation.maxZ, -currentBeam->radius(approximation.maxZ)));
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
			double sgn = sign((abs_lastObjCenter.x() - abs_waistPos.x())*(abs_ObjCenter.x() - abs_waistPos.x()));
			double view_rightRadius = currentBeam->radius(abs_lastObjCenter.x())*vScale();
			double view_leftRadius = currentBeam->radius(abs_ObjCenter.x())*vScale();

			QPolygonF ray;
			ray << QPointF(view_ObjCenter.x(), view_left.y() + view_leftRadius)
				<< QPointF(view_lastObjCenter.x(), view_left.y() + sgn*view_rightRadius)
				<< QPointF(view_lastObjCenter.x(), view_left.y() - sgn*view_rightRadius)
				<< QPointF(view_ObjCenter.x(), view_left.y() - view_leftRadius);
			painter.drawConvexPolygon(ray);
		}

		// Waist
		if (row == model()->rowCount()-1)
		{
			painter.setPen(textPen);
			QPointF view_waistTop(view_waistPos.x(), view_waistPos.y() - currentBeam->waist()*vScale());
			painter.drawLine(view_waistPos, view_waistTop);
			QString text; text.setNum(int(currentBeam->waist()*Units::getUnit(UnitWaist).divider()));
			QRectF view_textRect(0., 0., 100., 15.);
			view_textRect.moveCenter(view_waistTop - QPointF(0., 15.));
			painter.drawText(view_textRect, Qt::AlignHCenter | Qt::AlignBottom, text);
		}


		abs_lastObjCenter = abs_ObjCenter;
		view_lastObjCenter = view_ObjCenter;
	}

/*	for (int i = 0; i < 550; i++)
	{
		painter.setPen(QPen(wavelengthColor(double((i+300)*1e-9)), 1));
		painter.drawLine(i, 10, i, 100);
	}
*/

}
