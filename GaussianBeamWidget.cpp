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
//#include "GaussianBeamPlot.h"
#include "Unit.h"


#include <QApplication>
#include <QPushButton>
#include <QStandardItemModel>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QtXml/QDomDocument>
#include <QtXml/QXmlStreamWriter>

#include <cmath>

GaussianBeamWidget::GaussianBeamWidget(QWidget *parent)
	: QWidget(parent)
{
	m_lastLensName = 0;
	m_currentFile = QString();

	setupUi(this);
	//toolBox->setSizeHint(100);
	toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));

	// Extra widgets, not included in designer
	QVBoxLayout* layout = new QVBoxLayout(this);
	QSplitter *splitter = new QSplitter(Qt::Vertical, this);
//	plot = new GaussianBeamPlot(this);
	opticsView = new OpticsView(this);
//	splitter->addWidget(plot);
	splitter->addWidget(opticsView);
	layout->addWidget(splitter);
	layout->setMargin(0);
	widget->setLayout(layout);

	// Create model & views
	model = new GaussianBeamModel(this);
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
	connect(fitModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
	        this, SLOT(updateView(const QModelIndex&, const QModelIndex&)));

	// Set up default values
	on_doubleSpinBox_Wavelength_valueChanged(doubleSpinBox_Wavelength->value());
	m_lastLensName = 2;
	m_lastFlatInterfaceName = 0;
	m_lastCurvedInterfaceName = 0;
	updateUnits();
}

void GaussianBeamWidget::on_doubleSpinBox_Wavelength_valueChanged(double value)
{
	opticsView->setWavelength(value*Units::getUnit(UnitWavelength).multiplier());
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
	menu.addAction(action_AddFlatInterface);
	menu.addAction(action_AddCurvedInterface);
	menu.exec(pushButton_Add->mapToGlobal(QPoint(0, pushButton_Add->height())));
}

void GaussianBeamWidget::on_action_AddLens_triggered()
{
	QString name = "L" + QString::number(++m_lastLensName);
	double position = model->optics(model->rowCount()-1)->position() + 0.05;
	model->addOptics(new Lens(0.1, position, name.toUtf8().data()), model->rowCount());
	table->resizeColumnsToContents();
}

void GaussianBeamWidget::on_action_AddFlatInterface_triggered()
{
	QString name = "I" + QString::number(++m_lastFlatInterfaceName);
	double position = model->optics(model->rowCount()-1)->position() + 0.05;
	model->addOptics(new FlatInterface(1.5, position, name.toUtf8().data()), model->rowCount());
	table->resizeColumnsToContents();
}

void GaussianBeamWidget::on_action_AddCurvedInterface_triggered()
{
	QString name = "C" + QString::number(++m_lastCurvedInterfaceName);
	double position = model->optics(model->rowCount()-1)->position() + 0.05;
	model->addOptics(new CurvedInterface(0.1, 1.5, position, name.toUtf8().data()), model->rowCount());
	table->resizeColumnsToContents();
	table->resizeRowToContents(model->rowCount()-1);
}

void GaussianBeamWidget::on_pushButton_Remove_clicked()
{
	/// @todo transfer some logic to GaussianBeamModel
	for (int row = model->rowCount() - 1; row >= 0; row--)
		if ((model->optics(row)->type() != CreateBeamType) &&
		    selectionModel->isRowSelected(row, QModelIndex()))
			model->removeRow(row);
}

///////////////////////////////////////////////////////////
// MAGIC WAIST PAGE

void GaussianBeamWidget::on_pushButton_MagicWaist_clicked()
{
	label_MagicWaistResult->setText("");

	Beam inputBeam;
	inputBeam.setWavelength(model->wavelength());
	Beam targetBeam(doubleSpinBox_TargetWaist->value()*Units::getUnit(UnitWaist).multiplier(),
	                doubleSpinBox_TargetPosition->value()*Units::getUnit(UnitPosition).multiplier(),
	                inputBeam.wavelength());
	std::vector<Lens> lenses;

	/// @todo some cleaning is needed !
	for (int row = 0; row < model->rowCount(); ++row)
		if (model->optics(row)->type() == CreateBeamType)
			inputBeam = model->optics(row)->image(inputBeam);
		else if (model->optics(row)->type() == LensType)
			lenses.push_back(*dynamic_cast<const Lens*>(model->optics(row)));

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

}

///////////////////////////////////////////////////////////
// FIT PAGE

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

void GaussianBeamWidget::openFile(const QString &path)
{
	QString fileName = path;

	if (fileName.isNull())
		fileName = QFileDialog::getOpenFileName(this, tr("Choose a data file"), "", "*.xml");
	if (fileName.isEmpty())
		return;

	QFile file(fileName);
	if (!(file.open(QFile::ReadOnly | QFile::Text)))
	{
		QMessageBox::warning(this, tr("Opening file"), tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

	// Parsing XML file
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;

	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn))
	{
		QMessageBox::information(window(), tr("XML error"), tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr));
		return;
	}

	// XML version
	QDomElement root = domDocument.documentElement();
	if (root.tagName() != "gaussianBeam")
	{
		QMessageBox::information(window(), tr("XML error"), tr("The file is not an Gaussian Beam file."));
		return;
	}
	if (root.hasAttribute("version") && root.attribute("version").toDouble() > 1.)
	{
		QMessageBox::information(window(), tr("XML error"), tr("Your version of Gaussian Beam is too old."));
		return;
	}

	// Parse elements
	model->removeRows(0, model->rowCount());
	parseXml(root);
	file.close();

	setCurrentFile(fileName);
}

void GaussianBeamWidget::parseXml(const QDomElement& element)
{
	QDomElement child = element.firstChildElement();
	int fitRow = fitModel->rowCount() - 1;

	while (!child.isNull())
	{
		if (child.tagName() == "wavelength") ////////////////
			doubleSpinBox_Wavelength->setValue(child.text().toDouble()*Units::getUnit(UnitWavelength).divider());
		else if (child.tagName() == "magicWaist") ////////////////
			parseXml(child);
		else if (child.tagName() == "targetWaist")
			doubleSpinBox_TargetWaist->setValue(child.text().toDouble()*Units::getUnit(UnitWaist).divider());
		else if (child.tagName() == "waistTolerance")
			doubleSpinBox_WaistTolerance->setValue(child.text().toDouble()*100.);
		else if (child.tagName() == "targetPosition")
			doubleSpinBox_TargetPosition->setValue(child.text().toDouble()*Units::getUnit(UnitPosition).divider());
		else if (child.tagName() == "positionTolerance")
			doubleSpinBox_PositionTolerance->setValue(child.text().toDouble()*100.);
		else if (child.tagName() == "scramble")
			 checkBox_Scramble->setCheckState(Qt::CheckState(child.text().toInt()));
		else if (child.tagName() == "waistFit") ////////////////
		{
			fitModel->removeRows(0, fitModel->rowCount());
			parseXml(child);
		}
		else if (child.tagName() == "fitDataType")
			comboBox_FitData->setCurrentIndex(child.text().toInt());
		else if (child.tagName() == "fitData")
		{
			fitModel->insertRow(++fitRow);
			parseXml(child);
		}
		else if (child.tagName() == "dataPosition")
			fitModel->setData(fitModel->index(fitRow, 0), child.text().toDouble()*Units::getUnit(UnitPosition).divider());
		else if (child.tagName() == "dataValue")
			fitModel->setData(fitModel->index(fitRow, 1), child.text().toDouble()*Units::getUnit(UnitWaist).divider());
		else if (child.tagName() == "display") ////////////////
			parseXml(child);
		else if (child.tagName() == "HRange")
			doubleSpinBox_HRange->setValue(child.text().toDouble()*Units::getUnit(UnitPosition).divider());
		else if (child.tagName() == "VRange")
			doubleSpinBox_VRange->setValue(child.text().toDouble()*Units::getUnit(UnitPosition).divider());
		else if (child.tagName() == "HOffset")
			doubleSpinBox_HOffset->setValue(child.text().toDouble()*Units::getUnit(UnitPosition).divider());
		else if ((child.tagName() == "inputBeam") ||
		         (child.tagName() == "lens") ||
		         (child.tagName() == "flatInterface") ||
		         (child.tagName() == "curvedInterface"))
			parseXmlOptics(child);
		else
			qDebug() << " -> Unknown tag: " << child.tagName();

		child = child.nextSiblingElement();
	}
}

void GaussianBeamWidget::parseXmlOptics(const QDomElement& element)
{
	Optics* optics;

	if (element.tagName() == "inputBeam")
		optics = new CreateBeam(1., 1., "");
	else if (element.tagName() == "lens")
		optics = new Lens(1., 1., "");
	else if (element.tagName() == "flatInterface")
		optics = new FlatInterface(1., 1., "");
	else if (element.tagName() == "curvedInterface")
		optics = new CurvedInterface(1., 1., 1., "");
	else
		qDebug() << " -> Unknown tag: " << element.tagName();

	QDomElement child = element.firstChildElement();

	while (!child.isNull())
	{
		if (child.tagName() == "position")
			optics->setPosition(child.text().toDouble());
		else if (child.tagName() == "name")
			optics->setName(child.text().toUtf8().data());
		else if (child.tagName() == "focal")
			dynamic_cast<Lens*>(optics)->setFocal(child.text().toDouble());
		else if (child.tagName() == "waist")
			dynamic_cast<CreateBeam*>(optics)->setWaist(child.text().toDouble());
		else if (child.tagName() == "waistPosition") // For compatibility
			dynamic_cast<CreateBeam*>(optics)->setPosition(child.text().toDouble());
		else if (child.tagName() == "indexRatio")
			dynamic_cast<Interface*>(optics)->setIndexRatio(child.text().toDouble());
		else if (child.tagName() == "surfaceRadius")
			dynamic_cast<CurvedInterface*>(optics)->setSurfaceRadius(child.text().toDouble());
		else
			qDebug() << " -> Unknown tag: " << child.tagName();

		child = child.nextSiblingElement();
	}

	model->addOptics(optics, model->rowCount());
}

void GaussianBeamWidget::saveFile(const QString &path)
{
	QString fileName = path;

	if (fileName.isNull())
		fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::currentPath(), "*.xml");
	if (fileName.isEmpty())
		return;
	if (!fileName.endsWith(".xml"))
		fileName += ".xml";


	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("Saving file"), tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

	QXmlStreamWriter xmlWriter(&file);
	xmlWriter.setAutoFormatting(true);
	xmlWriter.writeStartDocument("1.0");
	xmlWriter.writeDTD("<!DOCTYPE gaussianBeam>");
	xmlWriter.writeStartElement("gaussianBeam");
	xmlWriter.writeAttribute("version", "1.0");
	xmlWriter.writeTextElement("wavelength", QString::number(model->wavelength()));
	xmlWriter.writeStartElement("magicWaist");
		xmlWriter.writeTextElement("targetWaist", QString::number(doubleSpinBox_TargetWaist->value()*Units::getUnit(UnitWaist).multiplier()));
		xmlWriter.writeTextElement("waistTolerance", QString::number(doubleSpinBox_WaistTolerance->value()/100.));
		xmlWriter.writeTextElement("targetPosition", QString::number(doubleSpinBox_TargetPosition->value()*Units::getUnit(UnitPosition).multiplier()));
		xmlWriter.writeTextElement("positionTolerance", QString::number(doubleSpinBox_PositionTolerance->value()/100.));
		xmlWriter.writeTextElement("scramble", QString::number(checkBox_Scramble->checkState()));
	xmlWriter.writeEndElement();
	xmlWriter.writeStartElement("waistFit");
		xmlWriter.writeTextElement("fitDataType", QString::number(comboBox_FitData->currentIndex()));
		for (int row = 0; row < fitModel->rowCount(); row++)
		{
			xmlWriter.writeStartElement("fitData");
				xmlWriter.writeTextElement("dataPosition", QString::number(fitModel->data(fitModel->index(row, 0)).toDouble()*Units::getUnit(UnitPosition).multiplier()));
				xmlWriter.writeTextElement("dataValue",  QString::number(fitModel->data(fitModel->index(row, 1)).toDouble()*Units::getUnit(UnitWaist).multiplier()));
			xmlWriter.writeEndElement();
		}
	xmlWriter.writeEndElement();
	xmlWriter.writeStartElement("display");
		xmlWriter.writeTextElement("HRange", QString::number(doubleSpinBox_HRange->value()*Units::getUnit(UnitPosition).multiplier()));
		xmlWriter.writeTextElement("VRange", QString::number(doubleSpinBox_VRange->value()*Units::getUnit(UnitPosition).multiplier()));
		xmlWriter.writeTextElement("HOffset", QString::number(doubleSpinBox_HOffset->value()*Units::getUnit(UnitPosition).multiplier()));
	xmlWriter.writeEndElement();
	for (int row = 0; row < model->rowCount(); row++)
	{
		const Optics* optics = model->optics(row);

		if (model->optics(row)->type() == CreateBeamType)
		{
			xmlWriter.writeStartElement("inputBeam");
			xmlWriter.writeTextElement("waist", QString::number(dynamic_cast<const CreateBeam*>(optics)->waist()));
		}
		else if (model->optics(row)->type() == LensType)
		{
			xmlWriter.writeStartElement("lens");
			xmlWriter.writeTextElement("focal", QString::number(dynamic_cast<const Lens*>(optics)->focal()));
		}
		else if (model->optics(row)->type() == FlatInterfaceType)
		{
			xmlWriter.writeStartElement("flatInterface");
			xmlWriter.writeTextElement("indexRatio", QString::number(dynamic_cast<const FlatInterface*>(optics)->indexRatio()));
		}
		else if (model->optics(row)->type() == CurvedInterfaceType)
		{
			xmlWriter.writeStartElement("curvedInterface");
			xmlWriter.writeTextElement("indexRatio", QString::number(dynamic_cast<const CurvedInterface*>(optics)->indexRatio()));
			xmlWriter.writeTextElement("surfaceRadius", QString::number(dynamic_cast<const CurvedInterface*>(optics)->surfaceRadius()));
		}

		xmlWriter.writeTextElement("position", QString::number(optics->position()));
		xmlWriter.writeTextElement("name", QString(optics->name().c_str()));
		xmlWriter.writeEndElement();
	}
	xmlWriter.writeEndElement();
	xmlWriter.writeEndDocument();
	file.close();

	setCurrentFile(fileName);
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

///////////////////////////////////////////////////////////
// General functions

void GaussianBeamWidget::updateView(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/)
{
	qDebug() << "UpdateView";
	opticsView->updateViewport();
}
