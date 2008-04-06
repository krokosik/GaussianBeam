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

#include "ui_GaussianBeamWidget.h"
#include "src/GaussianBeam.h"
#include "src/OpticsBench.h"
#include "src/Optics.h"

#include <QWidget>

class QStandardItemModel;
class QDomElement;
class QAction;

class OpticsScene;
class GaussianBeamPlot;
class GaussianBeamDelegate;
class GaussianBeamModel;

class GaussianBeamWidget : public QWidget, private OpticsBenchNotify, private Ui::GaussianBeamWidget
{
	Q_OBJECT

public:
	GaussianBeamWidget(OpticsBench& bench, OpticsScene* opticsScene, QWidget* parent = 0);

public:
	bool openFile(const QString& fileName = QString());
	bool saveFile(const QString& fileName = QString());
	void displayOverlap();

// UI slots
protected slots:
	// Magic waist
	void on_pushButton_MagicWaist_clicked();
	void on_checkBox_ShowTargetBeam_toggled(bool checked);
	void on_doubleSpinBox_TargetWaist_valueChanged(double value);
	void on_doubleSpinBox_TargetPosition_valueChanged(double value);
	void on_radioButton_Tolerance_toggled(bool checked);
	// Waist fit
	void on_comboBox_Fit_currentIndexChanged(int index);
	void on_comboBox_FitData_currentIndexChanged(int dataIndex);
	void on_pushButton_AddFit_clicked();
	void on_pushButton_RemoveFit_clicked();
	void on_pushButton_RenameFit_clicked();
	void on_pushButton_FitColor_clicked();
	void on_pushButton_SetInputBeam_clicked();
	void on_pushButton_SetTargetBeam_clicked();
	void on_pushButton_FitAddRow_clicked();
	void on_pushButton_FitRemoveRow_clicked();


// optics bench inherited virtual functions
private:
	virtual void OpticsBenchDataChanged(int startOptics, int endOptics);
	virtual void OpticsBenchTargetBeamChanged();
	virtual void OpticsBenchFitAdded(int index);
	virtual void OpticsBenchFitRemoved(int index);
	virtual void OpticsBenchFitDataChanged(int index);

private slots:
	void fitModelChanged(const QModelIndex& start = QModelIndex(), const QModelIndex& stop = QModelIndex());

private:
	void updateUnits();
	void insertOptics(OpticsType opticsType);
	void updateFitInformation(int index);

private:
	OpticsScene* m_opticsScene;

	QStandardItemModel* fitModel;
	QItemSelectionModel* fitSelectionModel;
	GaussianBeamPlot* plot;

	bool m_updatingFit;
};

#endif
