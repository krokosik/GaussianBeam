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
#include <QBuffer>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QtXml/QDomDocument>
#include <QtXmlPatterns/QXmlQuery>

#include "xslt/1_0_to_1_1.cpp"
#include "xslt/1_1_to_1_2.cpp"

/*

Change log for GaussianBeam files. All this changes are coded in an XSL-T document
able to automatically perform file format conversion

File versions:
==============

GaussianBeam 0.1 -> file version 1.0
GaussianBeam 0.2 -> file version 1.0
GaussianBeam 0.3 -> file version 1.0
GaussianBeam 0.4 -> file version 1.1
GaussianBeam 0.5 -> file version 1.2

GaussianBeam 0.2 (1.0)
======================

	New <flatMirror> and <genericABCD> tags

GaussianBeam 0.3 (1.0)
======================

	<scramble> deprecated
	New <showTargetWaist>, <absoluteLock> and <relativeLockParent> tags

GaussianBeam 0.4 (1.1)
======================

	New <bench id="0"> and <view id="0" bench="0"> tags

	<magicWaist> -> <targetBeam id="0">
		<targetWaist> -> <waist>
		<targetPosition> -> <position>
		New <minOverlap> and <overlapCriterion> tags
		<showTargetWaist> -> <showTargetBeam> (in <view>)

	<waistFit> -> <beamFit id="n">
		New <name> and <color> tags
		<fitDataType> -> <dataType>
		<fitData> -> <data id = "n">
			<dataPosition> -> <position>
			<dataValue> -> <value>

	<display> dropped. Values transfered to <view>

	New <opticsList> tag

	New <dielectricSlab> and <curvedMirror> tags

GaussianBeam 0.5 (1.2)
======================

*/

bool GaussianBeamWindow::parseFile(const QString& fileName)
{
	QFile file(fileName);
	if (!(file.open(QFile::ReadOnly | QFile::Text)))
	{
		QMessageBox::warning(this, tr("Opening file"), tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return false;
	}

	QXmlQuery query(QXmlQuery::XSLT20);
	// Converstion from 1.0 to 1.1
	QString data_1_1;
	query.setFocus(&file);
	query.setQuery(xslt_1_0_to_1_1);
	query.evaluateTo(&data_1_1);
	// Converstion from 1.1 to 1.2
	QString data_1_2;
		QByteArray array_1_1 = data_1_1.toUtf8();
		QBuffer buffer(&array_1_1);
		buffer.open(QIODevice::ReadOnly);
	query.setFocus(&buffer); // in Qt 4.6, all this buffer business will be replaced by query.setFocus(data_1_1);
	query.setQuery(xslt_1_1_to_1_2);
	query.evaluateTo(&data_1_2);
		buffer.close();

//	qDebug() << data_1_2;

	// Parse XML file
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;

	if (!domDocument.setContent(data_1_2, true, &errorStr, &errorLine, &errorColumn))
	{
		QMessageBox::information(window(), tr("XML error"), tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr));
		file.close();
		return false;
	}

	file.close();

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

	if (root.attribute("version") != "1.2")
	{
		QMessageBox::information(window(), tr("XML error"), tr("Your version of GaussianBeam is too old."));
		return false;
	}

	m_bench->clear();
	parseXml(root);

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

	while (!child.isNull())
	{
		/// @todo showTargetBeam is missing
		if (child.tagName() == "horizontalWaist")
			beam.setWaist(child.text().toDouble(), Horizontal);
		else if (child.tagName() == "verticalWaist")
			beam.setWaist(child.text().toDouble(), Vertical);
		else if (child.tagName() == "waist")
			beam.setWaist(child.text().toDouble(), Spherical);
		else if (child.tagName() == "horizontalPosition")
		{
			cerr << "horizontalPosition " << beam << endl;
			beam.setWaistPosition(child.text().toDouble(), Horizontal);
		}
		else if (child.tagName() == "verticalPosition")
			beam.setWaistPosition(child.text().toDouble(), Vertical);
		else if (child.tagName() == "position")
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
		/// @todo should come with the orientation tag of the beam ?
		else if (child.tagName() == "targetOrientation")
			m_bench->setTargetOrientation(Orientation(child.text().toInt()));
		else
			qDebug() << " -> Unknown tag in parseBeam: " << child.tagName();
		child = child.nextSiblingElement();
	}
}

void GaussianBeamWindow::parseFit(const QDomElement& element)
{
	QDomElement child = element.firstChildElement();
	Fit* fit = m_bench->addFit(m_bench->nFit());

	while (!child.isNull())
	{
		if (child.tagName() == "name")
			fit->setName(child.text().toUtf8().data());
		else if (child.tagName() == "dataType")
			fit->setDataType(FitDataType(child.text().toInt()));
		else if (child.tagName() == "color")
			fit->setColor(child.text().toUInt());
		else if (child.tagName() == "orientation")
			fit->setOrientation(Orientation(child.text().toUInt()));
		else if (child.tagName() == "data")
		{
			QDomElement dataElement = child.firstChildElement();
			double position = 0.;
			bool added = false;
			while (!dataElement.isNull())
			{
				if (dataElement.tagName() == "position")
					position = dataElement.text().toDouble();
				else if (dataElement.tagName() == "value")
				{
					double value = dataElement.text().toDouble();
					Orientation orientation = Orientation(dataElement.attribute("orientation").toInt());
					if (added)
						fit->setData(fit->size() - 1, position, value, orientation);
					else
					{
						fit->addData(position, value, orientation);
						added = true;
					}
				}
				else
					qDebug() << " -> Unknown tag: " << dataElement.tagName();
				dataElement = dataElement.nextSiblingElement();
			}
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
//		else if (child.tagName() == "verticalRange")
//			m_hOpticsView->setVerticalRange(child.text().toDouble());
		else if (child.tagName() == "origin")
			m_hOpticsView->setOrigin(child.text().toDouble());
		else if (child.tagName() == "showTargetBeam")
			showTargetBeam(child.text().toInt());
		else
			qDebug() << " -> Unknown tag: " << element.tagName();

		child = child.nextSiblingElement();
	}
}

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
