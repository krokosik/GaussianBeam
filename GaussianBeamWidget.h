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

#ifndef GAUSSIANBEAMWIDGET_H
#define GAUSSIANBEAMWIDGET_H

#include "ui_GaussianBeamForm.h"
#include "GaussianBeam.h"

#include <QWidget>

class QStandardItemModel;
class QDomElement;
class QAction;

class OpticsView;
class GaussianBeamPlot;
class GaussianBeamDelegate;
class GaussianBeamModel;

class GaussianBeamWidget : public QWidget, private Ui::GaussianBeamForm
{
	Q_OBJECT

public:
	GaussianBeamWidget(QString file = QString(), QWidget* parent = 0);

protected slots:
	void on_pushButton_Add_clicked();
	void on_pushButton_Remove_clicked();
	void on_pushButton_MagicWaist_clicked();
	void on_pushButton_Save_clicked();
	void on_pushButton_SaveAs_clicked();
	void on_pushButton_Open_clicked();
	void on_pushButton_Fit_clicked();
	void on_pushButton_SetInputBeam_clicked();
	void on_pushButton_SetTargetBeam_clicked();
	void on_checkBox_ShowGraph_toggled(bool checked);
	void on_checkBox_ShowTargetWaist_toggled(bool checked);
	void on_doubleSpinBox_Wavelength_valueChanged(double value);
	void on_doubleSpinBox_HRange_valueChanged(double value);
	void on_doubleSpinBox_VRange_valueChanged(double value);
	void on_doubleSpinBox_HOffset_valueChanged(double value);
	void on_doubleSpinBox_TargetWaist_valueChanged(double value);
	void on_doubleSpinBox_TargetPosition_valueChanged(double value);
	void on_radioButton_Tolerance_toggled(bool checked);
	void on_action_AddLens_triggered();
	void on_action_AddFlatMirror_triggered();
	void on_action_AddCurvedMirror_triggered();
	void on_action_AddFlatInterface_triggered();
	void on_action_AddCurvedInterface_triggered();
	void on_action_AddGenericABCD_triggered();

protected slots:
	void updateWidget(const QModelIndex& topLeft, const QModelIndex& bottomRight);
	void updateView(const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:
	void openFile(const QString& path = QString());
	void saveFile(const QString& path = QString());
	void parseXml(const QDomElement& element);
	void parseXmlOptics(const QDomElement& element);
	void setCurrentFile(const QString& path);
	void updateUnits();
	void insertOptics(Optics* optics, bool resizeRow = false);
	Beam targetWaist();
	void displayOverlap();

private:
	GaussianBeamModel* model;
	GaussianBeamDelegate* delegate;
	QItemSelectionModel* selectionModel;
	OpticsView* opticsView;
	GaussianBeamPlot* plot;
	QStandardItemModel* fitModel;

	Beam m_fitBeam;
	QString m_currentFile;
	int m_lastLensName, m_lastFlatMirrorName, m_lastCurvedMirrorName,
	    m_lastFlatInterfaceName, m_lastCurvedInterfaceName, m_lastGenericABCDName;
};

#endif
