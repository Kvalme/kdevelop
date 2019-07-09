/*  This file is part of KDevelop

    Copyright 2010 Yannick Motta <yannick.motta@gmail.com>
    Copyright 2010 Benjamin Port <port.benjamin@gmail.com>
    Copyright 2014 Milian Wolff <mail@milianw.de>

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

#include "manpagemodel.h"
#include "manpageplugin.h"
#include "manpagedocumentation.h"

#include <interfaces/icore.h>
#include <interfaces/idocumentationcontroller.h>

#include <QStringListModel>
#include <limits>

namespace {

const quintptr INVALID_ID = std::numeric_limits<quintptr>::max();

}

using namespace KDevelop;

ManPageModel::ManPageModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_indexModel(new QStringListModel(this))
{
    QMetaObject::invokeMethod(const_cast<ManPageModel*>(this), "initModel", Qt::QueuedConnection);
}

ManPageModel::~ManPageModel()
{
}

QModelIndex ManPageModel::parent(const QModelIndex& child) const
{
    if (child.isValid() && child.column() == 0 && child.internalId() != INVALID_ID) {
        return createIndex(child.internalId(), 0, INVALID_ID);
    }
    return QModelIndex();
}

QModelIndex ManPageModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || column != 0) {
        return QModelIndex();
    } else if (!parent.isValid() && row == m_sectionList.count()) {
        return QModelIndex();
    }

    return createIndex(row, column, parent.isValid() ? parent.row() : INVALID_ID);
}

QVariant ManPageModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid()) {
        if (role == Qt::DisplayRole) {
            int internal(index.internalId());
            if (internal >= 0) {
                int position = index.row();
                QString sectionUrl = m_sectionList.at(index.internalId()).first;
                return manPage(sectionUrl, position);
            } else {
                return m_sectionList.at(index.row()).second;
            }
        }
    }
    return QVariant();
}

int ManPageModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return m_sectionList.count();
    } else if (parent.internalId() == INVALID_ID) {
        const QString sectionUrl = m_sectionList.at(parent.row()).first;
        return m_manMap.value(sectionUrl).count();
    }
    return 0;
}

QString ManPageModel::manPage(const QString& sectionUrl, int position) const
{
    return m_manMap.value(sectionUrl).at(position);
}

void ManPageModel::initModel()
{
    m_sectionList.clear();
    m_manMap.clear();
    auto list = KIO::listDir(QUrl(QStringLiteral("man://")), KIO::HideProgressInfo);
    connect(list, &KIO::ListJob::entries, this, &ManPageModel::indexEntries);
    connect(list, &KIO::ListJob::result, this, &ManPageModel::indexLoaded);
}

void ManPageModel::indexEntries(KIO::Job* /*job*/, const KIO::UDSEntryList& entries)
{
    for (const KIO::UDSEntry& entry : entries) {
        if (entry.isDir()) {
            m_sectionList << qMakePair(entry.stringValue(KIO::UDSEntry::UDS_URL), entry.stringValue(KIO::UDSEntry::UDS_NAME));
        }
    }
}

void ManPageModel::indexLoaded(KJob* job)
{
    if (job->error() != 0) {
        m_errorString = job->errorString();
        emit error(m_errorString);
        return;
    }

    emit sectionListUpdated();

    iterator = new QListIterator<ManSection>(m_sectionList);
    if (iterator->hasNext()) {
        initSection();
    }
}

void ManPageModel::initSection()
{
    const QString sectionUrl = iterator->peekNext().first;
    m_manMap[sectionUrl].clear();
    auto list = KIO::listDir(QUrl(sectionUrl), KIO::HideProgressInfo);
    connect(list, &KIO::ListJob::entries, this, &ManPageModel::sectionEntries);
    connect(list, &KIO::ListJob::result, this, &ManPageModel::sectionLoaded);
}

void ManPageModel::sectionEntries(KIO::Job* /*job*/, const KIO::UDSEntryList& entries)
{
    const QString sectionUrl = iterator->peekNext().first;
    auto& pages = m_manMap[sectionUrl];
    pages.reserve(pages.size() + entries.size());
    for (const KIO::UDSEntry& entry : entries) {
        pages << entry.stringValue(KIO::UDSEntry::UDS_NAME);
    }
}

void ManPageModel::sectionLoaded()
{
    iterator->next();
    m_nbSectionLoaded++;
    emit sectionParsed();
    if (iterator->hasNext()) {
        initSection();
    } else {
        // End of init
        m_loaded = true;
        m_index.clear();
        for (const auto& entries : qAsConst(m_manMap)) {
            m_index += entries.toList();
        }
        m_index.sort();
        m_index.removeDuplicates();
        m_indexModel->setStringList(m_index);
        delete iterator;
        emit manPagesLoaded();
    }
}

void ManPageModel::showItem(const QModelIndex& idx)
{
    if (idx.isValid() && idx.internalId() != INVALID_ID) {
        QString sectionUrl = m_sectionList.at(idx.internalId()).first;
        QString page = manPage(sectionUrl, idx.row());
        IDocumentation::Ptr newDoc(new ManPageDocumentation(page, QUrl(sectionUrl + QLatin1Char('/') + page)));
        ICore::self()->documentationController()->showDocumentation(newDoc);
    }
}

void ManPageModel::showItemFromUrl(const QUrl& url)
{
    if (url.toString().startsWith(QLatin1String("man"))) {
        IDocumentation::Ptr newDoc(new ManPageDocumentation(url.path(), QUrl(url)));
        ICore::self()->documentationController()->showDocumentation(newDoc);
    }
}

QStringListModel* ManPageModel::indexList()
{
    return m_indexModel;
}

bool ManPageModel::containsIdentifier(const QString& identifier)
{
    return m_index.contains(identifier);
}

int ManPageModel::sectionCount() const
{
    return m_sectionList.count();
}

bool ManPageModel::isLoaded() const
{
    return m_loaded;
}

int ManPageModel::nbSectionLoaded() const
{
    return m_nbSectionLoaded;
}

bool ManPageModel::identifierInSection(const QString& identifier, const QString& section) const
{
    const QString sectionLink = QLatin1String("man:/(") + section + QLatin1Char(')');
    for (auto it = m_manMap.begin(); it != m_manMap.end(); ++it) {
        if (it.key().startsWith(sectionLink)) {
            return it.value().indexOf(identifier) != -1;
        }
    }
    return false;
}

bool ManPageModel::hasError() const
{
    return !m_errorString.isEmpty();
}

const QString& ManPageModel::errorString() const
{
    return m_errorString;
}
