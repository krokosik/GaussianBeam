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

#ifndef OPTICSWIDGETS_H
#define OPTICSWIDGETS_H

#include "ui_OpticsViewProperties.h"

#include <QScrollBar>

class OpticsView;

class RullerSlider : public QScrollBar
{
public:
	RullerSlider(OpticsView* view, bool zoomScroll = false);

protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void wheelEvent(QWheelEvent* event);

private:
	void drawGraduation(QPainter& painter, double position, double fractionalLength);

private:
	OpticsView* m_view;
	bool m_zoomScroll;
};

class OpticsViewProperties : public QWidget, private Ui::OpticsViewProperties
{
	Q_OBJECT

public:
	OpticsViewProperties(OpticsView* view);

public:
	void setViewWidth(double width);
	void setViewHeight(double height);

// UI slots
protected slots:
	void on_doubleSpinBox_Width_valueChanged(double value);
	void on_doubleSpinBox_Height_valueChanged(double value);

private:
	OpticsView* m_view;
};

class CornerWidget : public QWidget
{
public:
	CornerWidget(OpticsView* view);

protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);

private:
	OpticsView* m_view;
};

#endif
