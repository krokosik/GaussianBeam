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

#ifndef GAUSSIANBEAMWINDOWS_H
#define GAUSSIANBEAMWINDOW_H

#include "GaussianBeamWidget.h"
#include "ui_GaussianBeamWindow.h"

#include <QMainWindow>

class GaussianBeamWindow : public QMainWindow, private Ui::GaussianBeamWindow
{
	Q_OBJECT

public:
	GaussianBeamWindow(const QString& fileName);

protected slots:
	void on_action_Open_triggered();
	void on_action_Save_triggered();
	void on_action_SaveAs_triggered();

private:
	QToolBar* m_fileToolBar;

	GaussianBeamWidget m_widget;

	QString m_currentFile;
};

#endif
