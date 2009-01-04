/*
 * KFonbookModel.h
 *
 *  Created on: Dec 21, 2008
 *      Author: joachim
 */

#ifndef KFONBOOKMODEL_H_
#define KFONBOOKMODEL_H_

#include <QAbstractItemModel>
#include <QTextCodec>
#include <FonbookManager.h>

class KFonbookModel : public QAbstractItemModel  {
public:
	KFonbookModel();
	virtual ~KFonbookModel();
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column,
    		                  const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;


private:
	fritz::Fonbook *fonbook;
	QTextCodec *inputCodec;
};

#endif /* KFONBOOKMODEL_H_ */