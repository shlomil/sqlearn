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
    
private:
    QMap<QString, sqleditor*> openEditors;
    sqleditor* create_sqleditor_window(QString title, QString sql, bool isNew);
    QString dbFilename;
    QString sqlearnFilename;
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
    QMenu *windowMenu;
    QMenu *helpMenu;
    QAction *newAct;
    QAction *newQueryAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *closeAct;
    QAction *exitAct;
    QMap<QString, QString> queries;
private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void close();
    void queryListItemDoubleClicked(QModelIndex);
};

#endif // MAINWINDOW_H
