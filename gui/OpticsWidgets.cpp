/* This file is part of the Gaussian Beam project
   Copyright (C) 2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>

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

#include "OpticsWidgets.h"
#include "OpticsView.h"
#include "gui/Unit.h"

#include <QWheelEvent>
#include <QDebug>

/////////////////////////////////////////////////
// RullerSlider

RullerSlider::RullerSlider(OpticsView* view, bool zoomScroll)
	: QScrollBar(view)
{
	m_view = view;
	m_zoomScroll = zoomScroll;
}

void RullerSlider::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
}

void RullerSlider::wheelEvent(QWheelEvent* event)
{
	if (m_zoomScroll)
	{
		int numDegrees = event->delta()/8;
		int numSteps = numDegrees/15;
		double zoomFactor = pow(1.2, numSteps);
		if (orientation() == Qt::Horizontal)
			m_view->setHorizontalRange(m_view->horizontalRange()*zoomFactor);
		else
			m_view->setVerticalRange(m_view->verticalRange()*zoomFactor);
	}
	else
		QScrollBar::wheelEvent(event);
}

void RullerSlider::drawGraduation(QPainter& painter, double position, double fractionalLength)
{
	if (orientation() == Qt::Horizontal)
		painter.drawLine(position, 0., position, height()*fractionalLength);
	else
		painter.drawLine(0., position, width()*fractionalLength, position);
}

void RullerSlider::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	QColor backgroundColor(245, 245, 200);
	QBrush backgroundBrush(backgroundColor);
	QPen backgroundPen(backgroundColor);
	QPen mainTickPen(Qt::black);
	QPen secondTickPen(Qt::lightGray);

	int length = maximum() + pageStep() - minimum();
	double scale = double(length);
	if (orientation() == Qt::Horizontal)
		scale /= m_view->scene()->width();
	else
		scale /= m_view->scene()->height();
	double spacing = (orientation() == Qt::Horizontal ? 0.01 : 0.00005);
	int lastStep = int(double(value() + pageStep())/(scale*spacing)) + 1;
	int firstStep = int(double(value())/(scale*spacing)) - 1;

	painter.setBrush(backgroundBrush);
	painter.setPen(backgroundPen);
	painter.drawRect(rect());
	painter.setPen(secondTickPen);
	for (double x = double(lastStep)*spacing; x >= double(firstStep)*spacing; x -= spacing)
	{
		double pos = x*scale - double(value());
		drawGraduation(painter, pos, 0.4);
		if ((int(round(x/spacing)) % 10) == 5)
			drawGraduation(painter, pos, 0.65);
		else if ((int(round(x/spacing)) % 10) == 0)
		{
			painter.setPen(mainTickPen);
			drawGraduation(painter, pos, 0.8);
			QString text;
			text.setNum(round(x*1000.));
			QRectF textRect(0., 0., 0., 0.);
			textRect.moveCenter(QPointF(pos + 2., height()/2.));
			textRect = painter.boundingRect(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
			painter.drawText(textRect, Qt::AlignCenter, text);
			painter.setPen(secondTickPen);
		}
	}
}

/////////////////////////////////////////////////
// OpticsViewProperties

OpticsViewProperties::OpticsViewProperties(OpticsView* view)
	: QWidget(view)
{
	m_view = view;
	setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
}

void OpticsViewProperties::on_doubleSpinBox_Width_valueChanged(double value)
{
	m_view->setHorizontalRange(value*Units::getUnit(UnitPosition).multiplier());
}

void OpticsViewProperties::on_doubleSpinBox_Height_valueChanged(double value)
{
	m_view->setVerticalRange(value*Units::getUnit(UnitWaist).multiplier());
}

void OpticsViewProperties::setViewWidth(double width)
{
	doubleSpinBox_Width->setValue(width*Units::getUnit(UnitPosition).divider());
}

void OpticsViewProperties::setViewHeight(double height)
{
	doubleSpinBox_Height->setValue(height*Units::getUnit(UnitWaist).divider());
}

/////////////////////////////////////////////////
// CornerWidget

CornerWidget::CornerWidget(OpticsView* view)
	: QWidget(view)
{
	m_view = view;
}

void CornerWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	QColor backgroundColor(245, 245, 200);
	QBrush backgroundBrush(backgroundColor);
	QPen backgroundPen(backgroundColor);
	painter.setBrush(backgroundBrush);
	painter.setPen(backgroundPen);
	painter.drawRect(rect());
	QPixmap pixmap(":/images/zoom-best-fit.png");
	painter.drawPixmap(rect(), pixmap, pixmap.rect());
}

void CornerWidget::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);

	m_view->showProperties(!m_view->propertiesVisible());
}
