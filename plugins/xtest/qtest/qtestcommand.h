/* KDevelop xUnit plugin
 *
 * Copyright 2008 Manuel Breugelmans <mbr.nxi@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef QXQTEST_QTESTCOMMAND_H
#define QXQTEST_QTESTCOMMAND_H

#include <QString>
#include <QFileInfo>
#include <veritas/test.h>

namespace QTest
{
class QTestCase;

/*! Leaf item in the test tree. It represents a single test function, a
private slot in the QTestCase. */
class QTestCommand : public Veritas::Test
{
Q_OBJECT
public:
    QTestCommand(const QString&, QTestCase*);
    virtual ~QTestCommand();
    QString command(); // dead code for now, but might want it again to allow
                       // for execution of individual commands (vs full case)
};

} // end namespace QTest

#endif // QXQTEST_QTESTCOMMAND_H
