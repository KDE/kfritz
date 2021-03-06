/*
 * KFritz
 *
 * Copyright (C) 2010-2012 Joachim Wilke <kfritz@joachim-wilke.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef LIBFRITZINIT_H
#define LIBFRITZINIT_H

#include <QThread>

#include "libfritz++/Listener.h"

class LibFritzInit : public QThread {
	Q_OBJECT;
public:
    LibFritzInit(QString username, QString password, fritz::EventHandler *eventHandler);
	virtual ~LibFritzInit();
	void run();
    void setCredentials(QString username, QString password);
private:
    QString username;
	QString password;
	fritz::EventHandler *eventHandler;
Q_SIGNALS:
	void ready(bool isReady);
	void invalidPassword();
};

#endif /* LIBFRITZINIT_H_ */
