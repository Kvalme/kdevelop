#include "svn_logviewwidgets.h"
#include "svn_blamewidgets.h"
// #include "svn_models.h" // included int blamewidget

#include <kaction.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <QList>
#include <QVariant>
#include <QModelIndex>
#include <QContextMenuEvent>
#include <QCursor>

SvnLogviewWidget::SvnLogviewWidget( KUrl &url, KDevSubversionPart *part, QWidget *parent )
    :QWidget(parent), Ui::SvnLogviewWidget()
    ,m_url(url), m_part(part)
{
    Ui::SvnLogviewWidget::setupUi(this);
    
    m_item = new LogItem();
    m_logviewModel= new LogviewTreeModel(m_item);
    treeView->setModel( m_logviewModel );
    treeView->sortByColumn(0, Qt::DescendingOrder);
    treeView->setContextMenuPolicy( Qt::CustomContextMenu );

    m_logviewDetailedModel = new LogviewDetailedModel(m_item);
    listView->setModel( m_logviewDetailedModel );

    connect( treeView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(treeViewClicked(const QModelIndex &)) );
    connect( treeView, SIGNAL(customContextMenuRequested( const QPoint & )),
             this, SLOT( customContextMenuEvent( const QPoint & ) ) );
    connect( listView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(listViewClicked(const QModelIndex &)) );
}
SvnLogviewWidget::~SvnLogviewWidget()
{}
void SvnLogviewWidget::refreshWithNewData( QList<SvnLogHolder> datalist )
{
    m_logviewModel->prepareItemUpdate();
    m_item->setHolderList( datalist );
    m_logviewModel->finishedItemUpdate();
    treeView->resizeColumnToContents(0);
    treeView->resizeColumnToContents(1);
}

void SvnLogviewWidget::customContextMenuEvent( const QPoint &point )
{
    m_contextIndex = treeView->indexAt( point );
    if( !m_contextIndex.isValid() ){
        kDebug() << " contextMenu is not in TreeView " << endl;
        return;
    }

    KMenu menu( this );

    QAction *action = menu.addAction(i18n("Blame this Revision"));
    connect( action, SIGNAL(triggered(bool)), this, SLOT(blameRev()) );
    
    action = menu.addAction(i18n("Diff to previous revision"));
    connect( action, SIGNAL(triggered(bool)), this, SLOT(diffToPrev()) );
    
    menu.exec( treeView->viewport()->mapToGlobal(point) );
}

void SvnLogviewWidget::contextMenuEvent( QContextMenuEvent * event )
{
    kDebug() << " SvnLogviewWidget::contextMenuEvent " << endl;
//     m_contextIndex = treeView->indexAt( event->pos() );
//     if( !m_contextIndex.isValid() ){
//         kDebug() << " contextMenu is not in TreeView " << endl;
//         return;
//     }
// 
//     KMenu menu( this );
// 
//     QAction *action = menu.addAction(i18n("Blame this Revision"));
//     connect( action, SIGNAL(triggered(bool)), this, SLOT(blameRev()) );
//     menu.exec(event->globalPos());
//
    QWidget::contextMenuEvent( event );
}

void SvnLogviewWidget::blameRev()
{
    long rev = m_logviewModel->revision( m_contextIndex );
    if( rev == -1 ){ //error
        return;
    }
    // note that blame is done on single file.
    QStringList modifies = m_logviewModel->modifiedLists( m_contextIndex );
    QString selectedPath;
    if( modifies.count() > 1 ){
        SvnBlameFileSelectDlg dlg(this);
        dlg.setCandidate( &modifies );
        if( dlg.exec() == QDialog::Accepted ){
            selectedPath = dlg.selected();
        } else{
            return;
        }
        
    } else if( modifies.count() == 1 ){
        selectedPath = modifies.at(0);
    } else {
        return;
    }
    
    QString relPath = selectedPath.section( '/', 1 );
    // get repository root URL
    SvnInfoSyncJob job;
    QMap<KUrl, SvnInfoHolder> *infoMap = job.infoExec( this->m_url, NULL, NULL, false );
    if( !infoMap ) return;
    
    QList< SvnInfoHolder > holderList = infoMap->values();
    if( holderList.count() > 0 ){
        // get full Url
        SvnInfoHolder holder = holderList.first();
        KUrl absUrl =  holder.reposRootUrl;
        absUrl.addPath( relPath );
        kDebug() << " Blame requested on path " << absUrl << endl;
        // final request
        m_part->svncore()->spawnBlameThread( absUrl, true,  0, "", rev, "" );
    }
    else{
        return;
    }
}

void SvnLogviewWidget::diffToPrev()
{
    long rev = m_logviewModel->revision( m_contextIndex );
    if( rev == -1 ){ //error
        return;
    }
    SubversionUtils::SvnRevision rev1, rev2;
    rev1.revNum = rev-1;
    rev2.revNum = rev;
    m_part->svncore()->spawnDiffThread( m_url, m_url, rev1, rev2, true, false, false, false );
}
void SvnLogviewWidget::treeViewClicked( const QModelIndex &index )
{
    m_logviewDetailedModel->setNewRevision( index );
    
//     QMenu menu( this );
//     QAction *action = menu.addAction( i18n( "Blame this revision" ) );
//     connect( action, SIGNAL(triggered(bool)), this, SLOT(blameRev()) );
//     menu.exec(QCursor::pos());
}
void SvnLogviewWidget::listViewClicked( const QModelIndex &index )
{
}
// void SvnLogviewWidget::printLog(SubversionJob *j)
// {
// 
// }
#include "svn_logviewwidgets.moc"
