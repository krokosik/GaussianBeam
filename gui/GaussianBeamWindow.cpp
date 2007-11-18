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

#include "GaussianBeamWindow.h"
#include "OpticsView.h"

#include <QDebug>
#include <QToolBar>
#include <QFile>
#include <QFileDialog>

GaussianBeamWindow::GaussianBeamWindow(const QString& fileName)
	: QMainWindow()
	, m_widget(this)
{
	m_currentFile = QString();

	setupUi(this);

	m_fileToolBar = addToolBar(tr("File"));
	m_fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	m_fileToolBar->addAction(action_Open);
	m_fileToolBar->addAction(action_Save);
	m_fileToolBar->addAction(action_SaveAs);



	setCentralWidget(&m_widget);

	statusBar()->showMessage(tr("Ready"));
	m_widget.opticsItemView->setStatusBar(statusBar());
	m_widget.opticsView->setStatusBar(statusBar());

	if (!fileName.isEmpty())
		openFile(fileName);
}

void GaussianBeamWindow::on_action_Open_triggered()
{
	openFile();
}

void GaussianBeamWindow::on_action_Save_triggered()
{
	saveFile(m_currentFile);
}

void GaussianBeamWindow::on_action_SaveAs_triggered()
{
	saveFile();
}

void GaussianBeamWindow::openFile(const QString& path)
{
	QString fileName = path;

	if (fileName.isNull())
		fileName = QFileDialog::getOpenFileName(this, tr("Choose a data file"), "", "*.xml");
	if (fileName.isEmpty())
		return;

	if (m_widget.openFile(fileName))
	{
		setCurrentFile(fileName);
		statusBar()->showMessage(tr("File") + " " + QFileInfo(fileName).fileName() + " " + tr("loaded"));
	}
}

void GaussianBeamWindow::saveFile(const QString& path)
{
	QString fileName = path;

	if (fileName.isNull())
		fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::currentPath(), "*.xml");
	if (fileName.isEmpty())
		return;
	if (!fileName.endsWith(".xml"))
		fileName += ".xml";

	if (m_widget.saveFile(fileName))
	{
		setCurrentFile(fileName);
		statusBar()->showMessage(tr("File") + " " + QFileInfo(fileName).fileName() + " " + tr("saved"));
	}
}

void GaussianBeamWindow::setCurrentFile(const QString& path)
{
	m_currentFile = path;
	if (!m_currentFile.isEmpty())
		setWindowTitle(QFileInfo(m_currentFile).fileName() + " - GaussianBeam");
	else
		setWindowTitle("GaussianBeam");
}
