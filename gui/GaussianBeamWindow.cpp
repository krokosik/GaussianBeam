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

#include <QDebug>
#include <QToolBar>

GaussianBeamWindow::GaussianBeamWindow(const QString& fileName)
	: QMainWindow()
	, m_widget(fileName, this)
{
	setupUi(this);

	m_fileToolBar = addToolBar(tr("File"));
	m_fileToolBar->addAction(action_Open);
	m_fileToolBar->addAction(action_Save);
	m_fileToolBar->addAction(action_SaveAs);

	setCentralWidget(&m_widget);
}

void GaussianBeamWindow::on_action_Open_triggered()
{
	qDebug() << "Open";
}

void GaussianBeamWindow::on_action_Save_triggered()
{
	qDebug() << "Save";
}

void GaussianBeamWindow::on_action_SaveAs_triggered()
{
	qDebug() << "Save as";
}
