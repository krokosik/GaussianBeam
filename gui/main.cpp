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

#include "gui/GaussianBeamWidget.h"
#include "gui/GaussianBeamWindow.h"

#include <QApplication>
#include <QTextCodec>
#include <QTranslator>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QCoreApplication::setOrganizationName("GaussianBeam");
	QCoreApplication::setApplicationName("GaussianBeam");

	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF8"));
	QTranslator translator;
	QString locale = QLocale::system().name();
	translator.load(QString("GaussianBeam_") + locale);
	app.installTranslator(&translator);

	///@todo parse other arguments ?
	QString file;
	if (argc > 1)
		file = argv[1];

	GaussianBeamWindow window(file);
	window.show();
	return app.exec();
}
