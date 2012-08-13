/*
 * This file is part of KDevelop
 * Copyright 2012 Miha Čančula <miha@noughmad.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef CREATETESTASSISTANT_H
#define CREATETESTASSISTANT_H

#include "kassistantdialog.h"

class CreateTestAssistantPrivate;

class CreateTestAssistant : public KAssistantDialog
{
    Q_OBJECT

public:
    explicit CreateTestAssistant(const KUrl& baseUrl, QWidget* parent = 0, Qt::WFlags flags = 0);
    virtual ~CreateTestAssistant();

    virtual void accept();
    virtual void next();

public slots:
    void templateValid(bool valid);

private:
    class CreateTestAssistantPrivate* const d;

};

#endif // CREATETESTASSISTANT_H