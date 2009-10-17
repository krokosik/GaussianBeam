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
#include "gui/OpticsView.h"
#include "gui/OpticsWidgets.h"
#include "gui/Unit.h"

#include <QDebug>
#include <QSplitter>
#include <QToolBar>
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QDockWidget>
#include <QSettings>
#include <QHeaderView>

GaussianBeamWindow::GaussianBeamWindow(const QString& fileName)
	: QMainWindow()
{
	m_currentFile = QString();
	initSaveVariables();

	setupUi(this);
	setWindowIcon(QIcon(":/images/gaussianbeam16.png"));

	// Bench
	m_bench = new OpticsBench();
	m_bench->populateDefault();
	m_bench->registerEventListener(this);

	// Table
	m_tableConfigWidget = new TablePropertySelector(this);
	m_tableConfigWidget->setWindowFlags(Qt::Window);
	m_tableCornerWidget = new CornerWidget(Qt::transparent, ":/images/preferences-system.png", m_tableConfigWidget, this);
	m_model = new GaussianBeamModel(m_bench, m_tableConfigWidget, this);
	m_table = new QTableView(this);
	m_table->setModel(m_model);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	//m_table->setShowGrid(false);
	m_table->verticalHeader()->hide();
	m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_table->setAlternatingRowColors(true);
	m_selectionModel = new QItemSelectionModel(m_model);
	m_table->setSelectionModel(m_selectionModel);
	m_table->resizeColumnsToContents();
	GaussianBeamDelegate* delegate = new GaussianBeamDelegate(this, m_model, m_bench);
	m_table->setItemDelegate(delegate);
	m_table->setCornerWidget(m_tableCornerWidget);
	connect(m_model, SIGNAL(modelReset()), m_table, SLOT(resizeColumnsToContents()));

	// View
	m_hOpticsScene = new OpticsScene(m_bench, Horizontal, this);
	m_vOpticsScene = new OpticsScene(m_bench, Vertical, this);
	m_hOpticsScene->setOtherScene(m_vOpticsScene);
	m_vOpticsScene->setOtherScene(m_hOpticsScene);
	m_hOpticsView = new OpticsView(m_hOpticsScene, m_bench);
	m_vOpticsView = new OpticsView(m_vOpticsScene, m_bench);

	// Widget
	m_widget = new GaussianBeamWidget(m_bench, this);

	// Wavelength widget
	QWidget* wavelengthWidget = new QWidget(this);
	QVBoxLayout* wavelengthLayout = new QVBoxLayout(wavelengthWidget);
	QLabel* wavelengthLabel = new QLabel(tr("Wavelength"), wavelengthWidget);
	m_wavelengthSpinBox = new QDoubleSpinBox(wavelengthWidget);
	m_wavelengthSpinBox->setDecimals(0);
	m_wavelengthSpinBox->setSuffix(" nm");
	m_wavelengthSpinBox->setRange(1., 999999.);
	m_wavelengthSpinBox->setValue(532.);
	m_wavelengthSpinBox->setSingleStep(10.);
	wavelengthLayout->addWidget(m_wavelengthSpinBox);
	wavelengthLayout->addWidget(wavelengthLabel);
	wavelengthWidget->setLayout(wavelengthLayout);
	connect(m_wavelengthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(wavelengthSpinBox_valueChanged(double)));

	// Bars
	m_fileToolBar = addToolBar(tr("File"));
	m_fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	m_fileToolBar->addAction(action_New);
	m_recentFilesMenu = new QMenu(this);
	for (int i = 0; i < m_maxRecentFiles; i++)
	{
		m_recentFileAction[i] = new QAction(this);
		m_recentFileAction[i]->setVisible(false);
		connect(m_recentFileAction[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
		m_recentFilesMenu->addAction(m_recentFileAction[i]);
	}
	action_Open->setMenu(m_recentFilesMenu);
	m_fileToolBar->addAction(action_Open);
	m_fileToolBar->addAction(action_Save);
	m_fileToolBar->addAction(action_SaveAs);
	m_fileToolBar->addSeparator();
	m_addOpticsMenu = new QMenu(this);
	m_addOpticsMenu->setDefaultAction(action_AddLens);
	m_addOpticsMenu->addAction(action_AddLens);
	m_addOpticsMenu->addAction(action_AddFlatMirror);
	m_addOpticsMenu->addAction(action_AddCurvedMirror);
	m_addOpticsMenu->addAction(action_AddFlatInterface);
	m_addOpticsMenu->addAction(action_AddCurvedInterface);
	m_addOpticsMenu->addAction(action_AddDielectricSlab);
	m_addOpticsMenu->addAction(action_AddGenericABCD);
	action_AddOptics->setMenu(m_addOpticsMenu);
	m_fileToolBar->addAction(action_AddOptics);
	m_fileToolBar->addAction(action_RemoveOptics);
	m_fileToolBar->addSeparator();
	m_fileToolBar->addWidget(wavelengthWidget);

	StatusWidget* statusWidget = new StatusWidget(statusBar());
	statusBar()->addWidget(statusWidget, 1);
	m_hOpticsView->setStatusWidget(statusWidget);
	m_vOpticsView->setStatusWidget(statusWidget);

	// Layouts
	QSplitter *splitter = new QSplitter(Qt::Vertical, this);
	splitter->addWidget(m_table);
	splitter->addWidget(m_hOpticsView);
	splitter->addWidget(m_vOpticsView);
	QList<int> sizes;
	sizes << 10 << 10 << 10;
	splitter->setSizes(sizes);
	QDockWidget* dock = new QDockWidget(this);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	dock->setWidget(m_widget);
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	setCentralWidget(splitter);

	// Connect signal and slots
	connect(m_model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
			this, SLOT(updateWidget(const QModelIndex&, const QModelIndex&)));

	readSettings();
	onOpticsBenchSphericityChanged();

	for (int i = 0; i < 2; i++)
		m_bench->addOptics(LensType, m_bench->nOptics());
/*
	Cavity& cavity = m_bench->cavity();
	cavity.addOptics(dynamic_cast<const ABCD*>(m_bench->optics(1)));
	cavity.addOptics(dynamic_cast<const ABCD*>(m_bench->optics(2)));
	m_bench->notifyCavityChange();
*/
	// NOTE: this has to be the last part of the constructor
	if (!fileName.isEmpty())
		openFile(fileName);
}

void GaussianBeamWindow::closeEvent(QCloseEvent* event)
{
	Q_UNUSED(event);
	writeSettings();
}

/////////////////////////////////////////////////
// Settings

void GaussianBeamWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("GaussianBeamWindow/size", size());
	settings.setValue("GaussianBeamWindow/pos", pos());
	settings.setValue("GaussianBeamWindow/wavelength", m_bench->wavelength());
}

void GaussianBeamWindow::readSettings()
{
	QSettings settings;
	resize(settings.value("GaussianBeamWindow/size", QSize(800, 600)).toSize());
	move(settings.value("GaussianBeamWindow/pos", QPoint(100, 100)).toPoint());
	m_bench->setWavelength(settings.value("GaussianBeamWindow/wavelength", 461e-9).toDouble());
	updateRecentFileActions();
}

/////////////////////////////////////////////////
// Wavelength

void GaussianBeamWindow::wavelengthSpinBox_valueChanged(double wavelength)
{
	m_bench->setWavelength(wavelength*Units::getUnit(UnitWavelength).multiplier());
}

void GaussianBeamWindow::onOpticsBenchWavelengthChanged()
{
	m_wavelengthSpinBox->setValue(m_bench->wavelength()*Units::getUnit(UnitWavelength).divider());
}

/////////////////////////////////////////////////
// Optics

void GaussianBeamWindow::insertOptics(OpticsType opticsType)
{
	QModelIndex index = m_selectionModel->currentIndex();
	int row = m_model->rowCount();
	if (index.isValid() && m_selectionModel->hasSelection())
		row = index.row() + 1;

	m_bench->addOptics(opticsType, row);
	m_table->resizeColumnsToContents();
}

void GaussianBeamWindow::on_action_RemoveOptics_triggered()
{
	for (int row = m_model->rowCount() - 1; row >= 0; row--)
		if ((m_bench->optics(row)->type() != CreateBeamType) &&
			m_selectionModel->isRowSelected(row, QModelIndex()))
			m_bench->removeOptics(row);
}

/////////////////////////////////////////////////
// File managment

void GaussianBeamWindow::newFile()
{
	writeSettings();
	GaussianBeamWindow* newWindow = new GaussianBeamWindow;
	newWindow->move(newWindow->pos() + QPoint(15, 15));
	newWindow->show();
}

void GaussianBeamWindow::openFile(const QString& path)
{
	QSettings settings;
	QString fileName = path;
	QString dir = settings.value("GaussianBeamWindow/lastDirectory", "").toString();

	if (fileName.isNull())
		fileName = QFileDialog::getOpenFileName(this, tr("Choose a data file"), dir, "*.xml");
	if (fileName.isEmpty())
		return;

	if (parseFile(fileName))
	{
		setCurrentFile(fileName);
		statusBar()->showMessage(tr("File") + " " + QFileInfo(fileName).fileName() + " " + tr("loaded"));
		settings.setValue("GaussianBeamWindow/lastDirectory", QFileInfo(fileName).path());
	}
}

void GaussianBeamWindow::openRecentFile()
{
	QAction* action = qobject_cast<QAction*>(sender());
	if (action)
		openFile(action->data().toString());
}

void GaussianBeamWindow::saveFile(const QString& path)
{
	QSettings settings;
	QString fileName = path;
	QString dir = settings.value("GaussianBeamWindow/lastDirectory", "").toString();

	if (fileName.isNull())
		fileName = QFileDialog::getSaveFileName(this, tr("Save File"), dir, "*.xml");
	if (fileName.isEmpty())
		return;
	if (!fileName.endsWith(".xml"))
		fileName += ".xml";

	if (writeFile(fileName))
	{
		setCurrentFile(fileName);
		statusBar()->showMessage(tr("File") + " " + QFileInfo(fileName).fileName() + " " + tr("saved"));
		settings.setValue("GaussianBeamWindow/lastDirectory", QFileInfo(fileName).path());
	}
}

void GaussianBeamWindow::setCurrentFile(const QString& fileName)
{
	m_currentFile = fileName;
	if (!m_currentFile.isEmpty())
		setWindowTitle(QFileInfo(m_currentFile).fileName() + " - GaussianBeam");
	else
		setWindowTitle("GaussianBeam");

	// Update the recent file list
	QSettings settings;
	QStringList files = settings.value("GaussianBeamWindow/recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);
	while (files.size() > m_maxRecentFiles)
		files.removeLast();
	settings.setValue("GaussianBeamWindow/recentFileList", files);

	foreach (QWidget* widget, QApplication::topLevelWidgets())
	{
		GaussianBeamWindow* window = qobject_cast<GaussianBeamWindow*>(widget);
		if (window)
			window->updateRecentFileActions();
	}
}

void GaussianBeamWindow::updateRecentFileActions()
{
	QSettings settings;
	QStringList files = settings.value("GaussianBeamWindow/recentFileList").toStringList();

	int numRecentFiles = qMin(files.size(), (int)m_maxRecentFiles);

	for (int i = 0; i < numRecentFiles; i++)
	{
		QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
		m_recentFileAction[i]->setText(text);
		m_recentFileAction[i]->setData(files[i]);
		m_recentFileAction[i]->setStatusTip(tr("Open file ") + files[i]);
		m_recentFileAction[i]->setVisible(true);
	}
	for (int i = numRecentFiles; i < m_maxRecentFiles; i++)
		m_recentFileAction[i]->setVisible(false);
}

void GaussianBeamWindow::updateWidget(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/)
{
	m_table->resizeRowsToContents();
}

/////////////////////////////////////////////////
// Views

void GaussianBeamWindow::showTargetBeam(bool visible)
{
	m_hOpticsScene->showTargetBeam(visible);
	m_vOpticsScene->showTargetBeam(visible);
}

void GaussianBeamWindow::onOpticsBenchSphericityChanged()
{
	if (m_bench->isSpherical())
		m_vOpticsView->hide();
	else
		m_vOpticsView->show();

	m_table->resizeRowsToContents();
}


