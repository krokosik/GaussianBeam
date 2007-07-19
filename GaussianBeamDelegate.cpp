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

#include <QtGui>

#include "GaussianBeamDelegate.h"
#include "GaussianBeamModel.h"

GaussianBeamDelegate::GaussianBeamDelegate(QObject* parent)
	: QItemDelegate(parent)
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
	case COL_FOCAL:
	case COL_WAIST_POSITION:
	{
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(-infinity);
		editor->setMaximum(infinity);
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
		editor->addItem(tr("true"));
		editor->addItem(tr("false"));
		return editor;
	}
	default:
		return 0;
	}

	return 0;
}

void GaussianBeamDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	qDebug() << "setEditorData" << index.model()->data(index, Qt::DisplayRole);

	if (!index.isValid() || (editor == 0))
		return;

	switch (index.column())
	{
	case COL_POSITION:
	case COL_FOCAL:
	case COL_WAIST:
	case COL_WAIST_POSITION:
	case COL_RAYLEIGH:
	case COL_DIVERGENCE:
	{
		double value = index.model()->data(index, Qt::DisplayRole).toDouble();
		QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
		spinBox->setValue(value);
		break;
	}
	case COL_NAME:
	{
		QString name = index.model()->data(index, Qt::DisplayRole).toString();
		QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
		lineEdit->setText(name);
		break;
	}
	case COL_LOCK:
	{
		bool value = index.model()->data(index, Qt::DisplayRole).toBool();
		QComboBox *comboBox = static_cast<QComboBox*>(editor);
		comboBox->setCurrentIndex(value ? 0 : 1);
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
