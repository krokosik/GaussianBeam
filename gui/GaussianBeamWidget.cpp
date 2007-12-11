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

#include "gui/GaussianBeamWidget.h"
#include "gui/GaussianBeamWindow.h"
#include "gui/OpticsView.h"
#ifdef GBPLOT
	#include "gui/GaussianBeamPlot.h"
#endif
#include "gui/Unit.h"

#include <QApplication>
#include <QPushButton>
#include <QStandardItemModel>
#include <QDebug>
#include <QMessageBox>
#include <QMenu>

#include <cmath>

GaussianBeamWidget::GaussianBeamWidget(OpticsBench& bench, OpticsItemView* opticsItemView,
	                   OpticsView* opticsView, OpticsScene* opticsScene, QWidget *parent)
	: QWidget(parent)
	, m_opticsItemView(opticsItemView)
	, m_opticsView(opticsView)
	, m_opticsScene(opticsScene)
	, m_bench(bench)
{
	setupUi(this);
	//toolBox->setSizeHint(100);
	//toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));

	// Extra widgets, not included in designer
#ifdef GBPLOT
/*	plot = new GaussianBeamPlot(this, model);
	splitter->addWidget(plot);
	checkBox_ShowGraph->setVisible(true);
	checkBox_ShowGraph->setEnabled(true);
	//checkBox_ShowGraph->setChecked(true);
	plot->setVisible(checkBox_ShowGraph->isChecked());*/
#else
	checkBox_ShowGraph->setVisible(false);
	checkBox_ShowGraph->setEnabled(false);
#endif

	// Waist fit
	fitModel = new QStandardItemModel(5, 2, this);
	fitModel->setHeaderData(0, Qt::Horizontal, tr("Position") + "\n(" + Units::getUnit(UnitPosition).prefix() + "m)");
	fitModel->setHeaderData(1, Qt::Horizontal, tr("Value") + "\n(" + Units::getUnit(UnitWaist).prefix() + "m)");
	fitTable->setModel(fitModel);
	fitTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	fitTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	fitSelectionModel = new QItemSelectionModel(fitModel);
	fitTable->setSelectionModel(fitSelectionModel);
	fitTable->setColumnWidth(0, 82);
	fitTable->setColumnWidth(1, 82);
	comboBox_FitData->insertItem(0, tr("Radius @ 1/e²"));
	comboBox_FitData->insertItem(1, tr("Diameter @ 1/e²"));
	comboBox_FitData->setCurrentIndex(1);
	m_opticsItemView->setFitModel(fitModel);
	m_opticsItemView->setMeasureCombo(comboBox_FitData);

	// Connect slots
	connect(fitModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
	        dynamic_cast<GaussianBeamWindow*>(parent), SLOT(updateView(const QModelIndex&, const QModelIndex&)));

	// Set up default values
	on_doubleSpinBox_Wavelength_valueChanged(doubleSpinBox_Wavelength->value());
	on_radioButton_Tolerance_toggled(radioButton_Tolerance->isChecked());
	on_doubleSpinBox_TargetPosition_valueChanged(0./* unused. Note: this changes also the waist value */);
	on_doubleSpinBox_HRange_valueChanged(doubleSpinBox_HRange->value());
	on_doubleSpinBox_VRange_valueChanged(doubleSpinBox_VRange->value());
	on_doubleSpinBox_HOffset_valueChanged(doubleSpinBox_HOffset->value());
	on_checkBox_ShowTargetBeam_toggled(checkBox_ShowTargetBeam->isChecked());
	updateUnits();
}

void GaussianBeamWidget::on_doubleSpinBox_Wavelength_valueChanged(double value)
{
	m_bench.setWavelength(value*Units::getUnit(UnitWavelength).multiplier());
}

void GaussianBeamWidget::updateUnits()
{
	doubleSpinBox_Wavelength->setSuffix(Units::getUnit(UnitWavelength).string("m"));
	doubleSpinBox_HRange->setSuffix(Units::getUnit(UnitHRange).string("m"));
	doubleSpinBox_VRange->setSuffix(Units::getUnit(UnitVRange).string("m"));
	doubleSpinBox_HOffset->setSuffix(Units::getUnit(UnitHRange).string("m"));
	doubleSpinBox_TargetWaist->setSuffix(Units::getUnit(UnitWaist).string("m"));
	doubleSpinBox_TargetPosition->setSuffix(Units::getUnit(UnitPosition).string("m"));
	/// @todo update table headers and status bar
}

///////////////////////////////////////////////////////////
// TOOLS PAGE

///////////////////////////////////////////////////////////
// MAGIC WAIST PAGE

Beam GaussianBeamWidget::targetWaist()
{
	return Beam(doubleSpinBox_TargetWaist->value()*Units::getUnit(UnitWaist).multiplier(),
	            doubleSpinBox_TargetPosition->value()*Units::getUnit(UnitPosition).multiplier(),
	            m_bench.wavelength());
}

void GaussianBeamWidget::displayOverlap()
{
	if (m_bench.nOptics() > 0)
	{
		double overlap = GaussianBeam::overlap(m_bench.beam(m_bench.nOptics()-1), targetWaist());
		label_OverlapResult->setText(tr("Overlap: ") + QString::number(overlap*100., 'f', 2) + " %");
	}
	else
		label_OverlapResult->setText("");
}

void GaussianBeamWidget::on_doubleSpinBox_TargetWaist_valueChanged(double value)
{
	Q_UNUSED(value);
	m_bench.setTargetBeam(targetWaist());
	displayOverlap();
}

void GaussianBeamWidget::on_doubleSpinBox_TargetPosition_valueChanged(double value)
{
	Q_UNUSED(value);
	m_bench.setTargetBeam(targetWaist());
	displayOverlap();
}

void GaussianBeamWidget::on_checkBox_ShowTargetBeam_toggled(bool checked)
{
	m_opticsItemView->setShowTargetBeam(checked);
	m_opticsScene->showTargetBeam(checked);
}

void GaussianBeamWidget::on_radioButton_Tolerance_toggled(bool checked)
{
	if (checked)
	{
		label_MinOverlap->setVisible(false);
		doubleSpinBox_MinOverlap->setVisible(false);
		label_WaistTolerance->setVisible(true);
		doubleSpinBox_WaistTolerance->setVisible(true);
		label_PositionTolerance->setVisible(true);
		doubleSpinBox_PositionTolerance->setVisible(true);
	}
	else
	{
		label_WaistTolerance->setVisible(false);
		doubleSpinBox_WaistTolerance->setVisible(false);
		label_PositionTolerance->setVisible(false);
		doubleSpinBox_PositionTolerance->setVisible(false);
		label_MinOverlap->setVisible(true);
		doubleSpinBox_MinOverlap->setVisible(true);
	}
}

void GaussianBeamWidget::on_pushButton_MagicWaist_clicked()
{
	label_MagicWaistResult->setText("");

	Tolerance tolerance;
	tolerance.overlap = radioButton_Overlap->isChecked();
	tolerance.minOverlap = doubleSpinBox_MinOverlap->value()*0.01;
	tolerance.waistTolerance = doubleSpinBox_WaistTolerance->value()*0.01;
	tolerance.positionTolerance = doubleSpinBox_PositionTolerance->value()*0.01;

	if (!m_bench.magicWaist(tolerance))
		label_MagicWaistResult->setText(tr("Desired waist could not be found !"));
	else
		displayOverlap();
}

///////////////////////////////////////////////////////////
// FIT PAGE

void GaussianBeamWidget::on_pushButton_Fit_clicked()
{
	double factor = 1.;
	if (comboBox_FitData->currentIndex() == 1)
		factor = 0.5;

	Fit& fit = m_bench.fit(0);
	fit.clear();

	for (int row = 0; row < fitModel->rowCount(); ++row)
		if (fitModel->data(fitModel->index(row, 1)).toDouble() != 0.)
		{
			double position = fitModel->data(fitModel->index(row, 0)).toDouble()*Units::getUnit(UnitPosition).multiplier();
			double radius = factor*fitModel->data(fitModel->index(row, 1)).toDouble()*Units::getUnit(UnitWaist).multiplier();
			fit.addData(position, radius);
		}

	if (fit.size() <= 1)
		return;

	Beam fitBeam = fit.beam(m_bench.wavelength()); //GaussianBeam::fitBeam(positions, radii, m_bench.wavelength(), &rho2);
	QString text = tr("Waist") + " = " + QString::number(fitBeam.waist()*Units::getUnit(UnitWaist).divider()) + Units::getUnit(UnitWaist).string("m") + "\n" +
	               tr("Position") + " = " + QString::number(fitBeam.waistPosition()*Units::getUnit(UnitPosition).divider()) + Units::getUnit(UnitPosition).string("m") + "\n" +
	               tr("R²") + " = " + QString::number(fit.rho2(m_bench.wavelength()));
	label_FitResult->setText(text);
	pushButton_SetInputBeam->setEnabled(true);
	pushButton_SetTargetBeam->setEnabled(true);
}

void GaussianBeamWidget::on_pushButton_SetInputBeam_clicked()
{
	m_bench.setInputBeam(m_bench.fit(0).beam(m_bench.wavelength()));
}

void GaussianBeamWidget::on_pushButton_SetTargetBeam_clicked()
{
	Beam fitBeam = m_bench.fit(0).beam(m_bench.wavelength());

	doubleSpinBox_TargetWaist->setValue(fitBeam.waist()*Units::getUnit(UnitWaist).divider());
	doubleSpinBox_TargetPosition->setValue(fitBeam.waistPosition()*Units::getUnit(UnitPosition).divider());
}

void GaussianBeamWidget::on_pushButton_FitAddRow_clicked()
{
	QModelIndex index = fitTable->selectionModel()->currentIndex();
	int row = fitModel->rowCount();
	if (index.isValid())
		row = index.row() + 1;
	fitModel->insertRow(row);
}

void GaussianBeamWidget::on_pushButton_FitRemoveRow_clicked()
{
	for (int row = fitModel->rowCount() - 1; row >= 0; row--)
		if (fitSelectionModel->isRowSelected(row, QModelIndex()))
			fitModel->removeRow(row);
}

///////////////////////////////////////////////////////////
// DISPLAY PAGE

void GaussianBeamWidget::on_doubleSpinBox_HRange_valueChanged(double value)
{
	double horizontalRange = value*Units::getUnit(UnitHRange).multiplier();
	m_opticsItemView->setHRange(horizontalRange);
	m_opticsScene->setHorizontalRange(horizontalRange);
	m_bench.setRightBoundary(m_bench.leftBoundary() + horizontalRange);
}

void GaussianBeamWidget::on_doubleSpinBox_VRange_valueChanged(double value)
{
	double verticalRange = value*Units::getUnit(UnitVRange).multiplier();
	m_opticsItemView->setVRange(verticalRange);
	m_opticsScene->setVerticalRange(verticalRange);
}

void GaussianBeamWidget::on_doubleSpinBox_HOffset_valueChanged(double value)
{
	double horizontalOffset = value*Units::getUnit(UnitHRange).multiplier();
	double horizontalRange = doubleSpinBox_HRange->value()*Units::getUnit(UnitHRange).multiplier();
	m_opticsItemView->setHOffset(horizontalOffset);
	m_opticsScene->setHorizontalOffset(horizontalOffset);
	m_bench.setLeftBoundary(horizontalOffset);
	m_bench.setRightBoundary(horizontalOffset + horizontalRange);
}

void GaussianBeamWidget::on_checkBox_ShowGraph_toggled(bool checked)
{
	qDebug() << "Show graph";
#ifdef GBPLOT
	plot->setVisible(checked);
#else
	Q_UNUSED(checked);
#endif
}
