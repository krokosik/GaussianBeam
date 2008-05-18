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
#include <QColorDialog>
#include <QInputDialog>
#include <QSettings>

#include <cmath>

GaussianBeamWidget::GaussianBeamWidget(OpticsBench& bench, OpticsScene* opticsScene, QWidget* parent)
	: QWidget(parent)
	, OpticsBenchNotify(bench)
	, m_opticsScene(opticsScene)
{
	m_updatingFit = false;
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
/*	checkBox_ShowGraph->setVisible(false);
	checkBox_ShowGraph->setEnabled(false);*/
#endif

	// Waist fit
	fitModel = new QStandardItemModel(0, 2, this);
	fitModel->setHeaderData(0, Qt::Horizontal, tr("Position") + "\n(" + Units::getUnit(UnitPosition).string(false) + ")");
	fitModel->setHeaderData(1, Qt::Horizontal, tr("Value") + "\n(" + Units::getUnit(UnitWaist).string(false) + ")");
	fitTable->setModel(fitModel);
	fitTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	fitTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	fitSelectionModel = new QItemSelectionModel(fitModel);
	fitTable->setSelectionModel(fitSelectionModel);
	fitTable->setColumnWidth(0, 82);
	fitTable->setColumnWidth(1, 82);

	// Connect slots
	connect(fitModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
	        this, SLOT(fitModelChanged(const QModelIndex&, const QModelIndex&)));

	// Set up default values
	m_bench.setTargetBeam(Beam(0.000150, 0.6, m_bench.wavelength()));
	on_radioButton_Tolerance_toggled(radioButton_Tolerance->isChecked());
	on_checkBox_ShowTargetBeam_toggled(checkBox_ShowTargetBeam->isChecked());
	updateUnits();
	readSettings();

	m_bench.registerNotify(this);
}

GaussianBeamWidget::~GaussianBeamWidget()
{
	writeSettings();
}

void GaussianBeamWidget::writeSettings()
{
	QSettings settings;
	settings.setValue("GaussianBeamWidget/toolboxIndex", toolBox->currentIndex());
}

void GaussianBeamWidget::readSettings()
{
	QSettings settings;
	toolBox->setCurrentIndex(settings.value("GaussianBeamWidget/toolboxIndex", 0).toInt());
}

void GaussianBeamWidget::updateUnits()
{
	doubleSpinBox_TargetWaist->setSuffix(Units::getUnit(UnitWaist).string());
	doubleSpinBox_TargetPosition->setSuffix(Units::getUnit(UnitPosition).string());
	doubleSpinBox_LeftBoundary->setSuffix(Units::getUnit(UnitPosition).string());
	doubleSpinBox_RightBoundary->setSuffix(Units::getUnit(UnitPosition).string());
	/// @todo update table headers and status bar and wavelength
}

void GaussianBeamWidget::OpticsBenchDataChanged(int /*startOptics*/, int /*endOptics*/)
{
	displayOverlap();
}

void GaussianBeamWidget::OpticsBenchTargetBeamChanged()
{
	doubleSpinBox_TargetWaist->setValue(m_bench.targetBeam().waist()*Units::getUnit(UnitWaist).divider());
	doubleSpinBox_TargetPosition->setValue(m_bench.targetBeam().waistPosition()*Units::getUnit(UnitPosition).divider());
	displayOverlap();
}

void GaussianBeamWidget::OpticsBenchBoundariesChanged()
{
	doubleSpinBox_LeftBoundary->setValue(m_bench.leftBoundary()*Units::getUnit(UnitPosition).divider());
	doubleSpinBox_RightBoundary->setValue(m_bench.rightBoundary()*Units::getUnit(UnitPosition).divider());
}

///////////////////////////////////////////////////////////
// OPTICS BENCH PAGE

void GaussianBeamWidget::on_doubleSpinBox_LeftBoundary_valueChanged(double value)
{
	m_bench.setLeftBoundary(value*Units::getUnit(UnitPosition).multiplier());
}

void GaussianBeamWidget::on_doubleSpinBox_RightBoundary_valueChanged(double value)
{
	m_bench.setRightBoundary(value*Units::getUnit(UnitPosition).multiplier());
}

///////////////////////////////////////////////////////////
// MAGIC WAIST PAGE

void GaussianBeamWidget::displayOverlap()
{
	if (m_bench.nOptics() > 0)
	{
		double overlap = Beam::overlap(m_bench.beam(m_bench.nOptics()-1), m_bench.targetBeam());
		label_OverlapResult->setText(tr("Overlap: ") + QString::number(overlap*100., 'f', 2) + " %");
	}
	else
		label_OverlapResult->setText("");
}

void GaussianBeamWidget::on_doubleSpinBox_TargetWaist_valueChanged(double value)
{
	Q_UNUSED(value);
	Beam beam = m_bench.targetBeam();
	beam.setWaist(doubleSpinBox_TargetWaist->value()*Units::getUnit(UnitWaist).multiplier());
	m_bench.setTargetBeam(beam);
}

void GaussianBeamWidget::on_doubleSpinBox_TargetPosition_valueChanged(double value)
{
	Q_UNUSED(value);
	Beam beam = m_bench.targetBeam();
	beam.setWaistPosition(doubleSpinBox_TargetPosition->value()*Units::getUnit(UnitPosition).multiplier());
	m_bench.setTargetBeam(beam);
}

void GaussianBeamWidget::on_checkBox_ShowTargetBeam_toggled(bool checked)
{
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

void GaussianBeamWidget::updateFitInformation(int index)
{
	qDebug() << "updateFitInformation" << index;

	m_updatingFit = true;
	Fit& fit = m_bench.fit(index);
	comboBox_Fit->setItemText(index, QString::fromUtf8(fit.name().c_str()));
	pushButton_FitColor->setPalette(QPalette(QColor(fit.color())));
	comboBox_FitData->setCurrentIndex(int(fit.dataType()));

	if (fit.size() > fitModel->rowCount())
		for (int i = 0; i < fit.size() - fitModel->rowCount(); i++)
			fitModel->insertRow(0);
	else if (fit.size() < fitModel->rowCount())
		for (int i = 0; i < fitModel->rowCount() - fit.size(); i++)
			qDebug() << fitModel->removeRow(0);

	for (int i = 0; i < fit.size(); i++)
	{
		if ((fit.position(i) != 0.) || (fit.value(i) != 0.))
			fitModel->setData(fitModel->index(i, 0), fit.position(i)*Units::getUnit(UnitPosition).divider());
		else
			fitModel->setData(fitModel->index(i, 0), QString(""));

		if (fit.value(i) != 0.)
			fitModel->setData(fitModel->index(i, 1), fit.value(i)*Units::getUnit(UnitWaist).divider());
		else
			fitModel->setData(fitModel->index(i, 1), QString(""));
	}

	if (fit.nonZeroSize() <= 1)
	{
		pushButton_SetInputBeam->setEnabled(false);
		pushButton_SetTargetBeam->setEnabled(false);
		label_FitResult->setText(QString());
	}
	else
	{
		Beam fitBeam = fit.beam(m_bench.wavelength());
		QString text = tr("Waist") + " = " + QString::number(fitBeam.waist()*Units::getUnit(UnitWaist).divider()) + Units::getUnit(UnitWaist).string() + "\n" +
					tr("Position") + " = " + QString::number(fitBeam.waistPosition()*Units::getUnit(UnitPosition).divider()) + Units::getUnit(UnitPosition).string() + "\n" +
					tr("R²") + " = " + QString::number(fit.rho2(m_bench.wavelength()));
		label_FitResult->setText(text);
		pushButton_SetInputBeam->setEnabled(true);
		pushButton_SetTargetBeam->setEnabled(true);
	}

	m_updatingFit = false;
}

// Control callback

void GaussianBeamWidget::on_comboBox_Fit_currentIndexChanged(int index)
{
	if (index < 0)
		return;
	updateFitInformation(index);
}

void GaussianBeamWidget::on_pushButton_AddFit_clicked()
{
	m_bench.addFit(m_bench.nFit());
}

void GaussianBeamWidget::on_pushButton_RemoveFit_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	m_bench.removeFit(index);
}

void GaussianBeamWidget::on_pushButton_RenameFit_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	bool ok;
	QString text = QInputDialog::getText(this, tr("Rename fit)"),
		tr("Enter a new name for the current fit:"), QLineEdit::Normal,
		m_bench.fit(index).name().c_str(), &ok);
	if (ok && !text.isEmpty())
	{
		m_bench.fit(index).setName(text.toUtf8().data());
		m_bench.notifyFitChange(index);
	}
}

void GaussianBeamWidget::on_pushButton_FitColor_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	QColor color = QColorDialog::getColor(Qt::black, this);
	m_bench.fit(index).setColor(color.rgb());
	m_bench.notifyFitChange(index);
}

void GaussianBeamWidget::on_comboBox_FitData_currentIndexChanged(int dataIndex)
{
	int index = comboBox_Fit->currentIndex();
	if ((index < 0) || m_updatingFit)
		return;

	Fit& fit = m_bench.fit(index);
	fit.setDataType(FitDataType(dataIndex));
	m_bench.notifyFitChange(index);
}

void GaussianBeamWidget::fitModelChanged(const QModelIndex& start, const QModelIndex& stop)
{
	int index = comboBox_Fit->currentIndex();
	if ((index < 0) || m_updatingFit)
		return;

	Fit& fit = m_bench.fit(index);

	qDebug() << start.row() << stop.row();

	for (int row = start.row(); row <= stop.row(); row++)
	{
		double position = fitModel->data(fitModel->index(row, 0)).toDouble()*Units::getUnit(UnitPosition).multiplier();
		double value = fitModel->data(fitModel->index(row, 1)).toDouble()*Units::getUnit(UnitWaist).multiplier();
		qDebug() << "Fit changed" << row << position << value;
		fit.setData(row, position, value);
	}

	m_bench.notifyFitChange(index);
}

void GaussianBeamWidget::on_pushButton_FitAddRow_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	m_bench.fit(index).addData(0., 0.);
	m_bench.notifyFitChange(index);
}

void GaussianBeamWidget::on_pushButton_FitRemoveRow_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	for (int row = fitModel->rowCount() - 1; row >= 0; row--)
		if (fitSelectionModel->isRowSelected(row, QModelIndex()) && (row < m_bench.fit(index).size()))
			m_bench.fit(index).removeData(row);

	m_bench.notifyFitChange(index);
}

void GaussianBeamWidget::on_pushButton_SetInputBeam_clicked()
{
	m_bench.setInputBeam(m_bench.fit(0).beam(m_bench.wavelength()));
}

void GaussianBeamWidget::on_pushButton_SetTargetBeam_clicked()
{
	Beam fitBeam = m_bench.fit(0).beam(m_bench.wavelength());
	m_bench.setTargetBeam(fitBeam);
}

// OpticsBench callbacks

void GaussianBeamWidget::OpticsBenchFitAdded(int index)
{
	qDebug() << "Insert fit item" << index;
	comboBox_Fit->insertItem(index, QString::fromUtf8(m_bench.fit(index).name().c_str()));
	comboBox_Fit->setCurrentIndex(index);
}

void GaussianBeamWidget::OpticsBenchFitRemoved(int index)
{
	qDebug() << "Remove fit item" << index;
	comboBox_Fit->removeItem(index);
}

void GaussianBeamWidget::OpticsBenchFitDataChanged(int index)
{
	int comboIndex = comboBox_Fit->currentIndex();
	if ((comboIndex < 0) || (comboIndex != index))
		return;

	updateFitInformation(index);
}
