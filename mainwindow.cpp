#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QDebug>
#include <QDir>
#include <QStringListModel>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    showDatabseNames();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showDatabseNames(){
    QDir dir(QDir::currentPath());
    qDebug() << dir.absolutePath();
    QStringList fillters;
    fillters << "*.sqlite";
    QStringList fileList = dir.entryList(fillters, QDir::Files);
    qDebug() << fileList;
    QStringListModel *model = new QStringListModel(this);
    model->setStringList(fileList);
    ui->dbList->setModel(model);
}

void MainWindow::on_dbNameButton_clicked()
{
    showDatabseNames();

    QString dbName = ui->dbNameEdit->text();
    QString fullDbName = dbName + ".sqlite";

    if (QFile::exists(fullDbName)) {
        QMessageBox::warning(this, "Creating Error", "Database with this name already exists!");
        qDebug() << "Warning: Database file exists";
        return;
    }

    if (QSqlDatabase::contains(dbName)) {
        dbInit = QSqlDatabase::database(dbName);
    } else {
        dbInit = QSqlDatabase::addDatabase("QSQLITE", dbName);
        dbInit.setDatabaseName(fullDbName);
        QMessageBox::information(this, "Database created", "Database created");
        qDebug() << "Info: Database created";
    }
    ui->dbNameEdit->setText("");
    if (!dbInit.isOpen()) {
        if (!dbInit.open()) {
            QMessageBox::critical(this, "Database Error", dbInit.lastError().text());
            qDebug() << "Failed to reopen database: " << dbInit.lastError().text();
            return;
        }
    }
    qDebug() << "Database successfully opened: " << dbInit.databaseName();

    QSqlQuery query(dbInit);
    QString createTableText = R"(
    CREATE TABLE IF NOT EXISTS users (
        id INT PRIMARY KEY,
        username VARCHAR(255) NOT NULL UNIQUE,
        email VARCHAR(255) NOT NULL UNIQUE,
        password VARCHAR(255) NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );
)";

    if (!query.exec(createTableText)) {
        QMessageBox::critical(this, "Query Error", query.lastError().text());
        qDebug() << "Query execution creating table failed: " << query.lastError().text();
        return;
    }

    QString addValuesToTableText = R"(
    INSERT OR IGNORE INTO users (id,username, email, password) VALUES
        (1, 'john_doe', 'john.doe@example.com', 'hashed_password_1'),
        (2, 'jane_smith', 'jane.smith@example.com', 'hashed_password_2'),
        (3, 'alice_jones', 'alice.jones@example.com', 'hashed_password_3');
)";
    if (!query.exec(addValuesToTableText)) {
        QMessageBox::critical(this, "Query Error", query.lastError().text());
        qDebug() << "Query execution adding data failed: " << query.lastError().text();
        return;
    }


showDatabseNames();
if (dbInit.isOpen()) {
    dbInit.close();
    qDebug() << "Database connection closed: " << dbInit.databaseName();
} else {
    qDebug() << "Database was already closed.";
}
}

void MainWindow::on_execQuery_clicked()
{
    if (!db.isValid()) {
        QMessageBox::critical(this, "Database Error", "Database is not initialized!");
        qDebug() << "Database is not initialized!";
        return;
    }

    if (!db.isOpen()) {
        if (!db.open()) {
            QMessageBox::critical(this, "Database Error", db.lastError().text());
            qDebug() << "Failed to reopen database: " << db.lastError().text();
            return;
        }
    }

    QString queryText = ui->queryEdit->text();
    qDebug() << "Executing query:" << queryText;

    QSqlQuery*  query=new QSqlQuery(db);

    if (!query->exec(queryText)) {
        QMessageBox::critical(this, "Query Error", query->lastError().text());
        qDebug() << "Query execution failed: " << query->lastError().text();
        return;
    }

    while (query->next()) {
        QString temp = query->value(0).toString();
        qDebug() << "Result:" << temp;
    }

    QSqlQueryModel * model=new QSqlQueryModel();
    model->setQuery(std::move(*query));
    ui->dbView->setModel(model);
    ui->dbView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::on_dbOpenButton_clicked()
{
    QModelIndexList selectedIndex = ui->dbList->selectionModel()->selectedIndexes();

    QString selectedDB = selectedIndex.first().data().toString();
    QFileInfo info(selectedDB);
    QString connectionName = info.baseName();

    ui->dbOpenedName->setText("OPENED DATABASE: " + selectedDB);
    qDebug() << "Selected string:" << selectedDB;
    qDebug() << "Connection name:" << connectionName;

    qDebug() << "Available drivers: " << QSqlDatabase::drivers();
    if (!QSqlDatabase::drivers().contains("QSQLITE")) {
        QMessageBox::critical(this, "Driver Error", "SQLite driver is not available!");
        return;
    }

    db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(selectedDB);

    if (!db.isOpen() && !db.open()) {
        QMessageBox::critical(this, "Database Error", db.lastError().text());
        return;
    }
    QStringList tables = db.tables();
    qDebug() << tables;
    QStringListModel *model = new QStringListModel(this);
    model->setStringList(tables);
    ui->tablesView->setModel(model);
}

void MainWindow::on_showColumnButton_clicked()
{
    QModelIndexList selectedIndex = ui->tablesView->selectionModel()->selectedIndexes();
    QString selectedTable = selectedIndex.first().data().toString();

    if (!db.isValid()) {
        QMessageBox::critical(this, "Database Error", "Database is not initialized!");
        qDebug() << "Database is not initialized!";
        return;
    }

    if (!db.isOpen()) {
        if (!db.open()) {
            QMessageBox::critical(this, "Database Error", db.lastError().text());
            qDebug() << "Failed to reopen database: " << db.lastError().text();
            return;
        }
    }

    QString queryText = "PRAGMA table_info(" + selectedTable + ");";
    qDebug() << "Executing query:" << queryText;

    QSqlQuery *query = new QSqlQuery(db);

    if (!query->exec(queryText)) {
        QMessageBox::critical(this, "Query Error", query->lastError().text());
        qDebug() << "Query execution failed: " << query->lastError().text();
        return;
    }

    while (query->next()) {
        QString temp = query->value(0).toString();
        qDebug() << "Result:" << temp;
    }

    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(std::move(*query));
    ui->columnsView->setModel(model);
    ui->columnsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}
