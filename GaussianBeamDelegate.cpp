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

#include "GaussianBeamDelegate.h"
#include "GaussianBeamModel.h"
#include "Unit.h"

#include <QtGui>

ABCDWidget::ABCDWidget(QWidget* parent)
	: QWidget(parent)
{
	m_ADoubleSpinBox.setPrefix("A = ");
	m_ADoubleSpinBox.setMinimum(-Unit::infinity);
	m_ADoubleSpinBox.setMaximum(Unit::infinity);
	m_BDoubleSpinBox.setPrefix("B = ");
	m_BDoubleSpinBox.setMinimum(-Unit::infinity);
	m_BDoubleSpinBox.setMaximum(Unit::infinity);
	m_CDoubleSpinBox.setPrefix("C = ");
	m_CDoubleSpinBox.setMinimum(-Unit::infinity);
	m_CDoubleSpinBox.setMaximum(Unit::infinity);
	m_DDoubleSpinBox.setPrefix("D = ");
	m_DDoubleSpinBox.setMinimum(-Unit::infinity);
	m_DDoubleSpinBox.setMaximum(Unit::infinity);
	m_BDoubleSpinBox.setSuffix(Units::getUnit(UnitABCD).string("m"));
	m_CDoubleSpinBox.setSuffix(" /" + Units::getUnit(UnitABCD).string("m", false));
	m_widthDoubleSpinBox.setPrefix("width = ");
	m_widthDoubleSpinBox.setSuffix(Units::getUnit(UnitWidth).string("m"));
	m_widthDoubleSpinBox.setMinimum(0.);
	m_widthDoubleSpinBox.setMaximum(Unit::infinity);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(&m_ADoubleSpinBox);
	layout->addWidget(&m_BDoubleSpinBox);
	layout->addWidget(&m_CDoubleSpinBox);
	layout->addWidget(&m_DDoubleSpinBox);
	layout->addWidget(&m_widthDoubleSpinBox);
	setLayout(layout);
}

CurvedInterfaceWidget::CurvedInterfaceWidget(QWidget* parent)
	: QWidget(parent)
{
	m_surfaceRadiusDoubleSpinBox.setPrefix("R = ");
	m_surfaceRadiusDoubleSpinBox.setMinimum(-Unit::infinity);
	m_surfaceRadiusDoubleSpinBox.setMaximum(Unit::infinity);
	m_surfaceRadiusDoubleSpinBox.setSuffix(Units::getUnit(UnitCurvature).string("m"));
	m_indexRatioDoubleSpinBox.setPrefix("n2/n1 = ");
	m_indexRatioDoubleSpinBox.setMinimum(0.);
	m_indexRatioDoubleSpinBox.setMaximum(Unit::infinity);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(&m_indexRatioDoubleSpinBox);
	layout->addWidget(&m_surfaceRadiusDoubleSpinBox);
	setLayout(layout);
}

GaussianBeamDelegate::GaussianBeamDelegate(QObject* parent, GaussianBeamModel* model)
	: QItemDelegate(parent)
	, m_model(model)
{
}

QWidget *GaussianBeamDelegate::createEditor(QWidget* parent,
	const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
	/// @todo where are all these "new" things deleted ? Check Qt example

	switch (index.column())
	{
	case COL_WAIST:
	case COL_RAYLEIGH:
	case COL_DIVERGENCE:
	{
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(0.);
		editor->setMaximum(Unit::infinity);
		return editor;
	}
	case COL_POSITION:
	case COL_WAIST_POSITION:
	{
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(-Unit::infinity);
		editor->setMaximum(Unit::infinity);
		return editor;
	}
	case COL_PROPERTIES:
	{
		const Optics& optics = m_model->optics(index);

		if (optics.type() == CurvedInterfaceType)
		{
			CurvedInterfaceWidget* editor = new CurvedInterfaceWidget(parent);
			return editor;
		}
		else if (optics.type() == GenericABCDType)
		{
			ABCDWidget* editor = new ABCDWidget(parent);
			return editor;
		}

		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(-Unit::infinity);
		editor->setMaximum(Unit::infinity);
		if (optics.type() == LensType)
		{
			editor->setPrefix("f = ");
			editor->setSuffix(Units::getUnit(UnitFocal).string("m"));
		}
		else if (optics.type() == CurvedMirrorType)
		{
			editor->setPrefix("R = ");
			editor->setSuffix(Units::getUnit(UnitCurvature).string("m"));
		}
		else if (optics.type() == FlatInterfaceType)
		{
			editor->setMinimum(0.);
			editor->setPrefix("n2/n1 = ");
		}
		else if (optics.type() == CurvedInterfaceType)
		{
			editor->setPrefix("R = ");
			editor->setSuffix(Units::getUnit(UnitCurvature).string("m"));
		}
		return editor;
	}
	case COL_NAME:
	{
		QLineEdit* editor = new QLineEdit(parent);
		return editor;
	}
	case COL_LOCK:
	{
		const Optics& optics = m_model->optics(index);
		QComboBox* editor = new QComboBox(parent);
		editor->addItem(tr("none"));
		editor->addItem(tr("absolute"));
		for (int i = 0; i < m_model->rowCount(); i++)
			if (!optics.relativeLockedTo(m_model->opticsPtr(i)) || (m_model->opticsPtr(i) == optics.relativeLockParent()))
				editor->addItem(QString::fromUtf8(m_model->optics(i).name().c_str()));
		return editor;
	}
	default:
		return 0;
	}

	return 0;
}

void GaussianBeamDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	/// @todo why static cast ?
	qDebug() << "setEditorData" << m_model->data(index, Qt::DisplayRole);

	if (!index.isValid() || (editor == 0))
		return;

	switch (index.column())
	{
	case COL_POSITION:
	case COL_WAIST:
	case COL_WAIST_POSITION:
	case COL_RAYLEIGH:
	case COL_DIVERGENCE:
	{
		double value = m_model->data(index, Qt::DisplayRole).toDouble();
		QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
		spinBox->setValue(value);
		break;
	}
	case COL_PROPERTIES:
	{
		const Optics& optics = m_model->optics(index);

		if (optics.type() == CurvedInterfaceType)
		{
			const CurvedInterface curvedInterface = dynamic_cast<const CurvedInterface&>(optics);
			CurvedInterfaceWidget* widget = static_cast<CurvedInterfaceWidget*>(editor);
			widget->setSurfaceRadius(curvedInterface.surfaceRadius()*Units::getUnit(UnitCurvature).divider());
			widget->setIndexRatio(curvedInterface.indexRatio());
			break;
		}
		else if (optics.type() == GenericABCDType)
		{
			const GenericABCD ABCDOptics = dynamic_cast<const GenericABCD&>(optics);
			ABCDWidget* widget = static_cast<ABCDWidget*>(editor);
			widget->setA(ABCDOptics.A());
			widget->setB(ABCDOptics.B()*Units::getUnit(UnitABCD).divider());
			widget->setC(ABCDOptics.C()/Units::getUnit(UnitABCD).divider());
			widget->setD(ABCDOptics.D());
			widget->setWidth(ABCDOptics.width()*Units::getUnit(UnitWidth).divider());
			break;
		}

		double value = 0.;
		if (optics.type() == LensType)
			value = dynamic_cast<const Lens&>(optics).focal()*Units::getUnit(UnitFocal).divider();
		else if (optics.type() == CurvedMirrorType)
			value = dynamic_cast<const CurvedMirror&>(optics).curvatureRadius()*Units::getUnit(UnitCurvature).divider();
		else if (optics.type() == FlatInterfaceType)
			value = dynamic_cast<const FlatInterface&>(optics).indexRatio();
		QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
		spinBox->setValue(value);
		break;
	}
	case COL_NAME:
	{
		QString name = m_model->data(index, Qt::DisplayRole).toString();
		QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
		lineEdit->setText(name);
		break;
	}
	case COL_LOCK:
	{
		QString value = m_model->data(index, Qt::DisplayRole).toString();
		QComboBox* comboBox = static_cast<QComboBox*>(editor);
		comboBox->setCurrentIndex(comboBox->findText(value));
		break;
	}
	default:
		return QItemDelegate::setEditorData(editor, index);
	}
}

void GaussianBeamDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
	const QModelIndex& index) const
{
	if (!index.isValid() || (editor == 0))
		return;

	switch (index.column())
	{
	case COL_LOCK:
	{
		QComboBox *comboBox = static_cast<QComboBox*>(editor);
		model->setData(index, comboBox->itemText(comboBox->currentIndex()));
		break;
	}
	case COL_PROPERTIES:
	{
		const Optics& optics = m_model->optics(index);

		if (optics.type() == CurvedInterfaceType)
		{
			CurvedInterfaceWidget* widget = static_cast<CurvedInterfaceWidget*>(editor);
			QList<QVariant> propertyList;
			propertyList.push_back(widget->surfaceRadius());
			propertyList.push_back(widget->indexRatio());
			model->setData(index, propertyList);
		}
		else if (optics.type() == GenericABCDType)
		{
			ABCDWidget* widget = static_cast<ABCDWidget*>(editor);
			QList<QVariant> propertyList;
			propertyList.push_back(widget->A());
			propertyList.push_back(widget->B());
			propertyList.push_back(widget->C());
			propertyList.push_back(widget->D());
			propertyList.push_back(widget->width());
			model->setData(index, propertyList);
		}
		else
			QItemDelegate::setModelData(editor, model, index);
		break;
	}
	default:
		QItemDelegate::setModelData(editor, model, index);
	}
}

void GaussianBeamDelegate::updateEditorGeometry(QWidget* editor,
	const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (!index.isValid() || (editor == 0))
		return;

	QItemDelegate::updateEditorGeometry(editor, option, index);
}
