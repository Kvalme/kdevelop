/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright (C) 2007 Andreas Pakulat <apaku@gmx.de>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "makeoutputmodel.h"
#include "outputfilters.h"
#include <kdebug.h>

MakeOutputModel::MakeOutputModel( QObject* parent )
    : QStandardItemModel(parent), actionFilter(new MakeActionFilter), errorFilter(new ErrorFilter)
{
}

void MakeOutputModel::addStandardError( const QStringList& lines )
{
    foreach( QString line, lines)
    {
        QStandardItem* item = errorFilter->processAndCreate(line);
        if( !item )
            item = new QStandardItem(line);
        appendRow(item);
    }
}

void MakeOutputModel::addStandardOutput( const QStringList& lines )
{
    foreach( QString line, lines)
    {
        QStandardItem* item = actionFilter->processAndCreate(line);
        if( !item )
            item = new QStandardItem(line);
        appendRow(item);
    }
}

//kate: space-indent on; indent-width 4; replace-tabs on; auto-insert-doxygen on; indent-mode cstyle;

#include "makeoutputmodel.moc"
