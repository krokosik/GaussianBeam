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

#include "gui/GaussianBeamWindow.h"
#include "gui/Unit.h"
#include "src/GaussianFit.h"

#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QtXml/QDomDocument>
#include <QtXml/QXmlStreamWriter>

bool GaussianBeamWindow::parseFile(const QString& fileName)
{
	QFile file(fileName);
	if (!(file.open(QFile::ReadOnly | QFile::Text)))
	{
		QMessageBox::warning(this, tr("Opening file"), tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return false;
	}

	// Parsing XML file
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;

	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn))
	{
		QMessageBox::information(window(), tr("XML error"), tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr));
		return false;
	}

	// XML version
	QDomElement root = domDocument.documentElement();
	if (root.tagName() != "gaussianBeam")
	{
		QMessageBox::information(window(), tr("XML error"), tr("The file is not an Gaussian Beam file."));
		return false;
	}

	if (!root.hasAttribute("version"))
	{
		QMessageBox::information(window(), tr("XML error"), tr("This file does not contain any version information."));
		return false;
	}
	QString version = root.attribute("version");
	QStringList versionList = version.split(".");
	if (versionList.size() != 2)
	{
		QMessageBox::information(window(), tr("XML error"), tr("Wrong version format. Your file seems corrupted"));
		return false;
	}
	int majorVersion = versionList[0].toInt();
	int minorVersion = versionList[1].toInt();
	qDebug() << "version" << majorVersion << minorVersion;
	if ((majorVersion == 1) && (minorVersion == 0))
	{
		m_bench.removeOptics(0, m_bench.nOptics());
		//parseXml10(root);
	}
	if ((majorVersion == 1) && (minorVersion == 1))
	{
		m_bench.removeOptics(0, m_bench.nOptics());
		parseXml(root);
	}
	else
	{
		QMessageBox::information(window(), tr("XML error"), tr("Your version of Gaussian Beam is too old."));
		return false;
	}

	file.close();

	return true;
}

/////////////////////////////////////////////////
// Read functions

void GaussianBeamWindow::parseXml(const QDomElement& element)
{
	QDomElement child = element.firstChildElement();

	while (!child.isNull())
	{
		if (child.tagName() == "bench")
			parseBench(child);
		else if (child.tagName() == "view")
			parseView(child);
		child = child.nextSiblingElement();
	}

}

void GaussianBeamWindow::parseBench(const QDomElement& element)
{
	QDomElement child = element.firstChildElement();
	QList<QString> lockTree;

	while (!child.isNull())
	{
		if (child.tagName() == "wavelength")
			m_bench.setWavelength(child.text().toDouble());
		else if (child.tagName() == "leftBoundary")
			m_bench.setLeftBoundary(child.text().toDouble());
		else if (child.tagName() == "targetBeam")
		{
			QDomElement targetBeamElement = child.firstChildElement();
			/// @todo check targetbeam wavelength
			Beam targetBeam = m_bench.targetBeam();
			while (!targetBeamElement.isNull())
			{
				if (targetBeamElement.tagName() == "position")
					targetBeam.setWaistPosition(child.text().toDouble());
				if (targetBeamElement.tagName() == "waist")
					targetBeam.setWaist(child.text().toDouble());
				targetBeamElement = targetBeamElement.nextSiblingElement();
			}
			m_bench.setTargetBeam(targetBeam);
		}
		else if (child.tagName() == "beamFit")
		{
			QDomElement fitElement = child.firstChildElement();
			while (!fitElement.isNull())
			{
				Fit& fit = m_bench.addFit(m_bench.nFit());
				if (fitElement.tagName() == "name")
					fit.setName(child.text().toUtf8().data());
				if (fitElement.tagName() == "dataType")
					fit.setDataType(FitDataType(child.text().toInt()));
				if (fitElement.tagName() == "color")
					fit.setColor(child.text().toInt());
				if (fitElement.tagName() == "data")
				{
					QDomElement fitDataElement = fitElement.firstChildElement();
					double position = 0.;
					double value = 0.;
					while (!fitDataElement.isNull())
					{
						if (fitDataElement.tagName() == "position")
							position = child.text().toDouble();
						if (fitDataElement.tagName() == "value")
							value = child.text().toDouble();
					}
					fit.addData(position, value);
				}
				fitElement = fitElement.nextSiblingElement();
			}
		}
		else if (child.tagName() == "opticsList")
		{
			QDomElement opticsElement = child.firstChildElement();
			while (!opticsElement.isNull())
			{
				parseOptics(opticsElement, lockTree);
				opticsElement = opticsElement.nextSiblingElement();
			}
		}
		else
			qDebug() << " -> Unknown tag: " << child.tagName();

		child = child.nextSiblingElement();
	}


	for (int i = 0; i < lockTree.size(); i++)
		if (!lockTree[i].isEmpty())
			m_bench.lockTo(i, lockTree[i].toUtf8().data());

}

void GaussianBeamWindow::parseOptics(const QDomElement& element, QList<QString>& lockTree)
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

	m_bench.addOptics(optics, m_bench.nOptics());
}

void GaussianBeamWindow::parseView(const QDomElement& element)
{
}

/////////////////////////////////////////////////
// Write functions

bool GaussianBeamWindow::writeFile(const QString& fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("Saving file"), tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return false;
	}

	QXmlStreamWriter xmlWriter(&file);
	xmlWriter.setAutoFormatting(true);
	xmlWriter.writeStartDocument("1.0");
	xmlWriter.writeDTD("<!DOCTYPE gaussianBeam>");
	xmlWriter.writeStartElement("gaussianBeam");
	xmlWriter.writeAttribute("version", "1.1");
		xmlWriter.writeStartElement("bench");
		xmlWriter.writeAttribute("id", "0");
			writeBench(xmlWriter);
		xmlWriter.writeEndElement();
		xmlWriter.writeStartElement("view");
		xmlWriter.writeAttribute("id", "0");
		xmlWriter.writeAttribute("bench", "0");
			writeView(xmlWriter);
		xmlWriter.writeEndElement();
	xmlWriter.writeEndElement();
	xmlWriter.writeEndDocument();

	file.close();
	return true;
}

void GaussianBeamWindow::writeBench(QXmlStreamWriter& xmlWriter)
{
	xmlWriter.writeTextElement("wavelength", QString::number(m_bench.wavelength()));
	xmlWriter.writeTextElement("leftBoundary", QString::number(m_bench.leftBoundary()));
	xmlWriter.writeTextElement("rightBoundary", QString::number(m_bench.rightBoundary()));

	xmlWriter.writeStartElement("targetBeam");
	xmlWriter.writeAttribute("id", "0");
		xmlWriter.writeTextElement("position", QString::number(m_bench.targetBeam().waistPosition()));
		xmlWriter.writeTextElement("waist", QString::number(m_bench.targetBeam().waist()));
	xmlWriter.writeEndElement();

	for (int i = 0; i < m_bench.nFit(); i++)
	{
		Fit& fit = m_bench.fit(i);
		xmlWriter.writeStartElement("beamFit");
		xmlWriter.writeAttribute("id", QString::number(i));
			xmlWriter.writeTextElement("name", fit.name().c_str());
			xmlWriter.writeTextElement("dataType", QString::number(int(fit.dataType())));
			xmlWriter.writeTextElement("color", QString::number(fit.color()));
			for (int j = 0; j < fit.size(); j++)
			{
				xmlWriter.writeStartElement("data");
					xmlWriter.writeTextElement("position", QString::number(fit.position(j)));
					xmlWriter.writeTextElement("value",  QString::number(fit.value(j)));
				xmlWriter.writeEndElement();
			}
		xmlWriter.writeEndElement();
	}

	xmlWriter.writeStartElement("opticsList");
	for (int i = 0; i < m_bench.nOptics(); i++)
		writeOptics(xmlWriter, m_bench.optics(i));
	xmlWriter.writeEndElement();
}

void GaussianBeamWindow::writeOptics(QXmlStreamWriter& xmlWriter, const Optics* optics)
{
	if (optics->type() == CreateBeamType)
	{
		xmlWriter.writeStartElement("inputBeam");
		xmlWriter.writeTextElement("waist", QString::number(dynamic_cast<const CreateBeam*>(optics)->waist()));
	}
	else if (optics->type() == LensType)
	{
		xmlWriter.writeStartElement("lens");
		xmlWriter.writeTextElement("focal", QString::number(dynamic_cast<const Lens*>(optics)->focal()));
	}
	else if (optics->type() == FlatMirrorType)
	{
		xmlWriter.writeStartElement("flatMirror");
	}
	else if (optics->type() == CurvedMirrorType)
	{
		xmlWriter.writeStartElement("curvedMirror");
		xmlWriter.writeTextElement("curvatureRadius", QString::number(dynamic_cast<const CurvedMirror*>(optics)->curvatureRadius()));
	}
	else if (optics->type() == FlatInterfaceType)
	{
		xmlWriter.writeStartElement("flatInterface");
		xmlWriter.writeTextElement("indexRatio", QString::number(dynamic_cast<const FlatInterface*>(optics)->indexRatio()));
	}
	else if (optics->type() == CurvedInterfaceType)
	{
		xmlWriter.writeStartElement("curvedInterface");
		xmlWriter.writeTextElement("indexRatio", QString::number(dynamic_cast<const CurvedInterface*>(optics)->indexRatio()));
		xmlWriter.writeTextElement("surfaceRadius", QString::number(dynamic_cast<const CurvedInterface*>(optics)->surfaceRadius()));
	}
	else if (optics->type() == GenericABCDType)
	{
		xmlWriter.writeStartElement("genericABCD");
		xmlWriter.writeTextElement("width", QString::number(optics->width()));
		xmlWriter.writeTextElement("A", QString::number(dynamic_cast<const GenericABCD*>(optics)->A()));
		xmlWriter.writeTextElement("B", QString::number(dynamic_cast<const GenericABCD*>(optics)->B()));
		xmlWriter.writeTextElement("C", QString::number(dynamic_cast<const GenericABCD*>(optics)->C()));
		xmlWriter.writeTextElement("D", QString::number(dynamic_cast<const GenericABCD*>(optics)->D()));
	}
	xmlWriter.writeTextElement("position", QString::number(optics->position()));
	xmlWriter.writeTextElement("name", QString(optics->name().c_str()));
	xmlWriter.writeTextElement("absoluteLock", QString::number(optics->absoluteLock() ? true : false));
	if (optics->relativeLockParent())
		xmlWriter.writeTextElement("relativeLockParent", QString(optics->relativeLockParent()->name().c_str()));
	xmlWriter.writeEndElement();
}

void GaussianBeamWindow::writeView(QXmlStreamWriter& xmlWriter)
{
/*	xmlWriter.writeTextElement("showTargetWaist", QString::number(checkBox_ShowTargetBeam->checkState()));
	xmlWriter.writeTextElement("waistTolerance", QString::number(doubleSpinBox_WaistTolerance->value()/100.));
	xmlWriter.writeTextElement("positionTolerance", QString::number(doubleSpinBox_PositionTolerance->value()/100.));
	xmlWriter.writeTextElement("HRange", QString::number(doubleSpinBox_HRange->value()*Units::getUnit(UnitPosition).multiplier()));
	xmlWriter.writeTextElement("VRange", QString::number(doubleSpinBox_VRange->value()*Units::getUnit(UnitPosition).multiplier()));
	xmlWriter.writeTextElement("HOffset", QString::number(doubleSpinBox_HOffset->value()*Units::getUnit(UnitPosition).multiplier()));*/
}
