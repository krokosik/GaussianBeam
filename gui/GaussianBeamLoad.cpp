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

void GaussianBeamWindow::initSaveVariables()
{
	m_opticsElements.clear();
	m_opticsElements.insert(CreateBeamType, "inputBeam");
	m_opticsElements.insert(LensType, "lens");
	m_opticsElements.insert(FlatMirrorType, "flatMirror");
	m_opticsElements.insert(CurvedMirrorType, "curvedMirror");
	m_opticsElements.insert(FlatInterfaceType, "flatInterface");
	m_opticsElements.insert(CurvedInterfaceType, "curvedInterface");
	m_opticsElements.insert(DielectricSlabType, "dielectricSlab");
	m_opticsElements.insert(GenericABCDType, "genericABCD");
}

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
	qDebug() << "File version" << majorVersion << minorVersion;
	if ((majorVersion == 1) && (minorVersion == 0))
	{
		m_bench.removeOptics(0, m_bench.nOptics());
		//parseXml10(root);
	}
	else if ((majorVersion == 1) && (minorVersion == 1))
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

void GaussianBeamWindow::parseXml(const QDomElement& element)
{
	QDomElement child = element.firstChildElement();

	while (!child.isNull())
	{
		if (child.tagName() == "bench")
			parseBench(child);
		else if (child.tagName() == "view")
			parseView(child);
		else
			qDebug() << " -> Unknown tag: " << element.tagName();
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
		else if (child.tagName() == "rightBoundary")
			m_bench.setRightBoundary(child.text().toDouble());
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
				else
					qDebug() << " -> Unknown tag: " << element.tagName();
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
						else
							qDebug() << " -> Unknown tag: " << element.tagName();
						fitDataElement = fitDataElement.nextSiblingElement();
					}
					fit.addData(position, value);
				}
				else
					qDebug() << " -> Unknown tag: " << element.tagName();
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
	Optics* optics = 0;

	if (element.tagName() == m_opticsElements[CreateBeamType])
		optics = new CreateBeam(1., 1., 1., "");
	else if (element.tagName() == m_opticsElements[LensType])
		optics = new Lens(1., 1., "");
	else if (element.tagName() == m_opticsElements[FlatMirrorType])
		optics = new FlatMirror(1., "");
	else if (element.tagName() == m_opticsElements[CurvedMirrorType])
		optics = new CurvedMirror(1., 1., "");
	else if (element.tagName() == m_opticsElements[FlatInterfaceType])
		optics = new FlatInterface(1., 1., "");
	else if (element.tagName() == m_opticsElements[CurvedInterfaceType])
		optics = new CurvedInterface(1., 1., 1., "");
	else if (element.tagName() == m_opticsElements[DielectricSlabType])
		optics = new DielectricSlab(1., 1., 1., "");
	else if (element.tagName() == m_opticsElements[GenericABCDType])
		optics = new GenericABCD(1., 1., 1., 1., 1., 1., "");
	else
		qDebug() << " -> Unknown tag: " << element.tagName();

	if (!optics)
		return;

	QDomElement child = element.firstChildElement();
	lockTree.push_back(QString());

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
		else if (child.tagName() == "index")
			dynamic_cast<CreateBeam*>(optics)->setIndex(child.text().toDouble());
		else if (child.tagName() == "M2")
			dynamic_cast<CreateBeam*>(optics)->setM2(child.text().toDouble());
		else if (child.tagName() == "waistPosition") // For compatibility
			dynamic_cast<CreateBeam*>(optics)->setPosition(child.text().toDouble());
		else if (child.tagName() == "focal")
			dynamic_cast<Lens*>(optics)->setFocal(child.text().toDouble());
		else if (child.tagName() == "curvatureRadius")
			dynamic_cast<CurvedMirror*>(optics)->setCurvatureRadius(child.text().toDouble());
		else if (child.tagName() == "indexRatio")
			dynamic_cast<Dielectric*>(optics)->setIndexRatio(child.text().toDouble());
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
	QDomElement child = element.firstChildElement();

	while (!child.isNull())
	{
		if (child.tagName() == "horizontalRange")
			m_opticsView->setHorizontalRange(child.text().toDouble());
		else if (child.tagName() == "verticalRange")
			m_opticsView->setVerticalRange(child.text().toDouble());
		else if (child.tagName() == "origin")
			m_opticsView->setOrigin(child.text().toDouble());
		else if (child.tagName() == "showTargetBeam")
			m_opticsScene->showTargetBeam(child.text().toInt());
		else
			qDebug() << " -> Unknown tag: " << element.tagName();

		child = child.nextSiblingElement();
	}
}
