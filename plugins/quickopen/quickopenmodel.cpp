/* This file is part of the KDE libraries
   Copyright (C) 2007 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "quickopenmodel.h"

#include <QTreeView>

#include <ktexteditor/codecompletionmodel.h>
#include <hashedstring.h>
#include <kdebug.h>
#include <typeinfo>

using namespace KDevelop;

QuickOpenModel::QuickOpenModel( QWidget* parent ) : ExpandingWidgetModel( parent ), m_treeView(0), m_expandingWidgetHeightIncrease(0)
{
}

void QuickOpenModel::setExpandingWidgetHeightIncrease(int pixels)
{
  m_expandingWidgetHeightIncrease = pixels;
}

QStringList QuickOpenModel::allScopes() const
{
  QStringList scopes;
  foreach( const ProviderEntry& provider, m_providers )
    foreach( const QString& scope, provider.scopes )
      if( !scopes.contains( scope ) )
        scopes << scope;

  return scopes;
}

QStringList QuickOpenModel::allTypes() const
{
  QSet<QString> types;
  foreach( const ProviderEntry& provider, m_providers )
    types += provider.types;

  return types.toList();
}

void QuickOpenModel::registerProvider( const QStringList& scopes, const QStringList& types, KDevelop::QuickOpenDataProviderBase* provider )
{
  ProviderEntry e;
  e.scopes = QSet<QString>::fromList(scopes);
  e.types = QSet<QString>::fromList(types);
  e.provider = provider;

  m_providers << e; //.insert( types, e );

  connect( provider, SIGNAL( destroyed(QObject*) ), this, SLOT( destroyed( QObject* ) ) );

  restart(true);
}

bool QuickOpenModel::removeProvider( KDevelop::QuickOpenDataProviderBase* provider )
{
  bool ret = false;
  for( ProviderList::iterator it = m_providers.begin(); it != m_providers.end(); ++it ) {
    if( (*it).provider == provider ) {
      m_providers.erase( it );
      disconnect( provider, SIGNAL( destroyed(QObject*) ), this, SLOT( destroyed( QObject* ) ) );
      ret = true;
      break;
    }
  }

  restart(true);

  return ret;
}

void QuickOpenModel::enableProviders( const QStringList& _items, const QStringList& _scopes )
{
  QSet<QString> items = QSet<QString>::fromList( _items );
  QSet<QString> scopes = QSet<QString>::fromList( _scopes );
  kDebug() << "params " << items << " " << scopes;

  //We use 2 iterations here: In the first iteration, all providers that implement QuickOpenFileSetInterface are initialized, then the other ones.
  //The reason is that the second group can refer to the first one.
  for( ProviderList::iterator it = m_providers.begin(); it != m_providers.end(); ++it ) {
    if( !dynamic_cast<QuickOpenFileSetInterface*>((*it).provider) )
      continue;
    kDebug() << "comparing" << (*it).scopes << (*it).types;
    if( ( scopes.isEmpty() || !( scopes & (*it).scopes ).isEmpty() ) && ( !( items & (*it).types ).isEmpty() || items.isEmpty() ) ) {
      kDebug() << "enabling " << (*it).types << " " << (*it).scopes;
      (*it).enabled = true;
      (*it).provider->enableData( _items, _scopes );
    } else {
      kDebug() << "disabling " << (*it).types << " " << (*it).scopes;
      (*it).enabled = false;
    }
  }

  for( ProviderList::iterator it = m_providers.begin(); it != m_providers.end(); ++it ) {
    if( dynamic_cast<QuickOpenFileSetInterface*>((*it).provider) )
      continue;
    kDebug() << "comparing" << (*it).scopes << (*it).types;
    if( ( scopes.isEmpty() || !( scopes & (*it).scopes ).isEmpty() ) && ( !( items & (*it).types ).isEmpty() || items.isEmpty() ) ) {
      kDebug() << "enabling " << (*it).types << " " << (*it).scopes;
      (*it).enabled = true;
      (*it).provider->enableData( _items, _scopes );
    } else {
      kDebug() << "disabling " << (*it).types << " " << (*it).scopes;
      (*it).enabled = false;
    }
  }

  restart(true);
}

void QuickOpenModel::textChanged( const QString& str )
{
  m_filterText = str;
  foreach( const ProviderEntry& provider, m_providers )
    if( provider.enabled )
      provider.provider->setFilterText( str );

  m_cachedData.clear();
  clearExpanding();
  reset();
}

void QuickOpenModel::restart(bool keepFilterText)
{
  if(!keepFilterText)
    m_filterText = QString();
  
  bool anyEnabled = false;

  foreach( const ProviderEntry& e, m_providers )
    anyEnabled |= e.enabled;

  if( !anyEnabled )
    return;
  
  foreach( const ProviderEntry& provider, m_providers ) {
    if( !dynamic_cast<QuickOpenFileSetInterface*>(provider.provider) )
      continue;

    ///Always reset providers that implement QuickOpenFileSetInterface, because they may be needed by other providers.
    //if( provider.enabled && provider.provider )
    provider.provider->reset();
  }
  foreach( const ProviderEntry& provider, m_providers ) {
    if( dynamic_cast<QuickOpenFileSetInterface*>(provider.provider) )
      continue;

    if( provider.enabled && provider.provider )
      provider.provider->reset();
  }

  if(keepFilterText) {
    textChanged(m_filterText);
  }else{
    m_cachedData.clear();
    clearExpanding();

    reset();
  }
}

void QuickOpenModel::destroyed( QObject* obj )
{
  removeProvider( static_cast<KDevelop::QuickOpenDataProviderBase*>(obj) );
}

QModelIndex QuickOpenModel::index( int row, int column, const QModelIndex& /*parent*/) const
{
  if( column >= columnCount() )
    return QModelIndex();
  return createIndex( row, column );
}

QModelIndex QuickOpenModel::parent( const QModelIndex& ) const
{
  return QModelIndex();
}

int QuickOpenModel::rowCount( const QModelIndex& i ) const
{
  if( i.isValid() )
    return 0;

  int count = 0;
  foreach( const ProviderEntry& provider, m_providers )
    if( provider.enabled )
      count += provider.provider->itemCount();

  return count;
}

int QuickOpenModel::columnCount() const
{
  return 2;
}

int QuickOpenModel::columnCount( const QModelIndex& index ) const
{
  if( index.parent().isValid() )
    return 0;
  else {
    return columnCount();
  }
}

QVariant QuickOpenModel::data( const QModelIndex& index, int role ) const
{
  QuickOpenDataPointer d = getItem( index.row() );

  if( !d )
    return QVariant();

  switch( role ) {
    case KTextEditor::CodeCompletionModel::ItemSelected:
      return d->htmlDescription();

    case KTextEditor::CodeCompletionModel::IsExpandable:
      return d->isExpandable();

    case KTextEditor::CodeCompletionModel::ExpandingWidget: {
      QVariant v;
      QWidget* w =  d->expandingWidget();
      if(w && m_expandingWidgetHeightIncrease)
        w->resize(w->width(), w->height() + m_expandingWidgetHeightIncrease);
      
      v.setValue<QWidget*>(w);
      return v;
    }
  }

  if( index.column() == 1 )
  {
    switch( role ) {
      case Qt::DecorationRole:
        return d->icon();

      case Qt::DisplayRole:
        return d->text();
    }
  } else if( index.column() == 0 )
  {
    switch( role ) {
      case Qt::DecorationRole:
      {
        if( isExpandable(index) ) {
          //Show the expanded/unexpanded handles
          cacheIcons();
          if( isExpanded(index) ) {
            return m_expandedIcon;
          } else {
            return m_collapsedIcon;
          }
        }
      }
    }
  }

  return ExpandingWidgetModel::data( index, role );
}

QuickOpenDataPointer QuickOpenModel::getItem( int row ) const {
  ///@todo mix all the models alphabetically here. For now, they are simply ordered.

  if( m_cachedData.contains( row ) )
    return m_cachedData[row];

  foreach( const ProviderEntry& provider, m_providers ) {
    if( !provider.enabled )
      continue;
    if( (uint)row < provider.provider->itemCount() )
    {
      QList<QuickOpenDataPointer> items = provider.provider->data( row, row+1 );

      if( items.isEmpty() )
      {
        kWarning() << "Provider returned no item";
        return QuickOpenDataPointer();
      } else {
        m_cachedData[row] = items.first();
        return items.first();
      }
    } else {
      row -= provider.provider->itemCount();
    }
  }

  kWarning() << "No item for row " <<  row;

  return QuickOpenDataPointer();
}

QSet<HashedString> QuickOpenModel::fileSet() const {
  QSet<HashedString> merged;
  foreach( const ProviderEntry& provider, m_providers ) {
    if( QuickOpenFileSetInterface* iface = dynamic_cast<QuickOpenFileSetInterface*>(provider.provider) ) {
      QSet<HashedString> ifiles = iface->files();
      //kDebug() << "got file-list with" << ifiles.count() << "entries from data-provider" << typeid(*iface).name();
      merged += ifiles;
    }
  }
  return merged;
}


QTreeView* QuickOpenModel::treeView() const {
  return m_treeView;
}

bool QuickOpenModel::indexIsItem(const QModelIndex& /*index*/) const {
  return true;
}

void QuickOpenModel::setTreeView( QTreeView* view ) {
  m_treeView = view;
}

int QuickOpenModel::contextMatchQuality(const QModelIndex & /*index*/) const {
  return -1;
}

bool QuickOpenModel::execute( const QModelIndex& index, QString& filterText )
{
  kDebug() << "executing model";
  if( !index.isValid() ) {
    kWarning() << "Invalid index executed";
    return false;
  }

  QuickOpenDataPointer item = getItem( index.row() );

  if( item ) {
    return item->execute( filterText );
  }else{
    kWarning() << "Got no item for row " << index.row() << " ";
  }

  return false;
}
