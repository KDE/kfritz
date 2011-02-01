/*
 * KFritz
 *
 * Copyright (C) 2011 Joachim Wilke <kfritz@joachim-wilke.de>
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

#include "ContainerWidget.h"

ContainerWidget::ContainerWidget(QWidget *parent, QAdaptTreeView *treeview, KFonbookModel *model) :
QWidget(parent) {
	this->treeview = treeview;
	this->fonbookModel = model;
	this->calllistModel = NULL;
}

ContainerWidget::ContainerWidget(QWidget *parent, QAdaptTreeView *treeview, KCalllistModel *model) :
QWidget(parent) {
	this->treeview = treeview;
	this->fonbookModel = NULL;
	this->calllistModel = model;
}

ContainerWidget::~ContainerWidget() {
}
