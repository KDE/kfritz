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

#ifndef KFRITZDBUSSERVICE_H
#define KFRITZDBUSSERVICE_H
#include <KMainWindow>
#include <QObject>
#include <QDBusConnection>


/**
  DBus Service for KFritz
  will register at org.kde.KFritz
  */
class KFritzDbusService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KFritz")

    public:
        KFritzDbusService(KMainWindow *parent);


    public Q_SLOTS:
        /**
          dial a number through kfritz
          */
        void dialNumber(const QString &number);
};
#endif
