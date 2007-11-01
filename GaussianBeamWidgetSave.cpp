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
#include "Unit.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QtXml/QDomDocument>
#include <QtXml/QXmlStreamWriter>

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
	QList<QString> lockTree;

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
		else if (child.tagName() == "showTargetWaist")
			checkBox_ShowTargetWaist->setCheckState(Qt::CheckState(child.text().toInt()));
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
		         (child.tagName() == "flatMirror") ||
		         (child.tagName() == "curvedMirror") ||
		         (child.tagName() == "flatInterface") ||
		         (child.tagName() == "curvedInterface") ||
		         (child.tagName() == "genericABCD"))
			parseXmlOptics(child, lockTree);
		else
			qDebug() << " -> Unknown tag: " << child.tagName();

		child = child.nextSiblingElement();
	}

	for (int i = 0; i < lockTree.size(); i++)
		if (!lockTree[i].isEmpty())
			/// @todo transfert this logic to OpticsBench when in operation
			/// @todo check for and get rid of other occurences of setData outside GaussianBeamDelegate
			model->setData(model->index(i, COL_LOCK), lockTree[i]);

}

void GaussianBeamWidget::parseXmlOptics(const QDomElement& element, QList<QString>& lockTree)
{
	Optics* optics;

	lockTree.push_back(QString());

	if (element.tagName() == "inputBeam")
		optics = new CreateBeam(1., 1., "");
	else if (element.tagName() == "lens")
		optics = new Lens(1., 1., "");
	else if (element.tagName() == "flatMirror")
		optics = new FlatMirror(1., "");
	else if (element.tagName() == "curvedMirror")
		optics = new CurvedMirror(1., 1., "");
	else if (element.tagName() == "flatInterface")
		optics = new FlatInterface(1., 1., "");
	else if (element.tagName() == "curvedInterface")
		optics = new CurvedInterface(1., 1., 1., "");
	else if (element.tagName() == "genericABCD")
		optics = new GenericABCD(1., 1., 1., 1., 1., 1., "");
	else
	{
		qDebug() << " -> Unknown tag: " << element.tagName();
		return;
	}

	QDomElement child = element.firstChildElement();

	while (!child.isNull())
	{
		if (child.tagName() == "position")
			optics->setPosition(child.text().toDouble());
		else if (child.tagName() == "name")
			optics->setName(child.text().toUtf8().data());
		else if (child.tagName() == "absoluteLock")
			optics->setAbsoluteLock(child.text().toInt() == 1 ? true : false);
		else if (child.tagName() == "relativeLockParent")
			lockTree.back() = child.text();
		else if (child.tagName() == "width")
			optics->setWidth(child.text().toDouble());
		else if (child.tagName() == "waist")
			dynamic_cast<CreateBeam*>(optics)->setWaist(child.text().toDouble());
		else if (child.tagName() == "waistPosition") // For compatibility
			dynamic_cast<CreateBeam*>(optics)->setPosition(child.text().toDouble());
		else if (child.tagName() == "focal")
			dynamic_cast<Lens*>(optics)->setFocal(child.text().toDouble());
		else if (child.tagName() == "curvatureRadius")
			dynamic_cast<CurvedMirror*>(optics)->setCurvatureRadius(child.text().toDouble());
		else if (child.tagName() == "indexRatio")
			dynamic_cast<Interface*>(optics)->setIndexRatio(child.text().toDouble());
		else if (child.tagName() == "surfaceRadius")
			dynamic_cast<CurvedInterface*>(optics)->setSurfaceRadius(child.text().toDouble());
		else if (child.tagName() == "A")
			dynamic_cast<GenericABCD*>(optics)->setA(child.text().toDouble());
		else if (child.tagName() == "B")
			dynamic_cast<GenericABCD*>(optics)->setB(child.text().toDouble());
		else if (child.tagName() == "C")
			dynamic_cast<GenericABCD*>(optics)->setC(child.text().toDouble());
		else if (child.tagName() == "D")
			dynamic_cast<GenericABCD*>(optics)->setD(child.text().toDouble());
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
		xmlWriter.writeTextElement("showTargetWaist", QString::number(checkBox_ShowTargetWaist->checkState()));
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
		const Optics& optics = model->optics(row);

		if (optics.type() == CreateBeamType)
		{
			xmlWriter.writeStartElement("inputBeam");
			xmlWriter.writeTextElement("waist", QString::number(dynamic_cast<const CreateBeam&>(optics).waist()));
		}
		else if (optics.type() == LensType)
		{
			xmlWriter.writeStartElement("lens");
			xmlWriter.writeTextElement("focal", QString::number(dynamic_cast<const Lens&>(optics).focal()));
		}
		else if (optics.type() == FlatMirrorType)
		{
			xmlWriter.writeStartElement("flatMirror");
		}
		else if (optics.type() == CurvedMirrorType)
		{
			xmlWriter.writeStartElement("curvedMirror");
			xmlWriter.writeTextElement("curvatureRadius", QString::number(dynamic_cast<const CurvedMirror&>(optics).curvatureRadius()));
		}
		else if (optics.type() == FlatInterfaceType)
		{
			xmlWriter.writeStartElement("flatInterface");
			xmlWriter.writeTextElement("indexRatio", QString::number(dynamic_cast<const FlatInterface&>(optics).indexRatio()));
		}
		else if (optics.type() == CurvedInterfaceType)
		{
			xmlWriter.writeStartElement("curvedInterface");
			xmlWriter.writeTextElement("indexRatio", QString::number(dynamic_cast<const CurvedInterface&>(optics).indexRatio()));
			xmlWriter.writeTextElement("surfaceRadius", QString::number(dynamic_cast<const CurvedInterface&>(optics).surfaceRadius()));
		}
		else if (optics.type() == GenericABCDType)
		{
			xmlWriter.writeStartElement("genericABCD");
			xmlWriter.writeTextElement("width", QString::number(optics.width()));
			xmlWriter.writeTextElement("A", QString::number(dynamic_cast<const GenericABCD&>(optics).A()));
			xmlWriter.writeTextElement("B", QString::number(dynamic_cast<const GenericABCD&>(optics).B()));
			xmlWriter.writeTextElement("C", QString::number(dynamic_cast<const GenericABCD&>(optics).C()));
			xmlWriter.writeTextElement("D", QString::number(dynamic_cast<const GenericABCD&>(optics).D()));
		}
		xmlWriter.writeTextElement("position", QString::number(optics.position()));
		xmlWriter.writeTextElement("name", QString(optics.name().c_str()));
		xmlWriter.writeTextElement("absoluteLock", QString::number(optics.absoluteLock() ? true : false));
		if (optics.relativeLockParent())
			xmlWriter.writeTextElement("relativeLockParent", QString(optics.relativeLockParent()->name().c_str()));
		xmlWriter.writeEndElement();
	}
	xmlWriter.writeEndElement();
	xmlWriter.writeEndDocument();
	file.close();

	setCurrentFile(fileName);
}
