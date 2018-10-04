/*
 * Copyright 2014  Kevin Funk <kfunk@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DUCHAINUTILS_H
#define DUCHAINUTILS_H

#include "clangprivateexport.h"

#include "duchain/parsesession.h"

namespace KTextEditor {
class Range;
}

namespace KDevelop {
class Declaration;
}

namespace ClangIntegration {
namespace DUChainUtils
{

KDEVCLANGPRIVATE_EXPORT KTextEditor::Range functionSignatureRange(const KDevelop::Declaration* decl);

KDEVCLANGPRIVATE_EXPORT void registerDUChainItems();
KDEVCLANGPRIVATE_EXPORT void unregisterDUChainItems();

/**
 * Finds attached parse session data (aka AST) to the @p file
 *
 * If no session data found, then @p tuFile asked for the attached session data
 */
KDEVCLANGPRIVATE_EXPORT ParseSessionData::Ptr findParseSessionData(const KDevelop::IndexedString &file, const KDevelop::IndexedString &tufile);
}

}

#endif // DUCHAINUTILS_H
