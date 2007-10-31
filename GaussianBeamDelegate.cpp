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

GaussianBeamDelegate::GaussianBeamDelegate(QObject* parent, GaussianBeamModel* model)
	: QItemDelegate(parent)
	, m_model(model)
{
}

QWidget *GaussianBeamDelegate::createEditor(QWidget* parent,
	const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
	double infinity = 1000000.;

	switch (index.column())
	{
	case COL_WAIST:
	case COL_RAYLEIGH:
	case COL_DIVERGENCE:
	{
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(0.);
		editor->setMaximum(infinity);
		return editor;
	}
	case COL_POSITION:
	case COL_WAIST_POSITION:
	{
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(-infinity);
		editor->setMaximum(infinity);
		return editor;
	}
	case COL_PROPERTIES:
	{
		const Optics& optics = m_model->optics(index);
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(-infinity);
		editor->setMaximum(infinity);
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
		else if (optics.type() == CurvedInterfaceType) /// @todo enable to change the index ratio
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
		QComboBox* editor = new QComboBox(parent);
		editor->addItem(tr("none"));
		editor->addItem(tr("absolute"));
		for (int i = 0; i < m_model->rowCount(); i++)
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
		QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
		spinBox->setValue(value);
		break;
	}
	case COL_PROPERTIES:
	{
		const Optics& optics = m_model->optics(index);
		double value = 0.;
		if (optics.type() == LensType)
			value = dynamic_cast<const Lens&>(optics).focal()*Units::getUnit(UnitFocal).divider();
		else if (optics.type() == CurvedMirrorType)
			value = dynamic_cast<const CurvedMirror&>(optics).curvatureRadius()*Units::getUnit(UnitCurvature).divider();
		else if (optics.type() == FlatInterfaceType)
			value = dynamic_cast<const FlatInterface&>(optics).indexRatio();
		else if (optics.type() == CurvedInterfaceType)
			value = dynamic_cast<const CurvedInterface&>(optics).surfaceRadius()*Units::getUnit(UnitCurvature).divider();
		QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
		spinBox->setValue(value);
		break;
	}
	case COL_NAME:
	{
		QString name = m_model->data(index, Qt::DisplayRole).toString();
		QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
		lineEdit->setText(name);
		break;
	}
	case COL_LOCK:
	{
		QString value = m_model->data(index, Qt::DisplayRole).toString();
		QComboBox *comboBox = static_cast<QComboBox*>(editor);
		/// @bug todo with string
		//comboBox->setCurrentIndex(value ? 0 : 1);
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
		model->setData(index, comboBox->currentIndex() == 0);
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
