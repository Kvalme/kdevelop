/***************************************************************************
                    crealfileview.cpp - 
                             -------------------                                         
                            
    begin                : 9 Sept 1998                                        
    copyright            : (C) 1998 by Stefan Bartel                         
    email                : bartel@rz.uni-potsdam.de                                     
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#include "qdir.h"
#include "qstrlist.h"
#include <kmsgbox.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <klocale.h>
#include <iostream.h>
#include <assert.h>
#include "crealfileview.h"
#include "cproject.h"
#include "vc/versioncontrol.h"
#include "resource.h"

/*********************************************************************
 *                                                                   *
 *                     CREATION RELATED METHODS                      *
 *                                                                   *
 ********************************************************************/


/*------------------------------------- CRealFileView::CRealFileView()
 * CRealFileView()
 *   Constructor.
 *
 * Parameters:
 *   parent         Parent widget.
 *   name           The name of this widget.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
CRealFileView::CRealFileView(QWidget*parent,const char* name)
  : CTreeView(parent,name)
{
	addColumn("");
	addColumn("");
	header()->hide();
	header()->setClickEnabled(false);

  // Create the popupmenus.
  popup = 0;
  showNonPrjFiles=true;
  file_col = 0;
  connect(this,
          SIGNAL(selectionChanged(QListViewItem*)), 
          SLOT(slotSelectionChanged(QListViewItem*)));
}

CRealFileView::~CRealFileView(){
  if (popup)
      delete popup;
}


/*********************************************************************
 *                                                                   *
 *                          PUBLIC METHODS                           *
 *                                                                   *
 ********************************************************************/

/*------------------------------------------ CRealFileView::refresh()
 * refresh()
 *   Add all files in the project directory.
 *
 * Parameters:
 *   proj          The project specification.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CRealFileView::refresh(CProject* prj) 
{
  assert( prj );
  project=prj;

	if (showNonPrjFiles) {
		setColumnWidthMode(0,Maximum);
		setColumnWidthMode(1,Maximum);
		setColumnWidthMode(2,Maximum);
		setColumnWidth(0,0);
		setColumnWidth(1,0);
		setColumnWidth(2,0);
	} else {
		setColumnWidthMode(0,Maximum);
		setColumnWidthMode(1,Manual);
		setColumnWidthMode(2,Manual);
		setColumnWidth(0,0);
		setColumnWidth(1,0);
		setColumnWidth(2,0);
	}

  QString projectdir=project->getProjectDir();
  if(projectdir.right(1)=="/") {
    projectdir.truncate(projectdir.length()-1);
  }
  this->projectDir=projectdir;
  QDir dir(projectdir);
  if (!dir.exists()) {
    return;
  }

  // Remove all entries.
  treeH->clear();

  // Save all files registered in the project in the filelist variable.
  project->getAllFiles(filelist);

  // Add the root item.
  pRootItem = treeH->addRoot( projectdir, THPROJECT );
  pRootItem->setOpen(true);

  scanDir(projectdir, pRootItem);
}

void CRealFileView::addFilesFromDir( const QString& directory, QListViewItem* parent ) {

    QDir theDir( directory );
    QStrList fl;
    QListViewItem* item;
    QString f;
    QString d=directory;
    d.remove(0,projectDir.length());
    VersionControl* vc;
    VersionControl::State reg;

    // Add all files for this directory
    theDir.setFilter(QDir::Files);
    fl=*(theDir.entryList());
    
    for( fl.first(); fl.current(); fl.next() ) {
      f=d;
      f.append("/");
      f.append(fl.current());
      f.remove(0,1);
      if( isInstalledFile(f) ) {
	item = treeH->addItem( fl.current(), THINSTALLED_FILE, parent);
	if (showNonPrjFiles) {
	  item->setText(1,i18n("registered"));
	  vc=project->getVersionControl();
	  if (vc!=0) {
	    reg=vc->registeredState(directory+'/'+fl.current());
	    if (reg & VersionControl::canBeCommited) {
	      item->setText(2,i18n("VCS"));
	    } else {
	      item->setText(2,i18n("local"));
	    }
	  }
	}
      } else {
	if (showNonPrjFiles) {
	  item = treeH->addItem( fl.current(), THC_FILE, parent,"");
	  vc=project->getVersionControl();
	  if (vc!=0) {
	    reg=vc->registeredState(directory+'/'+fl.current());
	    if (reg & VersionControl::canBeCommited) {
	      item->setText(2,i18n("VCS"));
	    } else {
	      item->setText(2,i18n("local"));
	    }
	  }
	}
      }
    }
}

void CRealFileView::scanDir(const QString& directory, QListViewItem* parent) 
{
  QString currentPath;
  QListViewItem* lastFolder;
  QStrList dirList;
  QDir dir(directory);

  // Stop recursion if the directory doesn't exist.
  if (!dir.exists()) {
    return;
  }

  dir.setSorting(QDir::Name);
  dir.setFilter(QDir::Dirs);
  dirList = *(dir.entryList());
  
  // Remove '.' and  '..'
  dirList.first();
  dirList.remove();
  dirList.remove();

  // Recurse through all directories
  while( dirList.current() ) 
  {
    lastFolder = treeH->addItem( dirList.current(), THFOLDER, parent );
    lastFolder->setOpen( false );
    
    // Recursive call to fetch subdirectories
    currentPath = directory+"/"+dirList.current();
    scanDir( currentPath, lastFolder );
    
    // Add the files in the recursed directory.
    //    addFilesFromDir( currentPath, lastFolder );
    
    treeH->setLastItem( lastFolder );
    
    dirList.next();
  } 
  
  // Add files in THIS directory as well.
  addFilesFromDir( directory, parent );
}

/*----------------------------------- CRealFileView::getCurrentPopup()
 * getCurrentPopup()
 *   Get the current popupmenu.
 *
 * Parameters:
 *   -
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
KPopupMenu *CRealFileView::getCurrentPopup()
{
  if (popup)
    delete popup;
  
  switch( treeH->itemType() )
  {
    
  case THPROJECT :
    popup = new KPopupMenu(i18n("RFV Options"));
    popup->insertItem( i18n("Show non-project Files"),
		       this, SLOT(slotShowNonPrjFiles()), 0, ID_RFV_SHOW_NONPRJFILES );
    popup->setCheckable(true);
    if(showNonPrjFiles) popup->setItemChecked(ID_RFV_SHOW_NONPRJFILES, true);
    if (project->getVersionControl())
      {
	popup->insertSeparator(-1);
	popup->insertItem( i18n("Update"),
			   this, SLOT(slotUpdate()) );
	popup->insertItem( i18n("Commit"),
			   this, SLOT(slotCommit()) );
	popup->insertItem( i18n("Add to Repository"),
			   this, SLOT(slotAddToRepository()) );
	popup->insertItem( i18n("Remove from Repository (and Disk)"),
			   this, SLOT(slotRemoveFromRepository()) );
	break;
      }
    break;


  case THINSTALLED_FILE:
    popup = new KPopupMenu(i18n("File (Registered)"));
    popup->insertItem( i18n("Remove File from Project..."),
		       this, SLOT(slotRemoveFileFromProject()));
    popup->insertItem( *(treeH->getIcon( THDELETE )), i18n("Remove File from Disk..."),
		       this, SLOT(slotDeleteFilePhys()));
    popup->insertSeparator();
    popup->insertItem( i18n("Properties..."),
		       this, SLOT(slotShowFileProperties()));
    break;

  case THC_FILE:
      popup = new KPopupMenu(i18n("File"));
      popup->insertItem( i18n("Add File to Project..."),
                         this, SLOT(slotAddFileToProject()));
      popup->insertItem( *(treeH->getIcon( THDELETE )), i18n("Remove File from Disk..."),
                         this, SLOT(slotDeleteFilePhys()));
      break;
  case THFOLDER:
      if (project->getVersionControl())
          {
              popup = new KPopupMenu(i18n("Folder"));
              popup->insertItem( i18n("Update"),
                                 this, SLOT(slotUpdate()) );
              popup->insertItem( i18n("Commit"),
                                 this, SLOT(slotCommit()) );
              popup->insertItem( i18n("Add to Repository"),
                                 this, SLOT(slotAddToRepository()) );
              popup->insertItem( i18n("Remove from Repository (and Disk)"),
                                 this, SLOT(slotRemoveFromRepository()) );
              break;
          }
    default:
      popup = 0;
  }

  VersionControl *vc = project->getVersionControl();
  if ( (treeH->itemType() == THINSTALLED_FILE || treeH->itemType() == THC_FILE)
       && vc)
      {
          VersionControl::State reg =
              vc->registeredState(getFullFilename(currentItem()));
          int id;
          popup->insertSeparator();
          id = popup->insertItem( i18n("Update"),
                                  this, SLOT(slotUpdate()) );
          popup->setItemEnabled(id, reg & VersionControl::canBeCommited);
          id = popup->insertItem( i18n("Commit"),
                                  this, SLOT(slotCommit()) );
          popup->setItemEnabled(id, reg & VersionControl::canBeCommited);
          id = popup->insertItem( i18n("Add to Repository"),
                                  this, SLOT(slotAddToRepository()) );
          popup->setItemEnabled(id, reg & VersionControl::canBeAdded);
          id = popup->insertItem( i18n("Remove from Repository (and disk)"),
                                  this, SLOT(slotRemoveFromRepository()) );
          popup->setItemEnabled(id, !(reg & VersionControl::canBeAdded));
      }
              
  return popup;
}

QString CRealFileView::getRelFilename(QListViewItem* pItem) {

  QString filename = pItem->text(file_col);
  if (pItem != pRootItem) {
    pItem=pItem->parent();
  }
  while (pItem != pRootItem) {
    filename = QString(pItem->text(file_col)) + "/" + filename;
    pItem = pItem->parent();
  }
  return filename;
}

QString CRealFileView::getFullFilename(QListViewItem* pItem) {

  QString filename = pItem->text(file_col);
  while (pItem != pRootItem) {
    pItem = pItem->parent();
    filename = QString(pItem->text(file_col)) + "/" + filename;
  }
  return filename;
}

bool CRealFileView::isInstalledFile(QString filename) 
{
	int i=0;
	bool b=false;
	i=filelist.contains(filename);
	if ( i>0 ) b=true;
	return b;
}

/*********************************************************************
 *                                                                   *
 *                              SLOTS                                *
 *                                                                   *
 ********************************************************************/

void CRealFileView::slotSelectionChanged(QListViewItem* selection) 
{
  THType itemType=treeH->itemType();

  if( itemType!=THFOLDER && itemType!=THPROJECT &&
	(mouseBtn == LeftButton || mouseBtn == MidButton))
    emit fileSelected(getFullFilename(selection));
}

void CRealFileView::slotAddFileToProject() {

  QString filename=getFullFilename(currentItem());
  QString msg;
  msg.sprintf(i18n("Do you want to add the file\n%s\nto the project ?"), filename.data());
  if (KMsgBox::yesNo(0, i18n("Question"), msg, KMsgBox::QUESTION) == 2)
    return;

  emit addFileToProject(filename);
}

void CRealFileView::slotRemoveFileFromProject() {

  QString filename=getRelFilename(currentItem());
  QString msg;
  msg.sprintf(i18n("Do you really want to remove the file\n%s\nfrom project?\n\t\tIt will remain on disk."), filename.data());
  if (KMsgBox::yesNo(0, i18n("Warning"), msg, KMsgBox::EXCLAMATION) == 2)
    return;

  emit removeFileFromProject(filename);
}

void CRealFileView::slotDeleteFilePhys() {

  QString filename=getRelFilename(currentItem());
  QString msg;
  msg.sprintf(i18n("Do you really want to delete the file\n%s\nfrom the disk?\nThere is no way to restore it!"), filename.data());
  if(KMsgBox::yesNo(0, i18n("Warning"), msg, KMsgBox::EXCLAMATION) == 2)
    return;

  QFile::remove(getFullFilename(currentItem()));

  if (isInstalledFile(filename)) 
    emit removeFileFromProject(filename);

  refresh(project);
}

void CRealFileView::slotShowFileProperties() {
  emit showFileProperties(getRelFilename(currentItem()));
}


void CRealFileView::slotAddToRepository()
{
    project->getVersionControl()->add(getFullFilename(currentItem()));
}


void CRealFileView::slotRemoveFromRepository()
{
    project->getVersionControl()->remove(getFullFilename(currentItem()));
    refresh(project);
}
 

void CRealFileView::slotUpdate()
{
    QString file_dir_name = getFullFilename(currentItem());
    QFileInfo file_info(file_dir_name);
    if (file_info.isFile()){ 
	emit updateFileFromVCS(file_dir_name);
    }
    if(file_info.isDir()){
      emit updateDirFromVCS(file_dir_name);
    }
}


void CRealFileView::slotCommit()
{
    QString file_dir_name = getFullFilename(currentItem());
    QFileInfo file_info(file_dir_name);
    if (file_info.isFile()){ 
	emit commitFileToVCS(file_dir_name);
    }
    if(file_info.isDir()){
      emit commitDirToVCS(file_dir_name);
    }
}
 
void CRealFileView::slotShowNonPrjFiles() {
	showNonPrjFiles=!showNonPrjFiles;
	refresh(project);
}





















