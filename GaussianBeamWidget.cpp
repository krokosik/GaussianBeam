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

#include "GaussianBeamModel.h"
#include "GaussianBeamWidget.h"
#include "GaussianBeamDelegate.h"
#include "OpticsView.h"
#ifdef GBPLOT
	#include "GaussianBeamPlot.h"
#endif
#include "Unit.h"

#include <QApplication>
#include <QPushButton>
#include <QStandardItemModel>
#include <QDebug>
#include <QMessageBox>
#include <QMenu>
#include <QFileInfo>

#include <cmath>

GaussianBeamWidget::GaussianBeamWidget(QString file, QWidget *parent)
	: QWidget(parent)
{
	m_currentFile = QString();
	m_lastLensName = 0;
	m_lastFlatMirrorName = 0;
	m_lastCurvedMirrorName = 0;
	m_lastFlatInterfaceName = 0;
	m_lastCurvedInterfaceName = 0;
	m_lastGenericABCDName = 0;

	setupUi(this);
	//toolBox->setSizeHint(100);
	toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));

	// Pointer creation
	model = new GaussianBeamModel(this);

	// Extra widgets, not included in designer
	QVBoxLayout* layout = new QVBoxLayout(this);
	QSplitter *splitter = new QSplitter(Qt::Vertical, this);
	opticsView = new OpticsView(this);
#ifdef GBPLOT
	plot = new GaussianBeamPlot(this, model);
	splitter->addWidget(plot);
	checkBox_ShowGraph->setVisible(true);
	checkBox_ShowGraph->setEnabled(true);
	//checkBox_ShowGraph->setChecked(true);
	plot->setVisible(checkBox_ShowGraph->isChecked());
#else
	checkBox_ShowGraph->setVisible(false);
	checkBox_ShowGraph->setEnabled(false);
#endif
	splitter->addWidget(opticsView);
	layout->addWidget(splitter);
	layout->setMargin(0);
	widget->setLayout(layout);

	// Create model & views
	table->setModel(model);
	opticsView->setModel(model);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	selectionModel = new QItemSelectionModel(model);
	table->setSelectionModel(selectionModel);
	opticsView->setSelectionModel(selectionModel);
	table->resizeColumnsToContents();
	opticsView->setStatusLabel(label_Status);
	delegate = new GaussianBeamDelegate(this, model);
	table->setItemDelegate(delegate);
	for (int i = 1; i < 3; i++)
	{
		m_lastLensName++;
		QString name = "L" + QString::number(i);
		model->addOptics(new Lens(0.02*i + 0.001, 0.1*i + 0.02, name.toUtf8().data()), model->rowCount());
	}


	// Waist fit
	fitModel = new QStandardItemModel(5, 2, this);
	fitModel->setHeaderData(0, Qt::Horizontal, tr("Position") + "\n(" + Units::getUnit(UnitPosition).prefix() + "m)");
	fitModel->setHeaderData(1, Qt::Horizontal, tr("Value") + "\n(" + Units::getUnit(UnitWaist).prefix() + "m)");
	fitTable->setModel(fitModel);
	fitTable->setColumnWidth(0, 82);
	fitTable->setColumnWidth(1, 82);
	comboBox_FitData->insertItem(0, tr("Radius @ 1/e²"));
	comboBox_FitData->insertItem(1, tr("Diameter @ 1/e²"));
	comboBox_FitData->setCurrentIndex(1);
	opticsView->setFitModel(fitModel);
	opticsView->setMeasureCombo(comboBox_FitData);

	// Connect slots
	/// @bug this does not react ?
	connect(fitModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
	        this, SLOT(updateView(const QModelIndex&, const QModelIndex&)));

	// Set up default values
	on_doubleSpinBox_Wavelength_valueChanged(doubleSpinBox_Wavelength->value());
	on_radioButton_Tolerance_toggled(radioButton_Tolerance->isChecked());
	updateUnits();

	if (!file.isEmpty())
		openFile(file);
}

void GaussianBeamWidget::on_doubleSpinBox_Wavelength_valueChanged(double value)
{
	model->setWavelength(value*Units::getUnit(UnitWavelength).multiplier());
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

void GaussianBeamWidget::on_pushButton_Add_clicked()
{
	QMenu menu(this);
	menu.addAction(action_AddLens);
	menu.addAction(action_AddFlatMirror);
	menu.addAction(action_AddCurvedMirror);
	menu.addAction(action_AddFlatInterface);
	menu.addAction(action_AddCurvedInterface);
	menu.addAction(action_AddGenericABCD);
	menu.exec(pushButton_Add->mapToGlobal(QPoint(0, pushButton_Add->height())));
}

void GaussianBeamWidget::insertOptics(Optics* optics, bool resizeRow)
{
	QModelIndex index = table->selectionModel()->currentIndex();
	int row = model->rowCount();
	if (index.isValid())
		row = index.row() + 1;

	optics->setPosition(model->optics(row-1).position() + 0.05);
	model->addOptics(optics, row);

	table->resizeColumnsToContents();
	if (resizeRow)
		table->resizeRowToContents(row);
}

void GaussianBeamWidget::on_action_AddLens_triggered()
{
	QString name = "L" + QString::number(++m_lastLensName);
	insertOptics(new Lens(0.1, 0.0, name.toUtf8().data()));
}

void GaussianBeamWidget::on_action_AddFlatMirror_triggered()
{
	QString name = "M" + QString::number(++m_lastFlatMirrorName);
	insertOptics(new FlatMirror(0.0, name.toUtf8().data()));
}

void GaussianBeamWidget::on_action_AddCurvedMirror_triggered()
 {
	QString name = "R" + QString::number(++m_lastCurvedMirrorName);
	insertOptics(new CurvedMirror(0.05, 0.0, name.toUtf8().data()));
}

void GaussianBeamWidget::on_action_AddFlatInterface_triggered()
{
	QString name = "I" + QString::number(++m_lastFlatInterfaceName);
	insertOptics(new FlatInterface(1.5, 0.0, name.toUtf8().data()));
}

void GaussianBeamWidget::on_action_AddCurvedInterface_triggered()
{
	QString name = "C" + QString::number(++m_lastCurvedInterfaceName);
	insertOptics(new CurvedInterface(0.1, 1.5, 0.0, name.toUtf8().data()), true);
}

void GaussianBeamWidget::on_action_AddGenericABCD_triggered()
{
	QString name = "G" + QString::number(++m_lastGenericABCDName);
	insertOptics(new GenericABCD(1.0, 0.2, 0.0, 1.0, 0.1, 0.0, name.toUtf8().data()), true);
}

void GaussianBeamWidget::on_pushButton_Remove_clicked()
{
	/// @todo transfer some logic to GaussianBeamModel
	for (int row = model->rowCount() - 1; row >= 0; row--)
		if ((model->optics(row).type() != CreateBeamType) &&
		    selectionModel->isRowSelected(row, QModelIndex()))
			model->removeRow(row);
}

///////////////////////////////////////////////////////////
// MAGIC WAIST PAGE

Beam GaussianBeamWidget::targetWaist()
{
	return Beam(doubleSpinBox_TargetWaist->value()*Units::getUnit(UnitWaist).multiplier(),
	            doubleSpinBox_TargetPosition->value()*Units::getUnit(UnitPosition).multiplier(),
	            model->wavelength());
}

void GaussianBeamWidget::displayOverlap()
{
	double overlap = GaussianBeam::overlap(model->beam(model->rowCount()-1), targetWaist());
	label_OverlapResult->setText(tr("Overlap: ") + QString::number(overlap*100., 'f', 2) + " " + tr("%"));
}

void GaussianBeamWidget::on_doubleSpinBox_TargetWaist_valueChanged(double value)
{
	Q_UNUSED(value);
	opticsView->setTargetWaist(targetWaist(), checkBox_ShowTargetWaist->checkState() == Qt::Checked);
	displayOverlap();
}

void GaussianBeamWidget::on_doubleSpinBox_TargetPosition_valueChanged(double value)
{
	Q_UNUSED(value);
	opticsView->setTargetWaist(targetWaist(), checkBox_ShowTargetWaist->checkState() == Qt::Checked);
	displayOverlap();
}

void GaussianBeamWidget::on_checkBox_ShowTargetWaist_toggled(bool checked)
{
	opticsView->setTargetWaist(targetWaist(), checked);
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

	Beam inputBeam;
	inputBeam.setWavelength(model->wavelength());
	Beam targetBeam = targetWaist();
	std::vector<Lens> lenses;

	/// @todo some cleaning is needed !
	for (int row = 0; row < model->rowCount(); ++row)
		if (model->optics(row).type() == CreateBeamType)
			inputBeam = model->optics(row).image(inputBeam);
		else if (model->optics(row).type() == LensType)
			lenses.push_back(dynamic_cast<const Lens&>(model->optics(row)));

	bool result = GaussianBeam::magicWaist(inputBeam, targetBeam, lenses,
	              doubleSpinBox_WaistTolerance->value()*0.01,
	              doubleSpinBox_PositionTolerance->value()*0.01,
	              checkBox_Scramble->checkState() == Qt::Checked);
	if (!result)
	{
		label_MagicWaistResult->setText(tr("Desired waist could not be found !"));
		return;
	}

	model->removeRows(1, model->rowCount()-1);

	for (unsigned int l = 0; l < lenses.size(); l++)
		model->addOptics(new Lens(lenses[l]), model->rowCount());

	displayOverlap();
}

///////////////////////////////////////////////////////////
// FIT PAGE

///@bug fill optics views as fit points are entered

void GaussianBeamWidget::on_pushButton_Fit_clicked()
{
	std::vector<double> positions, radii;
	double rho2;
	double factor = 1.;
	if (comboBox_FitData->currentIndex() == 1)
		factor = 0.5;

	for (int row = 0; row < fitModel->rowCount(); ++row)
		if (factor*fitModel->data(fitModel->index(row, 1)).toDouble() != 0.)
		{
			qDebug() << fitModel->data(fitModel->index(row, 0)).toDouble() << fitModel->data(fitModel->index(row, 1)).toDouble();
			positions.push_back(fitModel->data(fitModel->index(row, 0)).toDouble()*Units::getUnit(UnitPosition).multiplier());
			radii.push_back(factor*fitModel->data(fitModel->index(row, 1)).toDouble()*Units::getUnit(UnitWaist).multiplier());
		}

	if (positions.size() <= 1)
	{
		qDebug() << "Empty fit";
		return;
	}

	qDebug() << "Fitting" << positions.size() << "elements";

	m_fitBeam = GaussianBeam::fitBeam(positions, radii, model->wavelength(), &rho2);
	QString text = tr("Waist") + " = " + QString::number(m_fitBeam.waist()*Units::getUnit(UnitWaist).divider()) + Units::getUnit(UnitWaist).string("m") + "\n" +
	               tr("Position") + " = " + QString::number(m_fitBeam.waistPosition()*Units::getUnit(UnitPosition).divider()) + Units::getUnit(UnitPosition).string("m") + "\n" +
	               tr("R²") + " = " + QString::number(rho2);
	label_FitResult->setText(text);
	pushButton_SetInputBeam->setEnabled(true);
	pushButton_SetTargetBeam->setEnabled(true);
}

void GaussianBeamWidget::on_pushButton_SetInputBeam_clicked()
{
	model->setInputBeam(m_fitBeam);
}

void GaussianBeamWidget::on_pushButton_SetTargetBeam_clicked()
{
	doubleSpinBox_TargetWaist->setValue(m_fitBeam.waist()*Units::getUnit(UnitWaist).divider());
	doubleSpinBox_TargetPosition->setValue(m_fitBeam.waistPosition()*Units::getUnit(UnitPosition).divider());
}

///////////////////////////////////////////////////////////
// DISPLAY PAGE

void GaussianBeamWidget::setCurrentFile(const QString& path)
{
	m_currentFile = path;
	if (!m_currentFile.isEmpty())
		setWindowTitle(QFileInfo(m_currentFile).fileName() + " - GaussianBeam");
	else
		setWindowTitle("GaussianBeam");
}

void GaussianBeamWidget::on_pushButton_Open_clicked()
{
	openFile();
}

void GaussianBeamWidget::on_pushButton_Save_clicked()
{
	saveFile(m_currentFile);
}

void GaussianBeamWidget::on_pushButton_SaveAs_clicked()
{
	saveFile();
}

void GaussianBeamWidget::on_doubleSpinBox_HRange_valueChanged(double value)
{
	opticsView->setHRange(value*Units::getUnit(UnitHRange).multiplier());
}

void GaussianBeamWidget::on_doubleSpinBox_VRange_valueChanged(double value)
{
	opticsView->setVRange(value*Units::getUnit(UnitVRange).multiplier());
}

void GaussianBeamWidget::on_doubleSpinBox_HOffset_valueChanged(double value)
{
	opticsView->setHOffset(value*Units::getUnit(UnitHRange).multiplier());
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

///////////////////////////////////////////////////////////
// General functions

void GaussianBeamWidget::updateWidget(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/)
{
}

void GaussianBeamWidget::updateView(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/)
{
	qDebug() << "UpdateView";
	opticsView->updateViewport();
}
