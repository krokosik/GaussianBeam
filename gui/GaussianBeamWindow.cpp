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
#include "gui/OpticsView.h"
#include "gui/Unit.h"

#include <QDebug>
#include <QSplitter>
#include <QToolBar>
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QDockWidget>

GaussianBeamWindow::GaussianBeamWindow(const QString& fileName)
	: QMainWindow()
{
	m_currentFile = QString();

	setupUi(this);

	// Table
	m_model = new GaussianBeamModel(m_bench, this);
	m_table = new QTableView(this);
	m_table->setModel(m_model);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_selectionModel = new QItemSelectionModel(m_model);
	m_table->setSelectionModel(m_selectionModel);
	m_table->resizeColumnsToContents();
	GaussianBeamDelegate* delegate = new GaussianBeamDelegate(this, m_model, m_bench);
	m_table->setItemDelegate(delegate);

	for (int i = 1; i < 3; i++)
		m_bench.addOptics(LensType, m_bench.nOptics());

	// View
	m_opticsItemView = new OpticsItemView(m_bench, this);
	m_opticsItemView->setModel(m_model);
	m_opticsItemView->setSelectionModel(m_selectionModel);
	m_opticsScene = new OpticsScene(m_bench, this);
	m_opticsView = new OpticsView(m_opticsScene);
	m_opticsView->setHorizontalRange(0.81);
	m_opticsView->setVerticalRange(0.002);

	// Widget
	m_widget = new GaussianBeamWidget(m_bench, m_opticsItemView, m_opticsView, m_opticsScene, this);

	// Wavelength widget
	QWidget* wavelengthWidget = new QWidget(this);
	QVBoxLayout* wavelengthLayout = new QVBoxLayout(wavelengthWidget);
	QLabel* wavelengthLabel = new QLabel(tr("Wavelength"), wavelengthWidget);
	QDoubleSpinBox* wavelengthSpinBox = new QDoubleSpinBox(wavelengthWidget);
	wavelengthSpinBox->setDecimals(0);
	wavelengthSpinBox->setSuffix(" nm");
	wavelengthSpinBox->setRange(1., 9999.);
	wavelengthSpinBox->setValue(532.);
	wavelengthSpinBox->setSingleStep(10.);
	wavelengthLayout->addWidget(wavelengthSpinBox);
	wavelengthLayout->addWidget(wavelengthLabel);
	wavelengthWidget->setLayout(wavelengthLayout);
	connect(wavelengthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(on_WavelengthSpinBox_valueChanged(double)));

	// Bars
	m_fileToolBar = addToolBar(tr("File"));
	m_fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	m_fileToolBar->addAction(action_Open);
	m_fileToolBar->addAction(action_Save);
	m_fileToolBar->addAction(action_SaveAs);
	m_fileToolBar->addSeparator();
	m_fileToolBar->addAction(action_AddOptics);
	m_fileToolBar->addAction(action_RemoveOptics);
	m_fileToolBar->addSeparator();
	m_fileToolBar->addWidget(wavelengthWidget);

	statusBar()->showMessage(tr("Ready"));
	m_opticsItemView->setStatusBar(statusBar());
	m_opticsView->setStatusBar(statusBar());

	// Layouts
	QSplitter *splitter = new QSplitter(Qt::Vertical, this);
	splitter->addWidget(m_table);
	splitter->addWidget(m_opticsItemView);
	splitter->addWidget(m_opticsView);
	QDockWidget* dock = new QDockWidget(this);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	dock->setWidget(m_widget);
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	setCentralWidget(splitter);

	// Connect signal and slots
	connect(m_model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
	        this, SLOT(updateWidget(const QModelIndex&, const QModelIndex&)));

	// Load values
	on_WavelengthSpinBox_valueChanged(wavelengthSpinBox->value());

	if (!fileName.isEmpty())
		openFile(fileName);
}

void GaussianBeamWindow::on_action_AddOptics_triggered()
{
	QWidget* button = m_fileToolBar->widgetForAction(action_AddOptics);

	QMenu menu(this);
	menu.addAction(action_AddLens);
	menu.addAction(action_AddFlatMirror);
	menu.addAction(action_AddCurvedMirror);
	menu.addAction(action_AddFlatInterface);
	menu.addAction(action_AddCurvedInterface);
	menu.addAction(action_AddGenericABCD);
	menu.exec(button->mapToGlobal(QPoint(0, button->height())));
}

void GaussianBeamWindow::on_action_RemoveOptics_triggered()
{
	/// @bug this does not work for block selection with MAJ !
	for (int row = m_model->rowCount() - 1; row >= 0; row--)
		if ((m_bench.optics(row)->type() != CreateBeamType) &&
		    m_selectionModel->isRowSelected(row, QModelIndex()))
			m_bench.removeOptics(row);
}

void GaussianBeamWindow::on_WavelengthSpinBox_valueChanged(double wavelength)
{
	m_bench.setWavelength(wavelength*Units::getUnit(UnitWavelength).multiplier());
}

void GaussianBeamWindow::insertOptics(OpticsType opticsType)
{
	QModelIndex index = m_selectionModel->currentIndex();
	int row = m_model->rowCount();
	if (index.isValid())
		row = index.row() + 1;

	m_bench.addOptics(opticsType, row);
	m_table->resizeColumnsToContents();
}

void GaussianBeamWindow::openFile(const QString& path)
{
	QString fileName = path;

	if (fileName.isNull())
		fileName = QFileDialog::getOpenFileName(this, tr("Choose a data file"), "", "*.xml");
	if (fileName.isEmpty())
		return;

	if (m_widget->openFile(fileName))
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

	if (m_widget->saveFile(fileName))
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

void GaussianBeamWindow::updateWidget(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/)
{
	m_widget->displayOverlap();
	m_table->resizeRowsToContents();
}

void GaussianBeamWindow::updateView(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/)
{
	m_opticsItemView->updateViewport();
}
