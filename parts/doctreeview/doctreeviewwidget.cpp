/***************************************************************************
 *   Copyright (C) 1999-2002 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
 *   Copyright (C) 2002 by Sebastian Kratzert                              *
 *   skratzert@gmx.de                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "doctreeviewwidget.h"

//#include <stdio.h>

//#include <qapplication.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qhbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qregexp.h>
//#include <qsizepolicy.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qvbox.h>

#include <kaction.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kcombobox.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
#include <ksimpleconfig.h>

#include "kdevcore.h"
#include "domutil.h"
#include "kdevtoplevel.h"
#include "kdevproject.h"
#include "kdevpartcontroller.h"

#include "../../config.h"
#include "misc.h"
#include "doctreeviewfactory.h"
#include "doctreeviewpart.h"
#include "doctreeglobalconfigwidget.h"


class DocTreeItem : public QListViewItem
{
public:
    enum Type { Folder, Book, Doc };
    DocTreeItem( KListView *parent, Type type, const QString &text, const QString &context );
    DocTreeItem( DocTreeItem *parent, Type type, const QString &text, const QString &context);

    void setFileName(const QString &fn)
        { filename = fn; }
    virtual QString fileName()
        { return filename; }
    virtual void clear();
    virtual QString context() const { return m_context; }
	virtual Type getType() const { return typ; }
    
private:
    void init();
    Type typ;
    QString filename, m_context;
};


DocTreeItem::DocTreeItem(KListView *parent, Type type, const QString &text, const QString &context)
    : QListViewItem(parent, text), typ(type), m_context(context)
{
    init();
}


DocTreeItem::DocTreeItem(DocTreeItem *parent, Type type, const QString &text, const QString &context)
    : QListViewItem(parent, text), typ(type), m_context(context)
{
    init();
}


void DocTreeItem::init()
{
    QString icon;
    if (typ == Folder)
        icon = "folder";
    else if (typ == Book)
        icon = "contents";
    else
        icon = "document";
    setPixmap(0, SmallIcon(icon));
}


void DocTreeItem::clear()
{
    QListViewItem *child = firstChild();
    while (child) {
        QListViewItem *old = child;
        child = child->nextSibling();
        delete old;
    }
}


/***************************************/
/* Folder "Qt/KDE libraries (Doxygen)" */
/***************************************/


/**
 * API documentation created with the configure option --with-apidocs
 * in kdelibs.
 */
class DocTreeDoxygenBook : public DocTreeItem
{
public:
    DocTreeDoxygenBook( DocTreeItem *parent, const QString &name,
                        const QString &tagFileName, const QString &context);
    ~DocTreeDoxygenBook();
    static bool isInstallationOK(const QString& bookDir)
    {
      return QFile::exists(bookDir + "/html/index.html");
    }

    virtual void setOpen(bool o);
    
private:
    void readTagFile();
    QString dirname;
};


DocTreeDoxygenBook::DocTreeDoxygenBook(DocTreeItem *parent, const QString &name,
                                       const QString &dirName, const QString &context)
    : DocTreeItem(parent, Book, name, context),
      dirname(dirName)
{
    QString fileName = dirName + "/index.html";
    setFileName(fileName);
    setExpandable(true);
}


DocTreeDoxygenBook::~DocTreeDoxygenBook()
{}


void DocTreeDoxygenBook::setOpen(bool o)
{
    if (o && childCount() == 0)
        readTagFile();
    DocTreeItem::setOpen(o);
}


void DocTreeDoxygenBook::readTagFile()
{
    QString tagName = dirname + "/" + text(0) + ".tag";
    QFile f(tagName);
    if(!f.exists())
    {
        tagName.remove("/html/");
        f.setName( tagName );
    }
    if (!f.open(IO_ReadOnly)) {
        kdDebug(9002) << "Could not open tag file: " << f.name() << endl;
        return;
    }
    
    QDomDocument dom;
    if (!dom.setContent(&f) || dom.documentElement().nodeName() != "tagfile") {
        kdDebug(9002) << "No valid tag file" << endl;
        return;
    }
    f.close();

    QDomElement docEl = dom.documentElement();

    QDomElement childEl = docEl.firstChild().toElement();
    while (!childEl.isNull()) {
        if (childEl.tagName() == "compound" && childEl.attribute("kind") == "class") {
            QString classname = childEl.namedItem("name").firstChild().toText().data();
            QString filename = childEl.namedItem("filename").firstChild().toText().data();

            if (QFile::exists(dirname + filename)) { // don't create bad links
                DocTreeItem *item = new DocTreeItem(this, Doc, classname, context());
                item->setFileName(dirname + filename);
            }
        }
        childEl = childEl.nextSibling().toElement();
    }

    sortChildItems(0, true);
}


class DocTreeDoxygenFolder : public DocTreeItem
{
public:
    DocTreeDoxygenFolder(KListView *parent, const QString &context)
        : DocTreeItem(parent, Folder, i18n("KDE Libraries (Doxygen)"), context)
        { setExpandable(true); }
    void refresh();
};

void DocTreeDoxygenFolder::refresh()
{
    DocTreeItem::clear();

    KConfig *config = DocTreeViewFactory::instance()->config();
    config->setGroup("General");
    QString docdir = config->readEntry("kdelibsdocdir", KDELIBS_DOXYDIR);
    //kdDebug(9002) << "docdir: " << docdir << endl;
    QDir d(docdir);
    QStringList fileList = d.entryList("*", QDir::Dirs);

    QStringList::ConstIterator it;
    for (it = fileList.begin(); it != fileList.end(); ++it) {
        QString dirName = (*it);
        //kdDebug(9002) << "dirname: " << dirName << endl;
        if (dirName == "." || dirName == ".." || dirName == "common")
            continue;
        if (DocTreeDoxygenBook::isInstallationOK(d.absFilePath(*it))) {
            new DocTreeDoxygenBook(this, *it, d.absFilePath(*it) + "/html/", context());
            //kdDebug(9002) << "foo: " << d.absFilePath(*it) + "/html/" + *it << endl;
        }
    }

    sortChildItems(0, true);
}
    

/***************************************/
/* Folder from the 'tocs' resource dir */
/***************************************/

class DocTreeTocFolder : public DocTreeItem
{
public:
    DocTreeTocFolder(KListView *parent, const QString &fileName, const QString &context);
    ~DocTreeTocFolder();

    QString tocName() const
    { return toc_name; }

private:
    QString toc_name;
};


DocTreeTocFolder::DocTreeTocFolder(KListView *parent, const QString &fileName, const QString &context)
    : DocTreeItem(parent, Folder, fileName, context)
{
    QFileInfo fi(fileName);
    toc_name = fi.baseName();

    QFile f(fileName);
    if (!f.open(IO_ReadOnly)) {
        kdDebug(9002) << "Could not read doc toc: " << fileName << endl;
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(&f) || doc.doctype().name() != "kdeveloptoc") {
        kdDebug() << "Not a valid kdeveloptoc file: " << fileName << endl;
        return;
    }
    
    f.close();
    
    QDomElement docEl = doc.documentElement();
    QDomElement titleEl = docEl.namedItem("title").toElement();
    setText(0, titleEl.firstChild().toText().data());

    QString base;
    QListViewItem *lastChildItem = 0;
    QDomElement childEl = docEl.firstChild().toElement();
    while (!childEl.isNull()) {
        if (childEl.tagName() == "tocsect1") {
            QString name = childEl.attribute("name");
            QString url = childEl.attribute("url");
            DocTreeItem *item = new DocTreeItem(this, Book, name, DocTreeItem::context());
            if (!url.isEmpty())
                item->setFileName(base + url);
            
            if (lastChildItem)
                item->moveItem(lastChildItem);
            lastChildItem = item;
            
            // Ok, this means we have two levels in the table of contents hardcoded
            // Eventually, this limitation should go, but at the moment it is simple to implement :-)
            QListViewItem *lastGrandchildItem = 0;
            QDomElement grandchildEl = childEl.firstChild().toElement();
            while (!grandchildEl.isNull()) {
                if (grandchildEl.tagName() == "tocsect2") {
                    QString name2 = grandchildEl.attribute("name");
                    QString url2 = grandchildEl.attribute("url");
                    DocTreeItem *item2 = new DocTreeItem(item, Doc, name2, DocTreeItem::context());
                    if (!url2.isEmpty())
                        item2->setFileName(base + url2);
                    if (lastGrandchildItem)
                        item2->moveItem(lastGrandchildItem);
                    lastGrandchildItem = item2;
		    // and nobody said there couldn't be another level ;-)
		    QListViewItem *last2GrandchildItem = 0;
		    QDomElement grand2childEl = grandchildEl.firstChild().toElement();
		    while (!grand2childEl.isNull()) {
			if (grand2childEl.tagName() == "tocsect3") {
			    QString name3 = grand2childEl.attribute("name");
			    QString url3 = grand2childEl.attribute("url");
			    DocTreeItem *item3 = new DocTreeItem(item2, Doc, name3, DocTreeItem::context());
                            if (!url3.isEmpty())
                                item3->setFileName(base + url3);
			    if (last2GrandchildItem)
				item3->moveItem(last2GrandchildItem);
			    last2GrandchildItem = item3;
			}
			grand2childEl = grand2childEl.nextSibling().toElement();
		    }
                }
                grandchildEl = grandchildEl.nextSibling().toElement();
            }
        } else if (childEl.tagName() == "base") {
            base = childEl.attribute("href");
            if (!base.isEmpty())
                base += "/";
        }

        childEl = childEl.nextSibling().toElement();
    }
}


DocTreeTocFolder::~DocTreeTocFolder()
{}


/*************************************/
/* Folder "Documentation Base"       */
/*************************************/

#ifdef WITH_DOCBASE


class DocTreeDocbaseFolder : public DocTreeItem
{
public:
    DocTreeDocbaseFolder(KListView *parent, const QString &context);
    ~DocTreeDocbaseFolder();
    virtual void setOpen(bool o);
private:
    void readDocbaseFile(FILE *f);
};


DocTreeDocbaseFolder::DocTreeDocbaseFolder(KListView *parent, const QString &context)
    : DocTreeItem(parent, Folder, i18n("Documentation Base"), context)
{
    setExpandable(true);
}


DocTreeDocbaseFolder::~DocTreeDocbaseFolder()
{}


void DocTreeDocbaseFolder::readDocbaseFile(FILE *f)
{
    char buf[1024];
    QString title;
    bool html = false;
    while (fgets(buf, sizeof buf, f)) {
        QString s = buf;
        if (s.right(1) == "\n")
            s.truncate(s.length()-1); // chop
        
        if (s.left(7) == "Title: ")
            title = s.mid(7, s.length()-7);
        else if (s.left(8) == "Format: ")
            html = s.find("HTML", 8, false) != -1;
        else if (s.left(7) == "Index: "
                 && html && !title.isEmpty()) {
            QString filename = s.mid(7, s.length()-7);
            DocTreeItem *item = new DocTreeItem(this, Doc, title, context());
            item->setFileName(filename);
            break;
        }
        else if (s.left(9) == "Section: "
                 && s.find("programming", 9, false) == -1)
            break;
    }
}


void DocTreeDocbaseFolder::setOpen(bool o)
{
    if (o && childCount() == 0) {
        QDir d("/usr/share/doc-base");
        QStringList fileList = d.entryList("*", QDir::Files);
        QStringList::Iterator it;
        for (it = fileList.begin(); it != fileList.end(); ++it) {
            FILE *f;
            if ( (f = fopen(d.filePath(*it), "r")) != 0) {
                readDocbaseFile(f);
                fclose(f);
            }
        }
    }
    DocTreeItem::setOpen(o);
}


#endif


/*************************************/
/* Folder "Bookmarks"                */
/*************************************/

class DocTreeBookmarksFolder : public DocTreeItem
{
public:
    DocTreeBookmarksFolder(KListView *parent, const QString &context);
    void refresh();
};

DocTreeBookmarksFolder::DocTreeBookmarksFolder(KListView *parent, const QString &context)
    : DocTreeItem(parent, Folder, i18n("Bookmarks"), context)
{}

void DocTreeBookmarksFolder::refresh()
{
    DocTreeItem::clear();

    QStringList othersTitle, othersURL;
    DocTreeViewTool::getBookmarks(&othersTitle, &othersURL);
    QStringList::Iterator it1, it2;
    for (it1 = othersTitle.begin(), it2 = othersURL.begin();
         it1 != othersTitle.end() && it2 != othersURL.end();
         ++it1, ++it2) {
        DocTreeItem *item = new DocTreeItem(this, Book, *it1, context());
        item->setFileName(*it2);
    }
}


/*************************************/
/* Folder "Current Project"          */
/*************************************/

class DocTreeProjectFolder : public DocTreeItem
{
public:
    DocTreeProjectFolder(KListView *parent, const QString &context);
    void setProject(KDevProject *project)
        { m_project = project; }
    void refresh();

private:
    KDevProject *m_project;
    QString m_userdocDir, m_apidocDir;
};

DocTreeProjectFolder::DocTreeProjectFolder(KListView *parent, const QString &context)
    : DocTreeItem(parent, Folder, i18n("Current Project"), context), m_project(0)
{
}


void DocTreeProjectFolder::refresh()
{
    //TODO: use doxygen tags
    if( !m_project )
        return;
        
    m_userdocDir = DomUtil::readEntry(
        *m_project->projectDom() , "/kdevdoctreeview/projectdoc/userdocDir");
    m_apidocDir = DomUtil::readEntry(
        *m_project->projectDom() , "/kdevdoctreeview/projectdoc/apidocDir");
    
    
    DocTreeItem::clear();

        // API documentation
        QDir apidir( m_apidocDir );
        if (apidir.exists()) {
            QStringList entries = apidir.entryList("*.html", QDir::Files);
            QString filename = apidir.absPath() + "/index.html";
            if (!QFileInfo(filename).exists())
                return;
            DocTreeItem *item = new DocTreeItem(
                this, Book, i18n("API of %1").arg(m_project->projectName() ), context());
                item->setFileName(filename);
            for (QStringList::Iterator it = entries.begin(); it != entries.end(); ++it) {
                filename = *it;
                DocTreeItem *ditem = new DocTreeItem(item, 
                    Doc, QFileInfo(filename).baseName() , context());
                ditem->setFileName(apidir.absPath() +"/"+ filename);
            }
        }
        // User documentation
        QDir userdir( m_userdocDir );
        if (userdir.exists()) {
            QStringList entries = userdir.entryList("*.html", QDir::Files);
            QString filename = userdir.absPath() + "/index.html";
            if (!QFileInfo(filename).exists())
                return;
            DocTreeItem *item = new DocTreeItem(
                this, Book, i18n("Usedoc for %1").arg(m_project->projectName() ), context());
                item->setFileName(filename);
            for (QStringList::Iterator it = entries.begin(); it != entries.end(); ++it) {
                filename = *it;
                DocTreeItem *ditem = new DocTreeItem(item, 
                    Doc, QFileInfo(filename).baseName() , context());
                ditem->setFileName(userdir.absPath() +"/"+ filename);
            }
        }

    if (!firstChild())
        setExpandable(false);

}


/**************************************/
/* Qt Folder                          */
/**************************************/

class DocTreeQtFolder : public DocTreeItem
{
public:
    DocTreeQtFolder(KListView *parent, const QString &_fileName, const QString &context);
    void refresh();
private:    
    QString filename;
};

DocTreeQtFolder::DocTreeQtFolder(KListView *parent, const QString &_fileName,
    const QString &context)
    : DocTreeItem(parent, Folder, i18n("Qt"), context)
{
    filename = _fileName;
}

void DocTreeQtFolder::refresh() 
{
    QFileInfo fi(filename);

    QFile f(filename);
    if (!f.open(IO_ReadOnly)) {
        kdDebug(9002) << "Could not read qt.xml" << endl;
        return;
    }
    QDomDocument doc;
    if (!doc.setContent(&f) || doc.doctype().name() != "DCF") {
        kdDebug(9002) << "Not a valid DCF file: " << filename << endl;
        return;
    }
    
    f.close();
    
    QDomElement docEl = doc.documentElement();
    QDomElement titleEl = docEl.namedItem("DCF").toElement();

    QDomElement childEl = docEl.lastChild().toElement();
    while (!childEl.isNull()) 
    {
        if (childEl.tagName() == "section") 
        {
            QString ref = childEl.attribute("ref");
            QString title = childEl.attribute("title");
            
            int i = title.find("Class Reference");
            if( i > 0 ) 
            {
                title = title.left(i);
                DocTreeItem* item = item = new DocTreeItem(this, Book, title, context());
                item->setFileName(fi.dirPath( true ) +"/"+ ref);

                QDomElement grandChild = childEl.lastChild().toElement();
                while(!grandChild.isNull()) 
                {
                    if (grandChild.tagName() == "keyword") 
                    {
                        QString dref = grandChild.attribute("ref");
                        QString dtitle = grandChild.text();
            
                        DocTreeItem* dItem = new DocTreeItem(item, Doc, dtitle, context());
                        dItem->setFileName(fi.dirPath( true ) +"/"+ dref);
                    }
                    grandChild = grandChild.previousSibling().toElement();
               }
               //kdDebug(9002) <<"ref: "<< ref <<"  title: " << title << endl;
            }
            childEl = childEl.previousSibling().toElement();            
        }
    }
}


/**************************************/
/* The DocTreeViewWidget itself       */
/**************************************/

DocTreeViewWidget::DocTreeViewWidget(DocTreeViewPart *part)
    : QVBox(0, "doc tree widget"), m_activeTreeItem ( 0L )
{
    /* initializing the documentation toolbar */
	KActionCollection* actions = new KActionCollection(this);
	
	docToolbar = new QHBox ( this, "documentation toolbar" );
	docToolbar->setMargin ( 2 );
	docToolbar->setSpacing ( 2 );
	
	hLine = new QLabel ( this, "horizontal line" );
	hLine->setFrameShape ( QLabel::HLine );
	hLine->setFrameShadow( QLabel::Sunken );
	hLine->setMaximumHeight ( 5 );
	hLine->hide();

	searchToolbar = new QHBox ( this, "search toolbar" );
	searchToolbar->setMargin ( 2 );
	searchToolbar->setSpacing ( 2 );
	searchToolbar->hide();

	docConfigButton = new QToolButton ( docToolbar, "configure button" );
	docConfigButton->setPixmap ( SmallIcon ( "configure" ) );
	docConfigButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, docConfigButton->sizePolicy().hasHeightForWidth() ) );
	docConfigButton->setEnabled ( false );
	QToolTip::add ( docConfigButton, i18n ( "Customize the selected documentation tree..." ) );

	QWidget *spacer = new QWidget(docToolbar);
	docToolbar->setStretchFactor(spacer, 1);

	showButton = new QToolButton ( docToolbar, "show button" );
	showButton->setText ( "Search Selected Folder..." );
	//showButton->setPixmap ( SmallIcon ( "find" ) );
	showButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, docConfigButton->sizePolicy().hasHeightForWidth() ) );
	showButton->setToggleButton ( true );
	showButton->setMinimumHeight ( 23 );

	completionCombo = new KHistoryCombo ( true, searchToolbar, "completion combo box" );
	
	startButton = new QToolButton ( searchToolbar, "start searching" );
	startButton->setPixmap ( SmallIcon ( "key_enter" ) );
	startButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType ) 0, 0, 0, startButton->sizePolicy().hasHeightForWidth() ) );
	QToolTip::add ( startButton, i18n ( "Start searching" ) );

	nextButton = new QToolButton ( searchToolbar, "next match button" );
	nextButton->setPixmap ( SmallIcon ( "next" ) );
	nextButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, nextButton->sizePolicy().hasHeightForWidth() ) );
	QToolTip::add ( nextButton, i18n ( "Jump to next matching entry" ) );
	nextButton->setEnabled( false );

	prevButton = new QToolButton ( searchToolbar, "previous match button" );
	prevButton->setPixmap ( SmallIcon ( "previous" ) );
	prevButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, prevButton->sizePolicy().hasHeightForWidth() ) );
	QToolTip::add ( prevButton, i18n ( "Jump to last matching entry" ) );
	prevButton->setEnabled( false );

	docToolbar->setMaximumHeight ( docConfigButton->height() );

	docView = new KListView ( this, "documentation list view" );

    docView->setFocusPolicy(ClickFocus);
    docView->setRootIsDecorated(true);
    docView->setResizeMode(QListView::LastColumn);
    docView->setSorting(-1);
    docView->header()->hide();
    docView->addColumn(QString::null);

    folder_bookmarks = new DocTreeBookmarksFolder(docView, "ctx_bookmarks");
    folder_bookmarks->refresh();
    
    folder_project   = new DocTreeProjectFolder(docView, "ctx_current");
    folder_project->refresh();

#ifdef WITH_DOCBASE
    folder_docbase   = new DocTreeDocbaseFolder(docView, "ctx_docbase");
#endif

    // doctocs
    KStandardDirs *dirs = DocTreeViewFactory::instance()->dirs();
    QStringList tocs = dirs->findAllResources("doctocs", QString::null, false, true);
    QStringList::Iterator tit;
    for (tit = tocs.begin(); tit != tocs.end(); ++tit)
        folder_toc.append(new DocTreeTocFolder(docView, *tit, QString("ctx_%1").arg(*tit)));

    folder_doxygen   = new DocTreeDoxygenFolder(docView, "ctx_doxygen");
    folder_doxygen->refresh();
    
    // eventually, Qt docu extra
    QListViewItem* pChild = folder_doxygen->firstChild();
    while (pChild && pChild->text(0) != "qt") {
        pChild = pChild->nextSibling();
    }
    
    if (!pChild) {        
        // qt docu not found in doxygen subtree
        KConfig *config = DocTreeViewFactory::instance()->config();
        if (config) 
        {
            config->setGroup("General");
            QString qtdocdir(config->readEntry("qtdocdir", QT_DOCDIR));
            if (!qtdocdir.isEmpty()) 
            {
                folder_qt = new DocTreeQtFolder(docView, qtdocdir+"qt.xml", "ctx_qt");
                if (folder_qt) 
                {
                    folder_qt->setFileName(qtdocdir + "/index.html");
                    folder_qt->refresh();
                }
            }
        }
    }
       
	docConfigAction = new KAction(i18n("Customize..."), "configure", 0,
		this, SLOT(slotConfigure()), actions, "documentation options");

	connect ( showButton, SIGNAL ( toggled ( bool ) ), this, SLOT ( slotShowButtonToggled ( bool ) ) );
	connect ( docConfigButton, SIGNAL ( clicked() ), this, SLOT ( slotConfigure() ) );
	connect ( nextButton, SIGNAL ( clicked() ), this, SLOT ( slotJumpToNextMatch() ) );
	connect ( prevButton, SIGNAL ( clicked() ), this, SLOT ( slotJumpToPrevMatch() ) );
	connect ( startButton, SIGNAL ( clicked() ), this, SLOT ( slotStartSearching() ) );
	connect ( completionCombo, SIGNAL ( returnPressed ( const QString& ) ), this, SLOT ( slotHistoryReturnPressed ( const QString& ) ) );

    connect( docView, SIGNAL(executed(QListViewItem*)),
             this, SLOT(slotItemExecuted(QListViewItem*)) );
    connect( docView, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
             this, SLOT(slotContextMenu(KListView*, QListViewItem*, const QPoint&)) );
	connect ( docView, SIGNAL ( selectionChanged ( QListViewItem* ) ), this, SLOT ( slotSelectionChanged ( QListViewItem* ) ) );

    m_part = part;
}


DocTreeViewWidget::~DocTreeViewWidget()
{}

void DocTreeViewWidget::searchForItem ( const QString& currentText )
{
	completionCombo->addToHistory( currentText );
	QListViewItem* current = docView->currentItem();
	if( current->firstChild() ) 
	{  //only allow items with childs to be searched in
		QListViewItemIterator  docViewIterator( current );
		while( docViewIterator.current() )
		{// now we do the search
			if( docViewIterator.current()->text(0).find( currentText, false )>=0 ) 
			{
				searchResultList.append( docViewIterator.current() );
			}
			++docViewIterator;
		}
	}
}

void DocTreeViewWidget::slotJumpToNextMatch()
{
	if( searchResultList.next() )
	{
		docView->setSelected ( searchResultList.current(), true );
		docView->ensureItemVisible ( searchResultList.current() );
		slotItemExecuted ( searchResultList.current() );
		prevButton->setEnabled( true );

		if(searchResultList.current() == searchResultList.getLast() )
			nextButton->setEnabled( false );
	}
	else
	{
		searchResultList.last();
	}

}

void DocTreeViewWidget::slotJumpToPrevMatch()
{        
	if( searchResultList.prev() )
	{
		docView->setSelected ( searchResultList.current(), true );
		docView->ensureItemVisible ( searchResultList.current() );
		slotItemExecuted ( searchResultList.current() );
		nextButton->setEnabled( true );

		if(searchResultList.current() == searchResultList.getFirst() )
			prevButton->setEnabled( false );
	}
	else
	{
		searchResultList.first();
	 }
}

void DocTreeViewWidget::slotShowButtonToggled ( bool on )
{
	if ( on )
	{
		searchToolbar->show();
		hLine->show();
	}
	else
	{
		searchToolbar->hide();
		hLine->hide();
	}
}

void DocTreeViewWidget::slotStartSearching()
{
	QString currentText = completionCombo->currentText();
	slotHistoryReturnPressed ( currentText );
}

void DocTreeViewWidget::slotHistoryReturnPressed ( const QString& currentText )
{
	nextButton->setEnabled( false );
	prevButton->setEnabled( false );
	searchResultList.clear();

	if( currentText.length() > 0 )
		searchForItem( currentText ); //fills searchResultList

	
	if ( searchResultList.count() )
	{
		kdDebug ( 9002 ) << "Found a matching entry!" << endl;
		docView->setSelected ( searchResultList.first(), true );
		docView->ensureItemVisible (  searchResultList.first() );
		slotItemExecuted ( searchResultList.first() );
	}
	if ( searchResultList.count() > 1 )
	{
		nextButton->setEnabled( true );
	}
}

void DocTreeViewWidget::slotSelectionChanged ( QListViewItem* item )
{
	docConfigButton->setEnabled ( true );
	contextItem = item;

	if( !item->parent() )
	{// current is a toplevel item, so we initialize all childs
		QListViewItem * myChild = item->firstChild();
		while( myChild && myChild->parent()) 
		{// only initialize current folder, not the below ones
			myChild->setOpen( true );
			myChild->setOpen( false );
            
			myChild = myChild->itemBelow();
		}
	}

}

void DocTreeViewWidget::slotItemExecuted(QListViewItem *item)
{
    if (!item)
        return;

    // We assume here that ALL items in the list view
    // are DocTreeItem's
    DocTreeItem *dtitem = static_cast<DocTreeItem*>(item);
    
    QString ident = dtitem->fileName();
    if (ident.isEmpty())
        return;
    
    kdDebug(9002) << "Showing: " << ident << endl;
    m_part->partController()->showDocument(KURL(ident), dtitem->context());
    m_part->topLevel()->lowerView(this);
}


void DocTreeViewWidget::slotContextMenu(KListView *, QListViewItem *item, const QPoint &p)
{
    if (!item)
        return;
    contextItem = item;
    KPopupMenu popup(i18n("Documentation Tree"), this);
    popup.insertItem(i18n("Properties..."), this, SLOT(slotConfigure()));
    
    DocTreeItem *dItem = dynamic_cast<DocTreeItem*>( item );
    DocumentationContext dcontext( dItem->fileName(), "" );

    QListViewItem* i = contextItem;
    while(i->parent()) // go to folder
    {
        i = i->parent();
    }
    if ( i != folder_bookmarks && dItem && !dItem->fileName().isEmpty() )
    {
        popup.insertItem(i18n("Add to Bookmarks"), this, SLOT(slotAddBookmark()));
        dcontext = DocumentationContext( dItem->fileName(), dItem->text(0) );    
    }
    if (  contextItem->parent() && dItem && contextItem->parent() == folder_bookmarks )
    {
        popup.insertItem(i18n("Remove"), this, SLOT(slotRemoveBookmark()));   
        dcontext = DocumentationContext( dItem->fileName(), dItem->text(0) );    
    }
    m_part->core()->fillContextMenu( &popup , &dcontext );
    popup.exec(p);
}


void DocTreeViewWidget::slotConfigure()
{                                                                                         
    KDialogBase dlg(KDialogBase::TreeList, i18n("Customize documentation tree"),
                    KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, this,
                    "customization dialog");
    QVBox *vbox1 = dlg.addVBoxPage(i18n("Documentation Tree Global"));
    DocTreeGlobalConfigWidget *w1 = new DocTreeGlobalConfigWidget(this, vbox1, "doctreeview config widget2");
    connect(&dlg, SIGNAL(okClicked()), w1, SLOT(accept()));
    dlg.exec();
    delete w1;

}



void DocTreeViewWidget::configurationChanged()
{
    kdDebug(9002) << "DocTreeViewWidget::configurationChanged()" << endl;
    QTimer::singleShot(0, this, SLOT(refresh()));
}


void DocTreeViewWidget::refresh()
{
    kdDebug(9002) << "DocTreeViewWidget::refresh()" << endl;
//    folder_kdelibs->refresh();
    folder_doxygen->refresh();
    folder_bookmarks->refresh();
    folder_project->refresh();
}


void DocTreeViewWidget::projectChanged(KDevProject *project)
{
    folder_project->setProject(project);
    folder_project->refresh();

    // Remove all...
    docView->takeItem(folder_bookmarks);
    docView->takeItem(folder_project);
#ifdef WITH_DOCBASE
    docView->takeItem(folder_docbase);
#endif
    QListIterator<DocTreeTocFolder> it1(folder_toc);
    for (; it1.current(); ++it1)
        docView->takeItem(it1.current());
    
    docView->takeItem(folder_doxygen);
    docView->takeItem(folder_qt);
//    docView->takeItem(folder_kdevelop);

    // .. and insert all again except for ignored items
    QDomElement docEl = m_part->projectDom()->documentElement();
    QDomElement doctreeviewEl = docEl.namedItem("kdevdoctreeview").toElement();

    // ignoretocs
    QStringList ignoretocs;
    if (project) {
        QDomElement ignoretocsEl = doctreeviewEl.namedItem("ignoretocs").toElement();
        QDomElement tocEl = ignoretocsEl.firstChild().toElement();
        while (!tocEl.isNull()) {
            if (tocEl.tagName() == "toc")
                ignoretocs << tocEl.firstChild().toText().data();
            tocEl = tocEl.nextSibling().toElement();
        }
    }
    
    docView->insertItem(folder_bookmarks);
    docView->insertItem(folder_project);
#ifdef WITH_DOCBASE
    docView->insertItem(folder_docbase);
#endif
    QListIterator<DocTreeTocFolder> it2(folder_toc);
    for (; it2.current(); ++it2) {
        if (!ignoretocs.contains(it2.current()->tocName()))
            docView->insertItem(it2.current());
    }
    
    docView->insertItem(folder_doxygen);
//    if (!ignoretocs.contains("kde"))
//        docView->insertItem(folder_kdelibs);

    docView->insertItem(folder_qt);

    docView->triggerUpdate();
}


QString DocTreeViewWidget::locatehtml(const QString &fileName)
{

    QString path = locate("html", KGlobal::locale()->language() + '/' + fileName);
    if (path.isNull())
       path = locate("html", "default/" + fileName);

    return path;
}


void DocTreeViewWidget::slotAddBookmark()
{
	DocTreeItem *item = dynamic_cast<DocTreeItem*>( contextItem );
	if( item ) 
	{
		DocTreeViewTool::addBookmark( item->text(0), item->fileName() );
		folder_bookmarks->refresh();
	}
}

void DocTreeViewWidget::slotRemoveBookmark()
{
	DocTreeItem *item = dynamic_cast<DocTreeItem*>( contextItem );
	if( item ) 
	{
		int posFolder = docView->itemIndex( folder_bookmarks );
		int i = docView->itemIndex( item ) - posFolder;
		//kdDebug(9002) << "remove item: " << i << endl;
		
		DocTreeViewTool::removeBookmark( i );

		folder_bookmarks->refresh();
	}
}

#include "doctreeviewwidget.moc"
