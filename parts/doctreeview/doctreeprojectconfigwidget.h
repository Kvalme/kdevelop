/***************************************************************************
 *   Copyright (C) 2002 by Sebastian Kratzert                              *
 *   skratzert@gmx.de                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _DOCTREEPROJECTCONFIGWIDGET_H_
#define _DOCTREEPROJECTCONFIGWIDGET_H_

#include "doctreeprojectconfigwidgetbase.h"

class DocTreeViewWidget;
class KDevProject;

class DocTreeProjectConfigWidget : public DocTreeProjectConfigWidgetBase
{
    Q_OBJECT

public:
    DocTreeProjectConfigWidget( DocTreeViewWidget *widget, QWidget *parent, KDevProject *project, const char *name=0 );
    //~DocTreeProjectConfigWidget();

public slots:
    void accept();
   // void setProject(KDevProject* project);

private:
    void readConfig();
    void storeConfig();

    /** The documentation items to ignore */
    QStringList m_ignoreQT_XML;
    QStringList m_ignoreDoxygen;
    QStringList m_ignoreKDoc;
    QStringList m_ignoreToc;
    QStringList m_ignoreDevHelp;

    QListViewItem *m_qtDocs;
    QListViewItem *m_doxygenDocs;
    QListViewItem *m_kdocDocs;
    QListViewItem *m_tocDocs;
    QListViewItem *m_devHelpDocs;

    DocTreeViewWidget *m_widget;
    KDevProject *m_project;

friend class DocCheckItem;
};

#endif
