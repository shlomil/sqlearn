#include <QtGui>
#include "mainwindow.h"
#include "simplesqlparser.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    newQueryCounter(1)
{
    this->mdiArea = new QMdiArea();
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    createActions();
    createMenus();

    QSplitter* qs1 = new QSplitter();
    QSplitter* qs2 = new QSplitter();    

    tablesListView = new QListView();
    tablesListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    queriesListView = new QListView();
    queriesListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    queriesListView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(queriesListView, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(showQueriesContextMenu(const QPoint&)));

    connect(queriesListView, SIGNAL(doubleClicked(QModelIndex)),
                 this, SLOT(queryListItemDoubleClicked(QModelIndex)));

    QVBoxLayout* tablesLayout = new QVBoxLayout();
    tablesLayout->setSpacing(0);
    tablesLayout->setMargin(0);
    tablesLayout->addWidget(new QLabel(" Tables:"));
    tablesLayout->addWidget(tablesListView);
    QWidget* tablesWidget = new QWidget();
    tablesWidget->setLayout(tablesLayout);

    QVBoxLayout* queriesLayout = new QVBoxLayout();
    queriesLayout->setMargin(0);
    queriesLayout->setSpacing(0);
    queriesLayout->addWidget(new QLabel(" Queries:"));
    queriesLayout->addWidget(queriesListView);
    QWidget* queriesWidget = new QWidget();
    queriesWidget->setLayout(queriesLayout);


    qs2->addWidget(tablesWidget);
    qs2->addWidget(queriesWidget);
    qs2->setOrientation(Qt::Vertical);

    qs2->setVisible(true);
    qs1->addWidget(qs2);
    qs1->addWidget(mdiArea);

    setCentralWidget(qs1);

    isNew = true;
    isUnsaved = false;

    updateTablesListView();

    newFile();
}

MainWindow::~MainWindow()
{
    //TODO: delete ui;
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open sqlearn db"), QDir::currentPath(), tr("SQLearn db Files (*.sqlearn)"));
    load_from_file(fileName);
    isNew = false;
    isUnsaved = false;
    updateAllUi();
}

void MainWindow::save()
{
    if(isNew)
    {
        saveAs();
        return;
    }

    save_to_file(sqlearnFilename);

    isUnsaved = false;
    updateTitle();
}

void MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save sqlearn db file as"),QDir::currentPath()+QDir::separator()+sqlearnFilename,tr("SQLearn db Files (*.sqlearn)"));
    if(fileName.isNull())
        return;

    save_to_file(fileName);

    sqlearnFilename = fileName;
    isNew = false;
    isUnsaved = false;
    updateTitle();
}



void MainWindow::close()
{
    newFile();
}


bool MainWindow::generateTempDbFName()
{
    QTemporaryFile tf("sqlitedb");

    //create unique temp name and check it's writeable
    if(!tf.open())
        return false;
    dbFilename = tf.fileName();
    tf.setAutoRemove(true);
    tf.close();
    return true;
}

sqleditor* MainWindow::create_sqleditor_window(QString title, QString sql, bool isNew)
{
    sqleditor* subWindow = new sqleditor(this, title, sql, isNew);

    connect(subWindow, SIGNAL(sqlQueryExecuted()), this, SLOT(updateTablesListView()));
    connect(subWindow, SIGNAL(saveQuery(QString,QString,QString)), this, SLOT(on_saveQuery(QString,QString,QString)));
    connect(subWindow, SIGNAL(sqleditorClosed(QCloseEvent*, QString)), this, SLOT(on_sqleditorClosed(QCloseEvent*, QString)));

    openEditors.insert(title, subWindow);

    mdiArea->addSubWindow(subWindow);
    subWindow->show();

    return subWindow;
}

void MainWindow::newQuery()
{
    QString qName;
    do{
        qName = QString("new-query-%1").arg(newQueryCounter++);
    }while(openEditors.keys().contains(qName) || queries.keys().contains(qName));
    create_sqleditor_window(qName, "", true);
}

void MainWindow::reset_ui()
{
    queries.clear();

    for(QList<sqleditor*> vals=openEditors.values() ;
        !vals.empty() ; vals.removeAt(0))
        vals.at(0)->parentWidget()->close();

    openEditors.clear();
    newQueryCounter=1;
}

bool MainWindow::tmp_db_is_open()
{
    return QSqlDatabase::contains(CONNECTION_NAME);
}

void MainWindow::open_tmp_db()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",CONNECTION_NAME);
    db.setHostName("localhost");
    db.setDatabaseName(dbFilename);
}

void MainWindow::close_tmp_db()
{
    if(!tmp_db_is_open())
        return;

    QSqlDatabase::database(CONNECTION_NAME).close();
    QSqlDatabase::removeDatabase(CONNECTION_NAME);
}


QByteArray MainWindow::tmp_db_to_byte_array()
{
    QByteArray ret;
    if(!tmp_db_is_open())
        return ret;

    close_tmp_db();

    QFile file(dbFilename);
    file.open(QIODevice::ReadOnly);

    ret = file.readAll();

    file.close();

    open_tmp_db();
    return ret;
}

void MainWindow::newFile()
{
    static int tmpNameCounter = 1;

    do{
        sqlearnFilename = QString("Untitled-%1.sqlearn").arg(tmpNameCounter++);
    }while(QFile::exists(sqlearnFilename));

    if(tmp_db_is_open())
    {
        close_tmp_db();
    }
    if(!generateTempDbFName())
    {
        //TODO: notify user about error
        return;
    }
    newQueryCounter = 1;

    open_tmp_db();

    reset_ui();

    isNew = true;
    isUnsaved = false;

    updateAllUi();
}


void MainWindow::createActions()
{
    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    newQueryAct = new QAction(QIcon(":/images/new.png"), tr("New &Query"), this);
    newQueryAct->setStatusTip(tr("Create a new SQL Query"));
    connect(newQueryAct, SIGNAL(triggered()), this, SLOT(newQuery()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    closeAct = new QAction(tr("Close"), this);
    closeAct->setShortcuts(QKeySequence::Close);
    closeAct->setStatusTip(tr("Close file"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

/*
    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

    closeAct = new QAction(tr("Cl&ose"), this);
    closeAct->setStatusTip(tr("Close the active window"));
    connect(closeAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeActiveSubWindow()));

    closeAllAct = new QAction(tr("Close &All"), this);
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeAllSubWindows()));

    tileAct = new QAction(tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

    cascadeAct = new QAction(tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));

    nextAct = new QAction(tr("Ne&xt"), this);
    nextAct->setShortcuts(QKeySequence::NextChild);
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, SIGNAL(triggered()),
            mdiArea, SLOT(activateNextSubWindow()));

    previousAct = new QAction(tr("Pre&vious"), this);
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    previousAct->setStatusTip(tr("Move the focus to the previous "
                                 "window"));
    connect(previousAct, SIGNAL(triggered()),
            mdiArea, SLOT(activatePreviousSubWindow()));

    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    */
}


void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(newQueryAct);

    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addAction(closeAct);
    /*fileMenu->addSeparator();
    QAction *action = fileMenu->addAction(tr("Switch layout direction"));
    connect(action, SIGNAL(triggered()), this, SLOT(switchLayoutDirection()));*/
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);
/*
    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);

    windowMenu = menuBar()->addMenu(tr("&Window"));
    updateWindowMenu();
    connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
    */
}

void MainWindow::updateQueriesListView()
{
    QStringList sl = QStringList(queries.keys());
    QStringListModel *slm = new QStringListModel(sl);
    this->queriesListView->setModel(slm);
}

void MainWindow::updateTablesListView()
{
    QStringListModel *slm = new QStringListModel(QSqlDatabase::database(CONNECTION_NAME).tables());
    this->tablesListView->setModel(slm);
}

void MainWindow::updateAllUi()
{
    updateTablesListView();
    updateQueriesListView();
    updateTitle();
}

void MainWindow::updateTitle()
{
    this->setWindowTitle(QString("SQLearn - %1%2").arg(sqlearnFilename, isUnsaved ? QString("*") : QString("")));
}

void MainWindow::on_saveQuery(QString name, QString sql, QString oldName)
{
    if(oldName != name && (queries.keys().contains(name)
                           || openEditors.keys().contains(name)))
    {
        QMessageBox::critical(this, "Cannot save query",
                              QString("a query already exists by this name: '%1'").arg(name),
                              QMessageBox::Ok,QMessageBox::Ok);
        return;
    }

    if(!oldName.isEmpty() && queries.keys().contains(oldName) && name != oldName)
    {
        queries.remove(oldName);
    }

    if(!oldName.isEmpty() && openEditors.keys().contains(oldName) && name != oldName)
    {
        openEditors.insert(name ,openEditors.value(oldName));
        openEditors.remove(oldName);
    }

    queries.insert(name, sql);
    isUnsaved = true;

    updateQueriesListView();
    updateTitle();

    emit querySaved(name, oldName);
}


void MainWindow::queryListItemDoubleClicked(QModelIndex index)
{
    QString queryName = index.data().toString();
    if(openEditors.contains(queryName))
    {
        openEditors.value(queryName)->setFocus();
        return;
    }

    openEditors.insert(queryName, create_sqleditor_window(queryName, queries.value(queryName), false));
}

void MainWindow::showQueriesContextMenu(const QPoint& pos)
{
    QPoint globalPos = queriesListView->mapToGlobal(pos);
    QMenu querieContextMenu;
    querieContextMenu.addAction("Delete");

    QAction* selectedItem = querieContextMenu.exec(globalPos);
    if (selectedItem)
    {
        QString selectedQuery = queriesListView->selectionModel()->selectedIndexes().at(0).data().toString();
        if(openEditors.keys().contains(selectedQuery))
            openEditors[selectedQuery]->parentWidget()->close();
        if(queries.contains(selectedQuery))
            queries.remove(selectedQuery);
        updateAllUi();
    }
}



void MainWindow::on_sqleditorClosed(QCloseEvent *event, QString name)
{
    openEditors.remove(name);
}

void MainWindow::save_to_file(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_8);
    out << (qint32)this->queries.count();   //number of queries

    foreach(QString queryName , queries.keys()) //pair of name & query
    {
        out << queryName;
        out << queries.value(queryName);
    }
    out << tmp_db_to_byte_array();
    file.close();
    isUnsaved = false;
}

void MainWindow::load_from_file(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_8);
    qint32 count;
    in >> count;
    for(int i=0; i<count; i++)
    {
        QString name;
        QString sql;
        in >> name >> sql;
        queries.insert(name, sql);
    }
    QByteArray db_data;
    in >> db_data;

    file.close();
    close_tmp_db();
    QFile dbfile(dbFilename);
    dbfile.open(QIODevice::WriteOnly);
    dbfile.write(db_data);
    dbfile.flush();
    dbfile.close();
    open_tmp_db();

    sqlearnFilename = fileName;

    isNew = false;

    updateQueriesListView();
}
