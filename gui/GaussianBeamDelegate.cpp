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

#include "gui/GaussianBeamDelegate.h"
#include "gui/GaussianBeamModel.h"
#include "gui/Unit.h"

#include <QtGui>

PropertyEditor::PropertyEditor(QList<EditorProperty>& properties, QWidget* parent)
	: QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);

	foreach (EditorProperty property, properties)
	{
		QDoubleSpinBox* spinBox = new QDoubleSpinBox(this);
		spinBox->setAccelerated(true);
		spinBox->setMinimum(property.minimum);
		spinBox->setMaximum(property.maximum);
		spinBox->setPrefix(property.prefix);
		spinBox->setSuffix(property.suffix);
		layout->addWidget(spinBox);
		m_doubleSpinBoxes << spinBox;
	}

	setLayout(layout);
}

QList<QVariant> PropertyEditor::values() const
{
	QList<QVariant> properties;

	foreach (QDoubleSpinBox* spinBox, m_doubleSpinBoxes)
		properties << spinBox->value();

	return properties;
}

GaussianBeamDelegate::GaussianBeamDelegate(QObject* parent, GaussianBeamModel* model, OpticsBench& bench)
	: QItemDelegate(parent)
	, m_model(model)
	, m_bench(bench)
{
}

QWidget *GaussianBeamDelegate::createEditor(QWidget* parent,
	const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
	/// @todo where are all these "new" things deleted ? Check Qt example

	int row = index.row();
	ColumnContent column = m_model->columnContent(index.column());
	const Optics* optics = m_bench.optics(row);

	switch (column)
	{
	case WaistColumn:
	case RayleighColumn:
	case DivergenceColumn:
	{
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(0.);
		editor->setMaximum(Unit::infinity);
		return editor;
	}
	case PositionColumn:
	case RelativePositionColumn:
	case WaistPositionColumn:
	{
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(-Unit::infinity);
		editor->setMaximum(Unit::infinity);
		return editor;
	}
	case PropertiesColumn:
	{
		QList<EditorProperty> properties;
		if (optics->type() == LensType)
			properties << EditorProperty(-Unit::infinity, Unit::infinity, "f = ", Units::getUnit(UnitFocal).string("m"));
		else if (optics->type() == CurvedMirrorType)
			properties << EditorProperty(-Unit::infinity, Unit::infinity, "R = ", Units::getUnit(UnitCurvature).string("m"));
		else if (optics->type() == FlatInterfaceType)
			properties << EditorProperty(0., Unit::infinity, "n2/n1 = ", "");
		else if (optics->type() == CurvedInterfaceType)
			properties << EditorProperty(0., Unit::infinity, "n2/n1 = ", "")
			           << EditorProperty(-Unit::infinity, Unit::infinity, "R = ", Units::getUnit(UnitCurvature).string("m"));
		else if (optics->type() == DielectricSlabType)
			properties << EditorProperty(0., Unit::infinity, "n2/n1 = ", "")
			           << EditorProperty(0., Unit::infinity, "width = ", Units::getUnit(UnitWidth).string("m"));
		else if (optics->type() == GenericABCDType)
			properties << EditorProperty(-Unit::infinity, Unit::infinity, "A = ", "")
			           << EditorProperty(-Unit::infinity, Unit::infinity, "B = ", Units::getUnit(UnitABCD).string("m"))
			           << EditorProperty(-Unit::infinity, Unit::infinity, "C = ", " /" + Units::getUnit(UnitABCD).string("m", false))
			           << EditorProperty(-Unit::infinity, Unit::infinity, "D = ", "")
			           << EditorProperty(0., Unit::infinity, "width = ", Units::getUnit(UnitWidth).string("m"));

		return new PropertyEditor(properties, parent);
	}
	case NameColumn:
	{
		QLineEdit* editor = new QLineEdit(parent);
		return editor;
	}
	case LockColumn:
	{
		QComboBox* editor = new QComboBox(parent);
		editor->addItem(tr("none"));
		editor->addItem(tr("absolute"));
		for (int i = 0; i < m_model->rowCount(); i++)
			if ((!m_bench.optics(i)->relativeLockedTo(optics)) || (m_bench.optics(i) == optics->relativeLockParent()))
				editor->addItem(QString::fromUtf8(m_bench.optics(i)->name().c_str()));
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

	if (!index.isValid() || (editor == 0))
		return;

	int row = index.row();
	ColumnContent column = m_model->columnContent(index.column());
	const Optics* optics = m_bench.optics(row);

	switch (column)
	{
	case PositionColumn:
	case RelativePositionColumn:
	case WaistColumn:
	case WaistPositionColumn:
	case RayleighColumn:
	case DivergenceColumn:
	{
		double value = m_model->data(index, Qt::DisplayRole).toDouble();
		QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
		spinBox->setValue(value);
		break;
	}
	case PropertiesColumn:
	{
		PropertyEditor* propertyEditor = static_cast<PropertyEditor*>(editor);
		if (optics->type() == LensType)
			propertyEditor->setValue(0, dynamic_cast<const Lens*>(optics)->focal()*Units::getUnit(UnitFocal).divider());
		else if (optics->type() == CurvedMirrorType)
			propertyEditor->setValue(0, dynamic_cast<const CurvedMirror*>(optics)->curvatureRadius()*Units::getUnit(UnitCurvature).divider());
		else if (optics->type() == FlatInterfaceType)
			propertyEditor->setValue(0, dynamic_cast<const FlatInterface*>(optics)->indexRatio());
		else if (optics->type() == CurvedInterfaceType)
		{
			const CurvedInterface* curvedInterface = dynamic_cast<const CurvedInterface*>(optics);
			propertyEditor->setValue(0, curvedInterface->indexRatio());
			propertyEditor->setValue(1, curvedInterface->surfaceRadius()*Units::getUnit(UnitCurvature).divider());
		}
		else if (optics->type() == GenericABCDType)
		{
			const GenericABCD* ABCDOptics = dynamic_cast<const GenericABCD*>(optics);
			propertyEditor->setValue(0, ABCDOptics->A());
			propertyEditor->setValue(1, ABCDOptics->B()*Units::getUnit(UnitABCD).divider());
			propertyEditor->setValue(2, ABCDOptics->C()/Units::getUnit(UnitABCD).divider());
			propertyEditor->setValue(3, ABCDOptics->D());
			propertyEditor->setValue(4, ABCDOptics->width()*Units::getUnit(UnitWidth).divider());
			break;
		}
		else if (optics->type() == DielectricSlabType)
		{
			const DielectricSlab* dielectricSlab = dynamic_cast<const DielectricSlab*>(optics);
			propertyEditor->setValue(0, dielectricSlab->indexRatio());
			propertyEditor->setValue(1, dielectricSlab->width()*Units::getUnit(UnitWidth).divider());
		}

		break;
	}
	case NameColumn:
	{
		QString name = m_model->data(index, Qt::DisplayRole).toString();
		QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
		lineEdit->setText(name);
		break;
	}
	case LockColumn:
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

	int row = index.row();
	ColumnContent column = m_model->columnContent(index.column());
	const Optics* optics = m_bench.optics(row);

	switch (column)
	{
	case LockColumn:
	{
		QComboBox *comboBox = static_cast<QComboBox*>(editor);
		model->setData(index, comboBox->itemText(comboBox->currentIndex()));
		break;
	}
	case PropertiesColumn:
	{
		model->setData(index, static_cast<PropertyEditor*>(editor)->values());
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
