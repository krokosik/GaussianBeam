/* This file is part of the GaussianBeam project
   Copyright (C) 2007-2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>

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

#include "src/GaussianBeam.h"
#include "src/OpticsBench.h"

#include "gui/GaussianBeamWidget.h"
#include "gui/GaussianBeamWindow.h"
#include "gui/OpticsView.h"
#include "gui/Unit.h"
#ifdef GBPLOT
	#include "gui/GaussianBeamPlot.h"
#endif

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

GaussianBeamWidget::GaussianBeamWidget(OpticsBench* bench, OpticsScene* opticsScene, QWidget* parent)
	: QWidget(parent)
	, m_bench(bench)
	, m_opticsScene(opticsScene)
{
	m_updatingFit = false;
	m_updatingTarget = false;
	setupUi(this);

	// Bench connections
	connect(m_bench, SIGNAL(dataChanged(int, int)), this, SLOT(onOpticsBenchDataChanged(int, int)));
	connect(m_bench, SIGNAL(targetBeamChanged()),   this, SLOT(onOpticsBenchTargetBeamChanged()));
	connect(m_bench, SIGNAL(boundariesChanged()),   this, SLOT(onOpticsBenchBoundariesChanged()));
	connect(m_bench, SIGNAL(fitAdded(int)),         this, SLOT(onOpticsBenchFitAdded(int)));
	connect(m_bench, SIGNAL(fitsRemoved(int, int)), this, SLOT(onOpticsBenchFitsRemoved(int, int)));
	connect(m_bench, SIGNAL(fitDataChanged(int)),   this, SLOT(onOpticsBenchFitDataChanged(int)));
	connect(m_bench, SIGNAL(wavelengthChanged()),   this, SLOT(onOpticsBenchWavelengthChanged()));

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
	on_checkBox_ShowTargetBeam_toggled(checkBox_ShowTargetBeam->isChecked());
	updateUnits();
	readSettings();

	// Sync with bench
	onOpticsBenchTargetBeamChanged();
	onOpticsBenchWavelengthChanged();
	onOpticsBenchBoundariesChanged();
	for (int i = 0; i < m_bench->nFit(); i++)
		onOpticsBenchFitAdded(i);

	updateFitInformation(comboBox_Fit->currentIndex());
	updateTargetInformation();
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
	doubleSpinBox_HTargetWaist->setSuffix(Units::getUnit(UnitWaist).string());
	doubleSpinBox_HTargetPosition->setSuffix(Units::getUnit(UnitPosition).string());
	doubleSpinBox_VTargetWaist->setSuffix(Units::getUnit(UnitWaist).string());
	doubleSpinBox_VTargetPosition->setSuffix(Units::getUnit(UnitPosition).string());
	doubleSpinBox_LeftBoundary->setSuffix(Units::getUnit(UnitPosition).string());
	doubleSpinBox_RightBoundary->setSuffix(Units::getUnit(UnitPosition).string());
	/// @todo update table headers and status bar and wavelength
}

void GaussianBeamWidget::onOpticsBenchDataChanged(int /*startOptics*/, int /*endOptics*/)
{
	displayOverlap();
}

void GaussianBeamWidget::onOpticsBenchWavelengthChanged()
{
	displayOverlap();
	updateFitInformation(comboBox_Fit->currentIndex());
}

///////////////////////////////////////////////////////////
// OPTICS BENCH PAGE

void GaussianBeamWidget::on_doubleSpinBox_LeftBoundary_valueChanged(double value)
{
	m_bench->setLeftBoundary(value*Units::getUnit(UnitPosition).multiplier());
}

void GaussianBeamWidget::on_doubleSpinBox_RightBoundary_valueChanged(double value)
{
	m_bench->setRightBoundary(value*Units::getUnit(UnitPosition).multiplier());
}

void GaussianBeamWidget::onOpticsBenchBoundariesChanged()
{
	doubleSpinBox_LeftBoundary->setValue(m_bench->leftBoundary()*Units::getUnit(UnitPosition).divider());
	doubleSpinBox_RightBoundary->setValue(m_bench->rightBoundary()*Units::getUnit(UnitPosition).divider());
}

///////////////////////////////////////////////////////////
// CAVITY PAGE

void GaussianBeamWidget::updateCavityInformation()
{

}

///////////////////////////////////////////////////////////
// MAGIC WAIST PAGE

void GaussianBeamWidget::displayOverlap()
{
	if (m_bench->nOptics() > 0)
	{
		double overlap = Beam::overlap(*m_bench->beam(m_bench->nOptics()-1), *m_bench->targetBeam());
		label_OverlapResult->setText(tr("Overlap: ") + QString::number(overlap*100., 'f', 2) + " %");
	}
	else
		label_OverlapResult->setText("");
}

void GaussianBeamWidget::updateTargetInformation()
{
	m_updatingTarget = true;

	if (m_bench->targetOrientation() == Ellipsoidal)
	{
		comboBox_TargetOrientation->setCurrentIndex(1);
		doubleSpinBox_VTargetWaist->setVisible(true);
		doubleSpinBox_VTargetPosition->setVisible(true);
	}
	else
	{
		comboBox_TargetOrientation->setCurrentIndex(0);
		doubleSpinBox_VTargetWaist->setVisible(false);
		doubleSpinBox_VTargetPosition->setVisible(false);
	}

	doubleSpinBox_HTargetWaist->setValue(m_bench->targetBeam()->waist(Horizontal)*Units::getUnit(UnitWaist).divider());
	doubleSpinBox_HTargetPosition->setValue(m_bench->targetBeam()->waistPosition(Horizontal)*Units::getUnit(UnitPosition).divider());
	doubleSpinBox_VTargetWaist->setValue(m_bench->targetBeam()->waist(Vertical)*Units::getUnit(UnitWaist).divider());
	doubleSpinBox_VTargetPosition->setValue(m_bench->targetBeam()->waistPosition(Vertical)*Units::getUnit(UnitPosition).divider());
	doubleSpinBox_MinOverlap->setValue(m_bench->targetOverlap()*100.);
	displayOverlap();

	m_updatingTarget = false;
}

void GaussianBeamWidget::on_comboBox_TargetOrientation_currentIndexChanged(int dataIndex)
{
	if (m_updatingTarget)
		return;

	if (dataIndex == 0)
		m_bench->setTargetOrientation(Spherical);
	else
		m_bench->setTargetOrientation(Ellipsoidal);
}

void GaussianBeamWidget::on_doubleSpinBox_HTargetWaist_valueChanged(double value)
{
	if (m_updatingTarget)
		return;

	Orientation orientation = Horizontal;
	if (m_bench->targetOrientation() == Spherical)
		orientation = Spherical;

	Beam beam = *m_bench->targetBeam();
	beam.setWaist(value*Units::getUnit(UnitWaist).multiplier(), orientation);
	m_bench->setTargetBeam(beam);
}

void GaussianBeamWidget::on_doubleSpinBox_HTargetPosition_valueChanged(double value)
{
	if (m_updatingTarget)
		return;

	Orientation orientation = Horizontal;
	if (m_bench->targetOrientation() == Spherical)
		orientation = Spherical;

	Beam beam = *m_bench->targetBeam();
	beam.setWaistPosition(value*Units::getUnit(UnitPosition).multiplier(), orientation);
	m_bench->setTargetBeam(beam);
}

void GaussianBeamWidget::on_doubleSpinBox_VTargetWaist_valueChanged(double value)
{
	if (m_updatingTarget)
		return;

	Beam beam = *m_bench->targetBeam();
	beam.setWaist(value*Units::getUnit(UnitWaist).multiplier(), Vertical);
	m_bench->setTargetBeam(beam);
}

void GaussianBeamWidget::on_doubleSpinBox_VTargetPosition_valueChanged(double value)
{
	if (m_updatingTarget)
		return;

	Beam beam = *m_bench->targetBeam();
	beam.setWaistPosition(value*Units::getUnit(UnitPosition).multiplier(), Vertical);
	m_bench->setTargetBeam(beam);
}

void GaussianBeamWidget::on_doubleSpinBox_MinOverlap_valueChanged(double value)
{
	if (m_updatingTarget)
		return;

	m_bench->setTargetOverlap(value*0.01);
}

void GaussianBeamWidget::on_checkBox_ShowTargetBeam_toggled(bool checked)
{
	if (m_updatingTarget)
		return;

	/// @todo
//	dynamic_cast<GaussianBeamWindow*>(parent)->showTargetBeam();
	m_opticsScene->showTargetBeam(checked);
}

void GaussianBeamWidget::on_pushButton_MagicWaist_clicked()
{
	label_MagicWaistResult->setText("");

	if (!m_bench->magicWaist())
		label_MagicWaistResult->setText(tr("Desired waist could not be found !"));
	else
		displayOverlap();
}

void GaussianBeamWidget::on_pushButton_LocalOptimum_clicked()
{
	if (!m_bench->localOptimum())
		label_MagicWaistResult->setText(tr("Local optimum not found !"));
	else
		displayOverlap();
}

void GaussianBeamWidget::onOpticsBenchTargetBeamChanged()
{
	updateTargetInformation();
}

///////////////////////////////////////////////////////////
// FIT PAGE

void GaussianBeamWidget::updateFitInformation(int index)
{
	if (index >= m_bench->nFit())
		return;

	// Enable or disable widgets
	if (index < 0)
	{
		pushButton_RemoveFit->setEnabled(false);
		pushButton_RenameFit->setEnabled(false);
		pushButton_FitColor->setEnabled(false);
		pushButton_FitAddRow->setEnabled(false);
		pushButton_FitRemoveRow->setEnabled(false);
		comboBox_FitData->setEnabled(false);
		comboBox_FitOrientation->setEnabled(false);
		fitTable->setEnabled(false);

		pushButton_SetInputBeam->setEnabled(false);
		pushButton_SetTargetBeam->setEnabled(false);
		return;
	}
	else
	{
		pushButton_RemoveFit->setEnabled(true);
		pushButton_RenameFit->setEnabled(true);
		pushButton_FitColor->setEnabled(true);
		pushButton_FitAddRow->setEnabled(true);
		pushButton_FitRemoveRow->setEnabled(true);
		comboBox_FitData->setEnabled(true);
		comboBox_FitOrientation->setEnabled(true);
		fitTable->setEnabled(true);
	}

	m_updatingFit = true;
	// Fill widgets
	Fit* fit = m_bench->fit(index);
	comboBox_Fit->setItemText(index, QString::fromUtf8(fit->name().c_str()));
	pushButton_FitColor->setPalette(QPalette(QColor(fit->color())));
	comboBox_FitData->setCurrentIndex(int(fit->dataType()));
	comboBox_FitOrientation->setCurrentIndex(int(fit->orientation()));

	// Resize rows to match the number of elements in the fit
	for (; fit->size() > fitModel->rowCount();)
		fitModel->insertRow(0);
	for (; fitModel->rowCount() > fit->size();)
		fitModel->removeRow(0);

	for (int i = 0; i < fit->size(); i++)
	{
		if ((fit->position(i) != 0.) || (fit->value(i) != 0.))
			fitModel->setData(fitModel->index(i, 0), fit->position(i)*Units::getUnit(UnitPosition).divider());
		else
			fitModel->setData(fitModel->index(i, 0), QString(""));

		if (fit->value(i) != 0.)
			fitModel->setData(fitModel->index(i, 1), fit->value(i)*Units::getUnit(UnitWaist).divider());
		else
			fitModel->setData(fitModel->index(i, 1), QString(""));
	}

	if (fit->nonZeroSize() <= 1)
	{
		pushButton_SetInputBeam->setEnabled(false);
		pushButton_SetTargetBeam->setEnabled(false);
		label_FitResult->setText(QString());
	}
	else
	{
		Beam fitBeam(m_bench->wavelength());
		double residue = fit->applyFit(fitBeam);
		QString text = tr("Waist") + " = " + QString::number(fitBeam.waist(fit->orientation())*Units::getUnit(UnitWaist).divider()) + Units::getUnit(UnitWaist).string() + "\n" +
					tr("Position") + " = " + QString::number(fitBeam.waistPosition(fit->orientation())*Units::getUnit(UnitPosition).divider()) + Units::getUnit(UnitPosition).string() + "\n" +
					tr("Residue") + " = " + QString::number(residue);
		label_FitResult->setText(text);
		pushButton_SetInputBeam->setEnabled(true);
		pushButton_SetTargetBeam->setEnabled(true);
	}

	m_updatingFit = false;
}

// Control callback

void GaussianBeamWidget::on_comboBox_Fit_currentIndexChanged(int index)
{
	updateFitInformation(index);
}

void GaussianBeamWidget::on_pushButton_AddFit_clicked()
{
	int index = m_bench->nFit();
	m_bench->addFit(index, 3);
}

void GaussianBeamWidget::on_pushButton_RemoveFit_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	m_bench->removeFit(index);
}

void GaussianBeamWidget::on_pushButton_RenameFit_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	bool ok;
	QString text = QInputDialog::getText(this, tr("Rename fit"),
		tr("Enter a new name for the current fit:"), QLineEdit::Normal,
		m_bench->fit(index)->name().c_str(), &ok);
	if (ok && !text.isEmpty())
		m_bench->fit(index)->setName(text.toUtf8().data());
}

void GaussianBeamWidget::on_pushButton_FitColor_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	QColor color = QColorDialog::getColor(Qt::black, this);
	m_bench->fit(index)->setColor(color.rgb());
}

void GaussianBeamWidget::on_comboBox_FitOrientation_currentIndexChanged(int dataIndex)
{
	int index = comboBox_Fit->currentIndex();
	if ((index < 0) || m_updatingFit)
		return;

	Fit* fit = m_bench->fit(index);
	fit->setOrientation(Orientation(dataIndex));
}

void GaussianBeamWidget::on_comboBox_FitData_currentIndexChanged(int dataIndex)
{
	int index = comboBox_Fit->currentIndex();
	if ((index < 0) || m_updatingFit)
		return;

	Fit* fit = m_bench->fit(index);
	fit->setDataType(FitDataType(dataIndex));
}

void GaussianBeamWidget::fitModelChanged(const QModelIndex& start, const QModelIndex& stop)
{
	int index = comboBox_Fit->currentIndex();
	if ((index < 0) || m_updatingFit)
		return;

	Fit* fit = m_bench->fit(index);

	for (int row = start.row(); row <= stop.row(); row++)
	{
		double position = fitModel->data(fitModel->index(row, 0)).toDouble()*Units::getUnit(UnitPosition).multiplier();
		double value = fitModel->data(fitModel->index(row, 1)).toDouble()*Units::getUnit(UnitWaist).multiplier();
		fit->setData(row, position, value);
	}
}

void GaussianBeamWidget::on_pushButton_FitAddRow_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	m_bench->fit(index)->addData(0., 0.);
}

void GaussianBeamWidget::on_pushButton_FitRemoveRow_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	QList<int> removedRows;

	for (int row = fitModel->rowCount() - 1; row >= 0; row--)
		if (fitSelectionModel->isRowSelected(row, QModelIndex()) && (row < m_bench->fit(index)->size()))
			removedRows << row;

	foreach (int row, removedRows)
		m_bench->fit(index)->removeData(row);
}

void GaussianBeamWidget::on_pushButton_SetInputBeam_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	Beam inputBeam = *m_bench->inputBeam();
	m_bench->fit(index)->applyFit(inputBeam);
	m_bench->setInputBeam(inputBeam);
}

void GaussianBeamWidget::on_pushButton_SetTargetBeam_clicked()
{
	int index = comboBox_Fit->currentIndex();
	if (index < 0)
		return;

	Beam targetBeam = *m_bench->targetBeam();
	m_bench->fit(index)->applyFit(targetBeam);
	m_bench->setTargetBeam(targetBeam);
}

void GaussianBeamWidget::displayShowTargetBeam(bool show)
{
	checkBox_ShowTargetBeam->setCheckState(show ? Qt::Checked : Qt::Unchecked);
}

// OpticsBench callbacks

void GaussianBeamWidget::onOpticsBenchFitAdded(int index)
{
	comboBox_Fit->insertItem(index, QString::fromUtf8(m_bench->fit(index)->name().c_str()));
	comboBox_Fit->setCurrentIndex(index);
}

void GaussianBeamWidget::onOpticsBenchFitsRemoved(int index, int count)
{
	for (int i = index + count - 1; i >= 0; i--)
		comboBox_Fit->removeItem(i);
}

void GaussianBeamWidget::onOpticsBenchFitDataChanged(int index)
{
	int comboIndex = comboBox_Fit->currentIndex();
	if ((comboIndex < 0) || (comboIndex != index))
		return;

	updateFitInformation(index);
}
