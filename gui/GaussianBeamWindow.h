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

#include "gui/GaussianBeamWidget.h"
#include "gui/GaussianBeamModel.h"
#include "gui/GaussianBeamDelegate.h"
#include "gui/OpticsView.h"
#include "ui_GaussianBeamWindow.h"
#include "src/Optics.h"
#include "src/OpticsBench.h"

#include <QMainWindow>

class GaussianBeamWindow : public QMainWindow, private Ui::GaussianBeamWindow
{
	Q_OBJECT

public:
	GaussianBeamWindow(const QString& fileName);

public slots:
	void updateWidget(const QModelIndex& topLeft, const QModelIndex& bottomRight);

	/// @todo put this protected once the load stuff is moved
	void on_WavelengthSpinBox_valueChanged(double wavelength);

protected slots:
	void on_action_Open_triggered()               { openFile();                        }
	void on_action_Save_triggered()               { saveFile(m_currentFile);           }
	void on_action_SaveAs_triggered()             { saveFile();                        }
	void on_action_AddOptics_triggered();
	void on_action_RemoveOptics_triggered();
	void on_action_AddLens_triggered()            { insertOptics(LensType);            }
	void on_action_AddFlatMirror_triggered()      { insertOptics(FlatMirrorType);      }
	void on_action_AddCurvedMirror_triggered()    { insertOptics(CurvedMirrorType);    }
	void on_action_AddFlatInterface_triggered()   { insertOptics(FlatInterfaceType);   }
	void on_action_AddCurvedInterface_triggered() { insertOptics(CurvedInterfaceType); }
	void on_action_AddGenericABCD_triggered()     { insertOptics(GenericABCDType);     }


private:
	void openFile(const QString& path = QString());
	void saveFile(const QString& path = QString());
	void setCurrentFile(const QString& path);
	void insertOptics(OpticsType opticsType);

private:
	QToolBar* m_fileToolBar;

	OpticsBench m_bench;
	GaussianBeamWidget* m_widget;
	GaussianBeamModel* m_model;
	QItemSelectionModel* m_selectionModel;
	QTableView* m_table;
	OpticsItemView* m_opticsItemView;
	OpticsView* m_opticsView;
	OpticsScene* m_opticsScene;


	QString m_currentFile;
};

#endif
