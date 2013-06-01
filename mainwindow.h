#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QtSql>

#include "sqleditor.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void newQuery();
    void updateAllUi();
    void updateTablesListView();
    void updateQueriesListView();
    void updateTitle();
    void on_saveQuery(QString name, QString sql, QString oldName);
    void on_sqleditorClosed(QCloseEvent *event, QString name);

signals:
    void querySaved(QString,QString);

protected:
    void closeEvent(QCloseEvent *event);
    
private:
    QMap<QString, sqleditor*> openEditors;
    sqleditor* create_sqleditor_window(QString title, QString sql, bool isNew);
    void reset_project();
    sqleditor *activeSqlEditor();
    QMdiSubWindow *findSqlEditor(const QString &queryName);
    QString dbFilename;
    QString sqlearnFilename;
    QSignalMapper* windowMapper;
    int newQueryCounter;
    bool isNew;
    bool isUnsaved;
    bool generateTempDbFName();

    void reset_ui();
    void load_from_file(QString fileName);
    void save_to_file(QString fileName);
    bool tmp_db_is_open();
    void open_tmp_db();
    void close_tmp_db();
    QByteArray tmp_db_to_byte_array();

    void createMenus();
    void createActions();

    //MainWindowUI *ui;
    QListView *tablesListView;
    QListView *queriesListView;
    QMdiArea *mdiArea;
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *queryMenu;
    QMenu *windowMenu;
    QMenu *helpMenu;
    QAction *newAct;
    QAction *newQueryAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *closeAct;
    QAction *closeSEAct;
    QAction *closeAllAct;
    QAction *tileAct;
    QAction *cascadeAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *separatorAct;
    QAction *exitAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QMap<QString, QString> queries;
private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void close();
    void about();
    void queryListItemDoubleClicked(QModelIndex);
    void showQueriesContextMenu(const QPoint& pos);
    void setActiveSubWindow(QWidget *window);
    void updateWindowMenu();
    void switchLayoutDirection();
};

#endif // MAINWINDOW_H
