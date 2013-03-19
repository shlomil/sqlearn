#ifndef SQLEDITOR_H
#define SQLEDITOR_H

#include <QWidget>
#include "ui_sqleditor.h"

#define CONNECTION_NAME "TMPDB"

class sqleditor : public QSplitter
{
    Q_OBJECT

    QString queryName;
    QString originalSql;
    bool isNew;
    bool isUnsaved;
    void closeEvent(QCloseEvent *event);
public:
    explicit sqleditor(QWidget *parent, QString name, QString sql, bool isNew);
    ~sqleditor();

    void insertRow();
    void deleteRow();
    void updateActions();

public slots:
    void exec();
    void showTable(const QString &table);
    void showMetaData(const QString &table);
    void currentChanged() { updateActions(); }
    void _on_querySaved(QString name, QString oldName);

signals:
    void sqleditorClosed(QCloseEvent*, QString);
    void sqlQueryExecuted();
    void saveQuery(QString name, QString sql, QString oldName);
    
private slots:
    void on_submitButton_clicked();
    void on_clearButton_clicked();
    void on_saveButton_clicked();
    void on_sqlEdit_textChanged();

private:
    Ui::sqleditor *ui;
    void updateTitle();
};

#endif // SQLEDITOR_H
