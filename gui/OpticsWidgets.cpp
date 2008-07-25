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

#include "gui/OpticsWidgets.h"
#include "gui/OpticsView.h"
#include "gui/Unit.h"

#include <QWheelEvent>
#include <QListWidget>
#include <QCheckBox>
#include <QSettings>
#include <QDebug>

/////////////////////////////////////////////////
// RullerSlider

RullerSlider::RullerSlider(OpticsView* view, bool zoomScroll)
	: QScrollBar(view)
{
	m_view = view;
	m_zoomScroll = zoomScroll;
}

double RullerSlider::rullerScale() const
{
	int length = maximum() + pageStep() - minimum();
	double scale = double(length);
	if (orientation() == Qt::Horizontal)
		scale /= m_view->scene()->width();
	else
		scale /= m_view->scene()->height();

	return scale;
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

	double scale = rullerScale();
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
		if (abs((int(round(x/spacing)) % 10)) == 5)
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
	m_update = true;
}

void OpticsViewProperties::on_doubleSpinBox_Width_valueChanged(double value)
{
	if (m_update)
		m_view->setHorizontalRange(value*Units::getUnit(UnitPosition).multiplier());
}

void OpticsViewProperties::on_doubleSpinBox_Height_valueChanged(double value)
{
	if (m_update)
		m_view->setVerticalRange(value*Units::getUnit(UnitWaist).multiplier());
}

void OpticsViewProperties::on_doubleSpinBox_Origin_valueChanged(double value)
{
	if (m_update)
		m_view->setOrigin(value*Units::getUnit(UnitPosition).multiplier());
}

void OpticsViewProperties::setViewWidth(double width)
{
	m_update = false;
	doubleSpinBox_Width->setValue(width*Units::getUnit(UnitPosition).divider());
	m_update = true;
}

void OpticsViewProperties::setViewHeight(double height)
{
	m_update = false;
	doubleSpinBox_Height->setValue(height*Units::getUnit(UnitWaist).divider());
	m_update = true;
}

void OpticsViewProperties::setViewOrigin(double origin)
{
	m_update = false;
	doubleSpinBox_Origin->setValue(origin*Units::getUnit(UnitPosition).divider());
	m_update = true;
}

/////////////////////////////////////////////////
// CornerWidget

CornerWidget::CornerWidget(QColor backgroundColor, const char* pixmapName, QWidget* widget, QWidget* parent)
	: QWidget(parent)
	, m_backgroundColor(backgroundColor)
	, m_widget(widget)
{
	m_pixmap = QPixmap(pixmapName);
	setFixedSize(m_pixmap.rect().size());
}

void CornerWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	QBrush backgroundBrush(m_backgroundColor);
	QPen backgroundPen(m_backgroundColor);
	painter.setBrush(backgroundBrush);
	painter.setPen(backgroundPen);
	painter.drawRect(rect());
	painter.drawPixmap(m_pixmap.rect(), m_pixmap, m_pixmap.rect());
}

void CornerWidget::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);

	m_widget->setVisible(!m_widget->isVisible());
}

/////////////////////////////////////////////////
// StatusConfigWidget

enum BeamPropertyType {BeamPosition = 0, BeamRadius, BeamDiameter, BeamCurvature, BeamGouyPhase,
                       BeamDistanceToWaist, BeamParameter, BeamIndex};

class ConfigItem
{
public:
	ConfigItem() {};
	ConfigItem(QString a_fullName, QString a_shortName, UnitType a_unit)
		: fullName(a_fullName), shortName(a_shortName),  unit(a_unit) {};
	QString fullName;
	QString shortName;
	UnitType unit;
};

QMap<BeamPropertyType, ConfigItem> configItems;

StatusConfigWidget::StatusConfigWidget(QWidget* parent)
	: QWidget(parent)
{
	setWindowTitle(tr("Configure status bar"));

	configItems.clear();
	configItems.insert(BeamPosition, ConfigItem(tr("Position"), tr("z"), UnitPosition));
	configItems.insert(BeamRadius, ConfigItem(tr("Beam radius"), tr("w"), UnitWaist));
	configItems.insert(BeamDiameter, ConfigItem(tr("Beam diameter"), tr("2w"), UnitWaist));
	configItems.insert(BeamCurvature, ConfigItem(tr("Beam curvature"), tr("R"), UnitCurvature));
	configItems.insert(BeamGouyPhase, ConfigItem(tr("Gouy phase"), tr("ζ"), UnitPhase));
	configItems.insert(BeamDistanceToWaist, ConfigItem(tr("Distance to waist"), tr("z-zw"), UnitPosition));
	configItems.insert(BeamParameter, ConfigItem(tr("Beam parameter"), tr("q"), UnitPosition));
	configItems.insert(BeamIndex, ConfigItem(tr("Index"), tr("n"), UnitLess));

	m_propertyListWidget = new QListWidget;
	m_propertyListWidget->setDragDropMode(QAbstractItemView::InternalMove);

	m_symbolCheck = new QCheckBox("Display full name");

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget(new QLabel("Check and sort properties:"));
	layout->addWidget(m_propertyListWidget);
	layout->addWidget(m_symbolCheck);
	setLayout(layout);

	readSettings();
}

StatusConfigWidget::~StatusConfigWidget()
{
	writeSettings();
}

void StatusConfigWidget::readSettings()
{
	QSettings settings;
	QList<BeamPropertyType> propertyList;
	QList<bool> checkList;
	QList<QVariant> propertySettings = settings.value("StatusConfigWidget/properties").toList();
	QList<QVariant> checkSettings = settings.value("StatusConfigWidget/checks").toList();

	// Read settings
	if ((propertySettings.size() == configItems.size()) && (checkSettings.size() ==  configItems.size()))
		for (int i = 0; i < configItems.size(); i++)
		{
			propertyList << BeamPropertyType(propertySettings[i].toInt());
			checkList << checkSettings[i].toBool();
		}
	// Default values
	else
	{
		propertyList << BeamPosition << BeamRadius << BeamDiameter << BeamCurvature
		             << BeamGouyPhase << BeamDistanceToWaist << BeamParameter << BeamIndex;
		checkList << true << true << false << true
		          << false << false << false << false;
	}

	m_propertyListWidget->clear();
	for (int i = 0; i < configItems.size(); i++)
	{
		QListWidgetItem* widgetItem = new QListWidgetItem;
		widgetItem->setText(configItems[propertyList[i]].fullName + " (" + configItems[propertyList[i]].shortName + ")");
		widgetItem->setCheckState(checkList[i] ? Qt::Checked : Qt::Unchecked);
		widgetItem->setData(Qt::UserRole, propertyList[i]);
		m_propertyListWidget->addItem(widgetItem);
	}

	m_symbolCheck->setCheckState(Qt::CheckState(settings.value("StatusConfigWidget/fullName", Qt::Checked).toInt()));
}

void StatusConfigWidget::writeSettings()
{
	QList<QVariant> propertySettings;
	QList<QVariant> checkSettings;

	for (int i = 0; i < m_propertyListWidget->count(); i++)
	{
		propertySettings << m_propertyListWidget->item(i)->data(Qt::UserRole);
		checkSettings << (m_propertyListWidget->item(i)->checkState() == Qt::Checked);
	}

	QSettings settings;
	settings.setValue("StatusConfigWidget/properties", propertySettings);
	settings.setValue("StatusConfigWidget/checks", checkSettings);
	settings.setValue("StatusConfigWidget/fullName", m_symbolCheck->checkState());
}

/////////////////////////////////////////////////
// StatusWidget

StatusWidget::StatusWidget(QStatusBar* statusBar)
	: QWidget(statusBar)
	, m_statusBar(statusBar)
{
	m_configWidget = new StatusConfigWidget(this);
	m_configWidget->setWindowFlags(Qt::Window);
	CornerWidget* corner = new CornerWidget(Qt::transparent, ":/images/preferences-system.png",
	                       m_configWidget, this);
	m_label = new QLabel;
	QHBoxLayout* layout = new QHBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(corner);
	layout->addWidget(m_label);
	setLayout(layout);
}

void StatusWidget::showBeamInfo(const Beam& beam, double z)
{
	m_statusBar->clearMessage();

	bool fullName = m_configWidget->m_symbolCheck->checkState() == Qt::Checked;
	QString text;

	for (int i = 0; i <  m_configWidget->m_propertyListWidget->count(); i++)
	{
		if (m_configWidget->m_propertyListWidget->item(i)->checkState() != Qt::Checked)
			continue;
		BeamPropertyType type = BeamPropertyType(m_configWidget->m_propertyListWidget->item(i)->data(Qt::UserRole).toInt());
		ConfigItem property = configItems[type];
		text += fullName ? (property.fullName + tr(": ")) : (property.shortName + tr(" = "));
		double value = 0.;
		if (type == BeamPosition)
			value = z;
		else if (type == BeamRadius)
			value = beam.radius(z);
		else if (type == BeamDiameter)
			value = 2.*beam.radius(z);
		else if (type == BeamCurvature)
			value = beam.curvature(z);
		else if (type == BeamGouyPhase)
			value = beam.gouyPhase(z);
		else if ((type == BeamDistanceToWaist) || (type == BeamParameter))
			value = z - beam.waistPosition();
		else if (type == BeamIndex)
			value = beam.index();

		value *=  Units::getUnit(property.unit).divider();
		text += QString::number(value, 'f', 2) + Units::getUnit(property.unit).string();

		if (type == BeamParameter)
		{
			value = beam.rayleigh()*Units::getUnit(UnitRayleigh).divider();
			text += " + i" + QString::number(value, 'f', 2) + Units::getUnit(UnitRayleigh).string();
		}

		text += "    ";

	}
	m_label->setText(text);
}
