/***************************************************************************
 *   Copyright (C) 1999-2001 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "appoutputwidget.h"

#include <klocale.h>

#include "appoutputviewpart.h"


AppOutputWidget::AppOutputWidget()
    : ProcessWidget(0, "app output widget")
{}


AppOutputWidget::~AppOutputWidget()
{}


void AppOutputWidget::childFinished(bool normal, int status)
{
    ProcessWidget::childFinished(normal, status);
}

#include "appoutputwidget.moc"
