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
	m_opticsElements.insert(CreateBeamType, "createBeam");
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
		QMessageBox::information(window(), tr("XML error"), tr("The file is not an GaussianBeam file."));
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
	if ((majorVersion == 1) && ((minorVersion >= 0) || (minorVersion <= 2)))
	{
		m_bench->removeOptics(0, m_bench->nOptics());
		m_bench->removeFits(0, m_bench->nFit());
		if (minorVersion == 0)
			parseXml10(root);
		else if (minorVersion >= 1)
			parseXml(root);
	}
	else
	{
		QMessageBox::information(window(), tr("XML error"), tr("Your version of GaussianBeam is too old."));
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
			m_bench->setWavelength(child.text().toDouble());
		else if (child.tagName() == "leftBoundary")
			m_bench->setLeftBoundary(child.text().toDouble());
		else if (child.tagName() == "rightBoundary")
			m_bench->setRightBoundary(child.text().toDouble());
		else if (child.tagName() == "targetBeam")
			parseTargetBeam(child);
		else if (child.tagName() == "beamFit")
			parseFit(child);
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
			m_bench->lockTo(i, lockTree[i].toInt());
}

void GaussianBeamWindow::parseTargetBeam(const QDomElement& element)
{
	Beam targetBeam = *m_bench->targetBeam();
	m_bench->setTargetOrientation(Spherical); // Default for compatibility with old file versions
	parseBeam(element, targetBeam);
	m_bench->setTargetBeam(targetBeam);
}

#include <iostream>
using namespace std;

void GaussianBeamWindow::parseBeam(const QDomElement& element, Beam& beam)
{
	QDomElement child = element.firstChildElement();

	// Alternative names are for the 1.0 or 1.1 file version
	while (!child.isNull())
	{
		/// @todo showTargetBeam is missing
		if (child.tagName() == "horizontalWaist")
			beam.setWaist(child.text().toDouble(), Horizontal);
		else if (child.tagName() == "verticalWaist")
			beam.setWaist(child.text().toDouble(), Vertical);
		else if ((child.tagName() == "waist") || (child.tagName() == "targetWaist")) // Old file version
			beam.setWaist(child.text().toDouble(), Spherical);
		else if (child.tagName() == "horizontalPosition")
		{
			cerr << "horizontalPosition " << beam << endl;
			beam.setWaistPosition(child.text().toDouble(), Horizontal);
		}
		else if (child.tagName() == "verticalPosition")
			beam.setWaistPosition(child.text().toDouble(), Vertical);
		else if ((child.tagName() == "position") || (child.tagName() == "waistPosition") ||(child.tagName() == "targetPosition")) // Old file version
			beam.setWaistPosition(child.text().toDouble(), Spherical);
		else if (child.tagName() == "wavelength")
			beam.setWavelength(child.text().toDouble());
		/// @todo should we load angle and origin ?
//		else if (child.tagName() == "angle")
//			beam.setAngle(child.text().toDouble());
		else if (child.tagName() == "index")
			beam.setIndex(child.text().toDouble());
		else if (child.tagName() == "M2")
			beam.setM2(child.text().toDouble());
		// The next tags are specific to target beams
		else if ((child.tagName() == "targetOverlap") || (child.tagName() == "minOverlap"))
			m_bench->setTargetOverlap(child.text().toDouble());
		else if (child.tagName() == "targetOrientation")
			m_bench->setTargetOrientation(Orientation(child.text().toInt()));
		else if (child.tagName() == "overlapCriterion")
			; // Deprecated
		else if (child.tagName() == "positionTolerance")
			; // Deprecated
		else if (child.tagName() == "waistTolerance")
			; // Deprecated
		else
			qDebug() << " -> Unknown tag in parseBeam: " << child.tagName();
		child = child.nextSiblingElement();
	}
}

void GaussianBeamWindow::parseFit(const QDomElement& element)
{
	QDomElement child = element.firstChildElement();
	Fit* fit = m_bench->addFit(m_bench->nFit());

	// Alternative names are for the 1.0 file version
	while (!child.isNull())
	{
		if (child.tagName() == "name")
			fit->setName(child.text().toUtf8().data());
		else if ((child.tagName() == "dataType") || (child.tagName() == "fitDataType"))
			fit->setDataType(FitDataType(child.text().toInt()));
		else if (child.tagName() == "color")
			fit->setColor(child.text().toUInt());
		else if (child.tagName() == "orientation")
			fit->setOrientation(Orientation(child.text().toUInt()));
		else if ((child.tagName() == "data") || (child.tagName() == "fitData"))
		{
			QDomElement dataElement = child.firstChildElement();
			double position = 0.;
			double value = 0.;
			while (!dataElement.isNull())
			{
				if ((dataElement.tagName() == "position") || (dataElement.tagName() == "dataPosition"))
					position = dataElement.text().toDouble();
				else if ((dataElement.tagName() == "value") || (dataElement.tagName() == "dataValue"))
					value = dataElement.text().toDouble();
				else
					qDebug() << " -> Unknown tag: " << dataElement.tagName();
				dataElement = dataElement.nextSiblingElement();
			}
			fit->addData(position, value);
		}
		else
			qDebug() << " -> Unknown tag: " << child.tagName();
		child = child.nextSiblingElement();
	}

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
	else if (element.tagName() == "inputBeam")
	{
		parseInputBeam11(element, lockTree);
		return;
	}
	else
		qDebug() << " -> Unknown tag in parseOptics: " << element.tagName();

	if (!optics)
		return;

	optics->setId(element.attribute("id").toInt());
	QDomElement child = element.firstChildElement();
	lockTree.push_back(QString());

	while (!child.isNull())
	{
		if (child.tagName() == "position")
			optics->setPosition(child.text().toDouble());
		else if (child.tagName() == "angle")
			optics->setAngle(child.text().toDouble());
		else if (child.tagName() == "orientation")
			optics->setOrientation(Orientation(child.text().toInt()));
		else if (child.tagName() == "name")
			optics->setName(child.text().toUtf8().data());
		else if (child.tagName() == "absoluteLock")
			optics->setAbsoluteLock(child.text().toInt() == 1 ? true : false);
		else if (child.tagName() == "relativeLockParent")
			lockTree.back() = child.text();
		else if (child.tagName() == "width")
			optics->setWidth(child.text().toDouble());
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
		else if (child.tagName() == "beam")
		{
			Beam inputBeam;
			parseBeam(child, inputBeam);
			dynamic_cast<CreateBeam*>(optics)->setBeam(inputBeam);
			std::cerr << "Loaded createBeam " << inputBeam << endl;
		}
		else
			qDebug() << " -> Unknown tag in parseOptics: " << child.tagName();

		child = child.nextSiblingElement();
	}

	m_bench->addOptics(optics, m_bench->nOptics());
}

void GaussianBeamWindow::parseView(const QDomElement& element)
{
	QDomElement child = element.firstChildElement();

	while (!child.isNull())
	{
		if (child.tagName() == "horizontalRange")
			m_hOpticsView->setHorizontalRange(child.text().toDouble());
		else if (child.tagName() == "verticalRange")
			m_hOpticsView->setVerticalRange(child.text().toDouble());
		else if (child.tagName() == "origin")
			m_hOpticsView->setOrigin(child.text().toDouble());
		else if (child.tagName() == "showTargetBeam")
			showTargetBeam(child.text().toInt());
		else
			qDebug() << " -> Unknown tag: " << element.tagName();

		child = child.nextSiblingElement();
	}
}

/////////////////////////////////////////////////
// Compatibility with old file version

void GaussianBeamWindow::parseInputBeam11(const QDomElement& element, QList<QString>& lockTree)
{
	CreateBeam* createBeam = new CreateBeam(1., 1., 1., "");
	Beam inputBeam;

 	createBeam->setId(element.attribute("id").toInt());
	QDomElement child = element.firstChildElement();
	lockTree.push_back(QString());

	while (!child.isNull())
	{
		if (child.tagName() == "position")
			inputBeam.setWaistPosition(child.text().toDouble(), Spherical);
		else if (child.tagName() == "waist")
			inputBeam.setWaist(child.text().toDouble(), Spherical);
		else if (child.tagName() == "name")
			createBeam->setName(child.text().toUtf8().data());
		else if (child.tagName() == "absoluteLock")
			createBeam->setAbsoluteLock(child.text().toInt() == 1 ? true : false);
		else if (child.tagName() == "relativeLockParent")
			lockTree.back() = child.text();
		else
			qDebug() << " -> Unknown tag in parseOptics: " << child.tagName();

		child = child.nextSiblingElement();
	}

	createBeam->setBeam(inputBeam);
	m_bench->addOptics(createBeam, m_bench->nOptics());

}

void GaussianBeamWindow::parseXml10(const QDomElement& element)
{
	QDomElement child = element.firstChildElement();

	QList<QString> lockTree;

	double hRange = 0.0;
	double hOffset = 0.0;

	while (!child.isNull())
	{
		if (child.tagName() == "wavelength")
			m_bench->setWavelength(child.text().toDouble());
		else if (child.tagName() == "magicWaist")
			parseTargetBeam(child);
		else if (child.tagName() == "waistFit")
			parseFit(child);
		else if (child.tagName() == "display")
			parseXml10(child);
		else if (child.tagName() == "HRange")
			hRange = child.text().toDouble();
		else if (child.tagName() == "VRange")
			m_hOpticsView->setVerticalRange(child.text().toDouble());
		else if (child.tagName() == "HOffset")
			hOffset = child.text().toDouble();
		else if (child.tagName() == "inputBeam")
			parseInputBeam11(child, lockTree);
		else if (m_opticsElements.values().contains(child.tagName()))
			parseOptics(child, lockTree);
		else
			qDebug() << " -> Unknown tag: " << child.tagName();

		child = child.nextSiblingElement();
	}

	if (hRange != 0.)
	{
		m_bench->setLeftBoundary(hOffset);
		m_bench->setRightBoundary(hOffset + hRange);
		m_hOpticsView->setOrigin(hOffset);
		m_hOpticsView->setHorizontalRange(hOffset + hRange);
	}

	if (m_bench->nFit() > 0)
		m_bench->fit(0)->setName("Fit");

	for (int i = 0; i < lockTree.size(); i++)
		if (!lockTree[i].isEmpty())
			m_bench->lockTo(i, lockTree[i].toUtf8().data());
}

