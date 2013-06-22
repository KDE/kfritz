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

#include "KFonbookModel.h"

#include <QFont>
#include <KIcon>
#include <KLocalizedString>

#include "liblog++/Log.h"

/**
 * KFonbookModel
 */

enum modelColumns {
	COLUMN_NAME,
	COLUMN_QUICKDIAL,
	COLUMN_VANITY,
	COLUMNS_COUNT
};

KFonbookModel::KFonbookModel(std::string techID) {
	// get the fonbook resource
    fritz::Fonbooks *books = fritz::FonbookManager::GetFonbookManager()->getFonbooks();
	fonbook = (*books)[techID];
	lastRows = 0;
}

KFonbookModel::~KFonbookModel() {
}

int KFonbookModel::rowCount(const QModelIndex & parent) const
{
	// top level
	if (!parent.isValid())
		return fonbook->getFonbookSize();
	// sub level
	if (parent.isValid() && !parent.parent().isValid())
		return fonbook->retrieveFonbookEntry(parent.row())->getNumbers().size();
	// any other level
	return 0;
}

int KFonbookModel::columnCount(const QModelIndex & parent) const
{
	// top level
	if (!parent.isValid())
		return COLUMNS_COUNT;
	// sub level
	if (parent.isValid() && !parent.parent().isValid())
		return 2;
	// any other level
	return 0;
}

QVariant KFonbookModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();
	if (orientation == Qt::Horizontal) {
		switch (section) {
		case COLUMN_NAME:
			return i18n("Name");
		case COLUMN_QUICKDIAL:
			return i18n("Quickdial");
		case COLUMN_VANITY:
			return i18n("Vanity");
		default:
			return QVariant();
		}
	} else
		return QVariant();
}

QVariant KFonbookModel::data(const QModelIndex & index, int role) const {
	// top level
	if (!index.parent().isValid())
		return dataTopLevel(index, role);
	// sub level
	if (index.parent().isValid() && !index.parent().parent().isValid())
		return dataSubLevel(index, role);
	// any other level
	return QVariant();
}

QVariant KFonbookModel::dataSubLevel(const QModelIndex & index, int role) const {
	const fritz::FonbookEntry *fe = fonbook->retrieveFonbookEntry(index.parent().row());
	switch(role) {
	case Qt::FontRole:
		// default number in bold
		if (fe->getPriority(index.row()) == 1) {
			QFont font;
			font.setBold(true);
			return font;
		}
	case Qt::DisplayRole:
		switch (index.column()) {
		case 0:
			return fe->getNumber(index.row()).c_str();
		case 1:
			return getTypeName(fe->getType(index.row()));
		}
		return QVariant();
	case Qt::EditRole:
		switch (index.column()) {
		case 0:
			return fe->getNumber(index.row()).c_str();
		case 1:
			return getTypeName(fe->getType(index.row()));
		}
		return QVariant();
	}
	return QVariant();
}

QVariant KFonbookModel::dataTopLevel(const QModelIndex & index, int role) const {
	const fritz::FonbookEntry *fe = fonbook->retrieveFonbookEntry(index.row());
	switch(role) {
	case Qt::DecorationRole:
		if (index.column() == 0)
			return QVariant(fe->isImportant() ? KIcon("emblem-important") : KIcon("x-office-contact"));
		break;
	case Qt::ToolTipRole:
		if (index.column() == 0)
			return QVariant(fe->isImportant() ? i18n("Important contact") : "");
		break;
	case Qt::DisplayRole:
		switch(index.column()) {
		case COLUMN_NAME:
			return toUnicode(fe->getName());
		case COLUMN_QUICKDIAL:
			return toUnicode(fe->getQuickdialFormatted());
		case COLUMN_VANITY:
			return toUnicode(fe->getVanityFormatted());
		}
		break;
	case Qt::EditRole:
		switch(index.column()) {
		case COLUMN_NAME:
			return toUnicode(fe->getName());
		case COLUMN_QUICKDIAL:
			return toUnicode(fe->getQuickdial());
		case COLUMN_VANITY:
			return toUnicode(fe->getVanity());
		}
		break;
	}
	return QVariant();
}

bool KFonbookModel::setData (const QModelIndex & index, const QVariant & value, int role) {
	if (role == Qt::EditRole) {
		const fritz::FonbookEntry *_fe;
		if (!index.parent().isValid())
			_fe = fonbook->retrieveFonbookEntry(index.row());
		else
			_fe = fonbook->retrieveFonbookEntry(index.parent().row());
		fritz::FonbookEntry fe(*_fe);

		if (!index.parent().isValid()) {
			switch(index.column()) {
			case COLUMN_NAME:
				fe.setName(fromUnicode(value.toString()));
				break;
			case COLUMN_QUICKDIAL:
				fe.setQuickdial(fromUnicode(value.toString()));  //TODO: check if unique
				break;
			case COLUMN_VANITY:
				fe.setVanity(fromUnicode(value.toString()));     //TODO: check if unique
				break;
			default:
				return false;
			}
		} else {
			switch(index.column()) {
			case 0:
				fe.setNumber(fromUnicode(value.toString()), 0);
				break;
			case 1:
				//TODO fe.setType();
				break;
			}
		}
		fonbook->changeFonbookEntry(index.row(), fe);
		emit dataChanged(index, index); // we changed one element
		return true;
	}
	return false;
}

void KFonbookModel::setDefault(const QModelIndex &index) {
	if (index.parent().isValid() && !index.parent().parent().isValid()) {
		fonbook->setDefault(index.parent().row(), index.row());
	}
	QModelIndex indexLeft = createIndex(index.row(), 0);
	QModelIndex indexRight = createIndex(index.row(), 1);
	emit dataChanged(indexLeft, indexRight);
}

void KFonbookModel::setType(const QModelIndex &index, fritz::FonbookEntry::eType type) {
	const fritz::FonbookEntry *_fe = fonbook->retrieveFonbookEntry(index.parent().row());
	fritz::FonbookEntry fe(*_fe);
	fe.setType(type, index.row());
    fonbook->changeFonbookEntry(index.row(), fe);
	emit dataChanged(index, index);
}

bool KFonbookModel::insertRows(int row, int count __attribute__((unused)), const QModelIndex &parent) {
	beginInsertRows(parent, row, row);
	fritz::FonbookEntry fe(i18n("New Entry").toStdString());
    fonbook->addFonbookEntry(fe, row);
	endInsertRows();
	return true;
}

bool KFonbookModel::insertFonbookEntry(const QModelIndex &index, fritz::FonbookEntry &fe) {
	beginInsertRows(QModelIndex(), index.row(), index.row());
    fonbook->addFonbookEntry(fe, index.row());
	endInsertRows();
	return true;
}

const fritz::FonbookEntry *KFonbookModel::retrieveFonbookEntry(const QModelIndex &index) const {
    return fonbook->retrieveFonbookEntry(index.row());
}

bool KFonbookModel::removeRows(int row, int count __attribute__((unused)), const QModelIndex &parent) {
	beginRemoveRows(parent,row,row);
    if(fonbook->deleteFonbookEntry(row)){
		endRemoveRows();
		return true;
	} else
		return false;
}

Qt::ItemFlags KFonbookModel::flags(const QModelIndex & index) const {
	if (fonbook->isWriteable())
		return Qt::ItemFlags(QAbstractItemModel::flags(index) | QFlag(Qt::ItemIsEditable));
	else
		return QAbstractItemModel::flags(index);
}

QString KFonbookModel::getTypeName(const fritz::FonbookEntry::eType type) {
	switch (type){
	case fritz::FonbookEntry::TYPE_HOME:
		return i18n("Home");
	case fritz::FonbookEntry::TYPE_MOBILE:
		return i18n("Mobile");
	case fritz::FonbookEntry::TYPE_WORK:
		return i18n("Work");
	default:
		return "";
	}
}

void KFonbookModel::sort(int column, Qt::SortOrder order) {
	fritz::FonbookEntry::eElements element;
	switch (column) {
	case COLUMN_NAME:
		element = fritz::FonbookEntry::ELEM_NAME;
		break;
	case COLUMN_QUICKDIAL:
		element = fritz::FonbookEntry::ELEM_QUICKDIAL;
		break;
	case COLUMN_VANITY:
		element = fritz::FonbookEntry::ELEM_VANITY;
		break;
	default:
		ERR("Invalid column addressed while sorting.");
		return;
	}
    fonbook->sort(element, order == Qt::AscendingOrder);
	emit dataChanged(index(0,              0,                          QModelIndex()),
			index(rowCount(QModelIndex()), columnCount(QModelIndex()), QModelIndex()));
}

std::string KFonbookModel::number(const QModelIndex &i) const {
	if (i.parent().isValid() && !i.parent().parent().isValid()) {
		const fritz::FonbookEntry *fe = fonbook->retrieveFonbookEntry(i.parent().row());
		return fe->getNumber(i.row());
	}
	return "";
}

QModelIndex KFonbookModel::index(int row, int column, const QModelIndex &parent) const {
	// index of top level hierarchie
	if (!parent.isValid())
		return createIndex(row, column, -1);
	// index of sub level hierarchie, top level item row as identifier
	return createIndex(row, column, parent.row());
}

QModelIndex KFonbookModel::parent(const QModelIndex &child) const {
	// invalid index
	if (!child.isValid())
		return QModelIndex();
	// index of top level hierarchie
	if (child.internalId() == -1)
		return QModelIndex();
	// index of sub level hierarchie, returns corresponding top level index of first column
	return createIndex(child.internalId(), 0, -1);
}

void KFonbookModel::check() {
	if (lastRows != rowCount(QModelIndex())) {
		reset();
		emit updated();
		// stop timer, because no more changes are expected
		timer->stop();
		lastRows = rowCount(QModelIndex());
	}
}
