/* This file is part of KDevelop
  Copyright 2005 Adam Treat <treat@kde.org>
  Copyright 2013 Sebastian Kügler <sebas@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "kdevdocumentmodel.h"
#include <KFileItem>

using namespace KDevelop;

KDevDocumentItem::KDevDocumentItem( const QString &name )
    : QStandardItem(name)
    , m_documentState(IDocument::Clean)
{
    setIcon( icon() );
}

KDevDocumentItem::~KDevDocumentItem()
{}

QIcon KDevDocumentItem::icon() const
{
    switch(m_documentState)
    {
    case IDocument::Clean:
        return QIcon::fromTheme(m_fileIcon);
    case IDocument::Modified:
        return QIcon::fromTheme(QStringLiteral("document-save"));
    case IDocument::Dirty:
        return QIcon::fromTheme(QStringLiteral("document-revert"));
    case IDocument::DirtyAndModified:
        return QIcon::fromTheme(QStringLiteral("edit-delete"));
    }
    Q_UNREACHABLE();
}

IDocument::DocumentState KDevDocumentItem::documentState() const
{
    return m_documentState;
}

void KDevDocumentItem::setDocumentState(IDocument::DocumentState state)
{
    m_documentState = state;
    setIcon(icon());
}

const QUrl KDevDocumentItem::url() const
{
    return m_url;
}

void KDevDocumentItem::setUrl(const QUrl& url)
{
    m_url = url;
}

QVariant KDevDocumentItem::data(int role) const
{
    if (role == UrlRole) {
        return m_url;
    }
    return QStandardItem::data(role);
}

KDevCategoryItem::KDevCategoryItem( const QString &name )
    : KDevDocumentItem( name )
{
    setFlags(Qt::ItemIsEnabled);
    setToolTip( name );
    setIcon( QIcon::fromTheme( QStringLiteral( "folder") ) );
}

KDevCategoryItem::~KDevCategoryItem()
{}

QList<KDevFileItem*> KDevCategoryItem::fileList() const
{
    QList<KDevFileItem*> lst;

    for ( int i = 0; i < rowCount(); ++i )
    {
        if (KDevFileItem* item = static_cast<KDevDocumentItem*>(child(i))->fileItem())
            lst.append( item );
    }

    return lst;
}

KDevFileItem* KDevCategoryItem::file( const QUrl &url ) const
{
    const auto fileList = this->fileList();
    for (KDevFileItem* item : fileList) {
        if ( item->url() == url )
            return item;
    }

    return nullptr;
}

KDevFileItem::KDevFileItem( const QUrl &url )
        : KDevDocumentItem( url.fileName() )
{
    setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setUrl( url );
    if (!url.isEmpty()) {
        m_fileIcon = KFileItem(url, QString(), 0).iconName();
    }
    setIcon( QIcon::fromTheme( m_fileIcon ) );
}

KDevFileItem::~KDevFileItem()
{}

KDevDocumentModel::KDevDocumentModel( QObject *parent )
    : QStandardItemModel( parent )
{
    setRowCount(0);
    setColumnCount(1);
}

KDevDocumentModel::~KDevDocumentModel()
{}

QList<KDevCategoryItem*> KDevDocumentModel::categoryList() const
{
    QList<KDevCategoryItem*> lst;
    for ( int i = 0; i < rowCount() ; ++i )
    {
        if (KDevCategoryItem* categoryitem = static_cast<KDevDocumentItem*>(item(i))->categoryItem()) {
            lst.append( categoryitem );
        }
    }

    return lst;
}

KDevCategoryItem* KDevDocumentModel::category( const QString& category ) const
{
    const auto categoryList = this->categoryList();
    for (KDevCategoryItem* item : categoryList) {
        if ( item->toolTip() == category )
            return item;
    }

    return nullptr;
}

