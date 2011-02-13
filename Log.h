/*
 * KFritz
 *
 * Copyright (C) 2010 Joachim Wilke <kfritz@joachim-wilke.de>
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

#include <iostream>
#include <KTextEdit>

#include <Config.h> // logging macros DBG, INF, ERR
#undef NAMESPACE
#define NAMESPACE "kfritz"

#ifndef LOG_H_
#define LOG_H_

class LogBuf : public QObject, public std::streambuf {
	Q_OBJECT

public:
	enum eLogType {
		DEBUG,
		INFO,
		LOG_ERROR,
		NumLogTypes
	};
private:
	void	PutBuffer(void);
	void	PutChar(char c);
	eLogType type;
	KTextEdit *logWidget;
protected:
	int	overflow(int);
	int	sync();
public:
	LogBuf(eLogType type);
	virtual ~LogBuf();
	void setLogWidget(KTextEdit *te);
Q_SIGNALS:
	void signalAppend(QString m);
private Q_SLOTS:
	void slotAppend(QString m);
};

class LogStream: public std::ostream {
private:
	LogBuf *buffer;
	static LogStream *streams[LogBuf::NumLogTypes];
	LogStream(LogBuf::eLogType type); //-> use getLogStream()
public:
	LogStream *setLogWidget(KTextEdit *te);
	static LogStream *getLogStream(LogBuf::eLogType type);
};

#endif /* LOG_H_ */
