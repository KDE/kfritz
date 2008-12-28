/*
 * KCalllistModel.cpp
 *
 *  Created on: Dec 22, 2008
 *      Author: joachim
 */

#include <KIcon>
#include "Tools.h"
#include "KCalllistModel.h"

KCalllistModel::KCalllistModel() {
	// get the calllist resource
	calllist = fritz::CallList::getCallList();
	inputCodec  = QTextCodec::codecForName(fritz::CharSetConv::SystemCharacterTable() ? fritz::CharSetConv::SystemCharacterTable() : "UTF-8");


}

KCalllistModel::~KCalllistModel() {
	// TODO: somthing to do here?
}

QModelIndex KCalllistModel::index(int row, int column, const QModelIndex & parent) const
{
	if (parent.isValid())
		return QModelIndex();
	else
		return createIndex(row, column);
}

QVariant KCalllistModel::data(const QModelIndex & index, int role) const
{
	if (role == Qt::DecorationRole && index.column() == 0)
		return QVariant(KIcon("document-new"));
	if (role != Qt::DisplayRole)
		return QVariant();

	fritz::CallEntry *ce = calllist->RetrieveEntry(fritz::CallEntry::ALL,index.row());
	switch (index.column()) {
	case 0:
		switch (ce->type){
		case fritz::CallEntry::INCOMING:
			return QVariant("<--");
			break;
		case fritz::CallEntry::MISSED:
			return QVariant("?<--");
			break;
		case fritz::CallEntry::OUTGOING:
			return QVariant("-->");
			break;
		case fritz::CallEntry::ALL:
			// this type is not part of the call list (it's just a "meta" type)
			break;
		}
	case 1:
		return QVariant(inputCodec->toUnicode((ce->date+" "+ce->time).c_str()));
		break;
	case 2:
		if (ce->remoteName.size() == 0)
			if (ce->remoteNumber.size() == 0)
				return QVariant("unknown");
			else
				return QVariant(inputCodec->toUnicode(ce->remoteNumber.c_str()));
		else
			return QVariant(inputCodec->toUnicode(ce->remoteName.c_str()));
		break;
	case 3:
		return QVariant(inputCodec->toUnicode(ce->localName.c_str()));
		break;
	case 4:
		return QVariant(inputCodec->toUnicode(ce->duration.c_str()));
		break;
	default:
		return QVariant();
	}
	return QVariant();
}

QVariant KCalllistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();
	if (orientation == Qt::Horizontal){
		switch (section) {
		case 0:
			return "Type";
			break;
		case 1:
			return "Date";
			break;
		case 2:
			return "Name / Number";
			break;
		case 3:
			return "Local Device";
			break;
		case 4:
			return "Duration";
			break;
		default:
			return QVariant();
		}
	}else {
		return QVariant();
	}
}

int KCalllistModel::columnCount(const QModelIndex & parent) const
{
	return 5;
}

int KCalllistModel::rowCount(const QModelIndex & parent) const
{
	if (parent.isValid())
		// the model does not have any hierarchy
		return 0;
	else
		return calllist->GetSize(fritz::CallEntry::ALL);
}

QModelIndex KCalllistModel::parent(const QModelIndex & child) const
{
	return QModelIndex();
}


