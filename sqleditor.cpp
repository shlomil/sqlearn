#include <QtSql>
#include <QtGui>
#include "sqleditor.h"
#include "ui_sqleditor.h"
#include "simplesqlparser.h"

void sqleditor::updateTitle() {
    this->setWindowTitle((isUnsaved ? QString("*") : QString("")) + queryName);
}

sqleditor::sqleditor(QWidget *parent, QString name, QString sql, bool is_new) :
    QSplitter(parent),
    queryName(name),
    originalSql(sql),
    isNew(is_new),
    ui(new Ui::sqleditor)
{
    ui->setupUi(this);

    connect(parent, SIGNAL(querySaved(QString,QString)),this, SLOT(_on_querySaved(QString,QString)));

    ui->statusLine->setText(tr("Ready."));
    ui->sqlEdit->setText(sql);
    isUnsaved = false;
    updateTitle();
}

sqleditor::~sqleditor()
{
    delete ui;
}


void sqleditor::exec()
{
    QSqlQueryModel *model = new QSqlQueryModel(ui->table);
    model->setQuery(QSqlQuery(ui->sqlEdit->toPlainText(), QSqlDatabase::database(CONNECTION_NAME)));
    ui->table->setModel(model);

    if (model->lastError().type() != QSqlError::NoError)
        ui->statusLine->setText(model->lastError().text());
    else if (model->query().isSelect()) {
        SimpleSqlParser p;
        p.parse(ui->sqlEdit->toPlainText());
        if(p.detected_error_unaggregated_args())
            QMessageBox::critical(this, tr("Selection of non-aggregated columns!"), tr("In an aggregated query you should select only aggregated columns or use aggregation functions"));
        if(p.detected_error_select_nested_in_select())
            QMessageBox::critical(this, tr("Nested select query in the select line!"), tr(""));
        if(p.detected_error_nested_call_to_aggregated_func())
            QMessageBox::critical(this, tr("PARSER ERROR"), tr("Selection of non-aggregated columns!"));

        ui->statusLine->setText(tr("Query OK."));
    } else
        ui->statusLine->setText(tr("Query OK, number of affected rows: %1").arg(model->query().numRowsAffected()));

    updateActions();

    emit sqlQueryExecuted();

}

void sqleditor::updateActions()
{

}

void sqleditor::insertRow()
{
    QSqlTableModel *model = qobject_cast<QSqlTableModel *>(ui->table->model());
    if (!model)
        return;

    QModelIndex insertIndex = ui->table->currentIndex();
    int row = insertIndex.row() == -1 ? 0 : insertIndex.row();
    model->insertRow(row);
    insertIndex = model->index(row, 0);
    ui->table->setCurrentIndex(insertIndex);
    ui->table->edit(insertIndex);
}

void sqleditor::deleteRow()
{
    QSqlTableModel *model = qobject_cast<QSqlTableModel *>(ui->table->model());
    if (!model)
        return;

    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    QModelIndexList currentSelection = ui->table->selectionModel()->selectedIndexes();
    for (int i = 0; i < currentSelection.count(); ++i) {
        if (currentSelection.at(i).column() != 0)
            continue;
        model->removeRow(currentSelection.at(i).row());
    }

    model->submitAll();
    model->setEditStrategy(QSqlTableModel::OnRowChange);

    updateActions();
}

void sqleditor::showTable(const QString &t)
{
    QSqlTableModel *model = new QSqlTableModel(ui->table);
    model->setEditStrategy(QSqlTableModel::OnRowChange);
    model->setTable(QSqlDatabase::database(CONNECTION_NAME).driver()->escapeIdentifier(t, QSqlDriver::TableName));
    model->select();
    if (model->lastError().type() != QSqlError::NoError)
        //emit statusMessage(model->lastError().text());
        ui->statusLine->setText(model->lastError().text());
    ui->table->setModel(model);
    ui->table->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed);

    connect(ui->table->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentChanged()));
    updateActions();
}

void sqleditor::showMetaData(const QString &t)
{
    QSqlRecord rec = QSqlDatabase::database(CONNECTION_NAME).record(t);
    QStandardItemModel *model = new QStandardItemModel(ui->table);

    model->insertRows(0, rec.count());
    model->insertColumns(0, 7);

    model->setHeaderData(0, Qt::Horizontal, "Fieldname");
    model->setHeaderData(1, Qt::Horizontal, "Type");
    model->setHeaderData(2, Qt::Horizontal, "Length");
    model->setHeaderData(3, Qt::Horizontal, "Precision");
    model->setHeaderData(4, Qt::Horizontal, "Required");
    model->setHeaderData(5, Qt::Horizontal, "AutoValue");
    model->setHeaderData(6, Qt::Horizontal, "DefaultValue");


    for (int i = 0; i < rec.count(); ++i) {
        QSqlField fld = rec.field(i);
        model->setData(model->index(i, 0), fld.name());
        model->setData(model->index(i, 1), fld.typeID() == -1
                ? QString(QVariant::typeToName(fld.type()))
                : QString("%1 (%2)").arg(QVariant::typeToName(fld.type())).arg(fld.typeID()));
        model->setData(model->index(i, 2), fld.length());
        model->setData(model->index(i, 3), fld.precision());
        model->setData(model->index(i, 4), fld.requiredStatus() == -1 ? QVariant("?")
                : QVariant(bool(fld.requiredStatus())));
        model->setData(model->index(i, 5), fld.isAutoValue());
        model->setData(model->index(i, 6), fld.defaultValue());
    }

    ui->table->setModel(model);
    ui->table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    updateActions();
}



void sqleditor::on_submitButton_clicked()
{
    exec();
    ui->sqlEdit->setFocus();
}

void sqleditor::on_clearButton_clicked()
{
    ui->sqlEdit->clear();
    ui->sqlEdit->setFocus();
}

void sqleditor::on_saveButton_clicked()
{
    bool ok;
    QString oldName = queryName;
    QString newName = queryName;

    if(isNew) {
        QString text = QInputDialog::getText(this, "Set Query Name",
                                             "New name for query", QLineEdit::Normal,
                                             queryName, &ok);

        if (!ok || text.isEmpty())
            return;

        newName = text;
    }

    emit saveQuery(newName, ui->sqlEdit->toPlainText(), oldName);

}

void sqleditor::_on_querySaved(QString name, QString oldName)
{
    if(oldName != this->queryName)
        return;

    this->queryName = name;
    this->isNew = false;
    this->isUnsaved = false;

    updateTitle();
}

void sqleditor::closeEvent(QCloseEvent *event)
{
    int ret = QMessageBox::Discard;

    if(isNew && this->ui->sqlEdit->toPlainText().isEmpty())
    {
    }
    else if(isUnsaved)
    {
        QMessageBox msgBox;
        msgBox.setText("The query has been modified.");
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        ret = msgBox.exec();
    }

    if(ret == QMessageBox::Cancel)
    {
        event->ignore();
        return;
    }
    else if(ret == QMessageBox::Save)
    {
        on_saveButton_clicked();
    }

    emit sqleditorClosed(event, queryName);

    event->accept();
}

void sqleditor::on_sqlEdit_textChanged()
{
    if(!isUnsaved) {
        isUnsaved = true;
        updateTitle();
    }

}
