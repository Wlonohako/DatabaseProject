#include "mainwindow.h"
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QStringListModel>
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    showDatabseNames();
    qDebug() << "MainWindow initialized.";
}

MainWindow::~MainWindow()
{
    delete ui;
    qDebug() << "MainWindow destroyed.";
}

void MainWindow::showDatabseNames()
{
    QDir dir(QDir::currentPath());
    qDebug() << "Current directory: " << dir.absolutePath();

    QStringList filters;
    filters << "*.sqlite";
    QStringList fileList = dir.entryList(filters, QDir::Files);
    qDebug() << "Database files found: " << fileList;

    QStringListModel *model = new QStringListModel(this);
    model->setStringList(fileList);
    ui->dbList->setModel(model);
}

void MainWindow::on_dbNameButton_clicked()
{
    showDatabseNames();

    QString dbName = ui->dbNameEdit->text();
    if (dbName.isEmpty()) {
        QMessageBox::warning(this, "Creating Error", "Database name can't be empty");
        qDebug() << "Warning: Database name can't be empty";
        return;
    }
    QString fullDbName = dbName + ".sqlite";

    if (QFile::exists(fullDbName)) {
        QMessageBox::warning(this, "Creating Error", "Database with this name already exists!");
        qDebug() << "Warning: Database file already exists: " << fullDbName;
        return;
    }

    if (QSqlDatabase::contains(dbName)) {
        dbInit = QSqlDatabase::database(dbName);
        qDebug() << "Using existing database: " << dbName;
    } else {
        dbInit = QSqlDatabase::addDatabase("QSQLITE", dbName);
        dbInit.setDatabaseName(fullDbName);
        QMessageBox::information(this, "Database Created", "Database created successfully!");
        qDebug() << "Info: New database created: " << fullDbName;
    }

    ui->dbNameEdit->clear();

    if (!dbInit.isOpen()) {
        if (!dbInit.open()) {
            QMessageBox::critical(this, "Database Error", dbInit.lastError().text());
            qDebug() << "Error: Failed to open database: " << dbInit.lastError().text();
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
        qDebug() << "Error: Table creation failed: " << query.lastError().text();
        return;
    }

    QString addValuesToTableText = R"(
        INSERT OR IGNORE INTO users (id, username, email, password) VALUES
        (1, 'john_doe', 'john.doe@example.com', 'hashed_password_1'),
        (2, 'jane_smith', 'jane.smith@example.com', 'hashed_password_2'),
        (3, 'alice_jones', 'alice.jones@example.com', 'hashed_password_3');
    )";

    if (!query.exec(addValuesToTableText)) {
        QMessageBox::critical(this, "Query Error", query.lastError().text());
        qDebug() << "Error: Inserting data into users table failed: " << query.lastError().text();
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
        qDebug() << "Error: Database is not initialized.";
        return;
    }

    if (!db.isOpen()) {
        if (!db.open()) {
            QMessageBox::critical(this, "Database Error", db.lastError().text());
            qDebug() << "Error: Failed to open database: " << db.lastError().text();
            return;
        }
    }

    QString queryText = ui->queryEdit->text();
    qDebug() << "Executing query: " << queryText;

    QSqlQuery *query = new QSqlQuery(db);

    if (!query->exec(queryText)) {
        QMessageBox::critical(this, "Query Error", query->lastError().text());
        qDebug() << "Error: Query execution failed: " << query->lastError().text();
        return;
    }

    while (query->next()) {
        QString temp = query->value(0).toString();
        qDebug() << "Query result: " << temp;
    }

    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(std::move(*query));
    ui->dbView->setModel(model);
    ui->dbView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    on_dbOpenButton_clicked();
}

void MainWindow::on_dbOpenButton_clicked()
{
    QItemSelectionModel *selectionModel = ui->dbList->selectionModel();
    if (!selectionModel) {
        QMessageBox::warning(this, "Opening Error", "Table not selected");
        qDebug() << "Warning: Table not selected";
        return;
    }
    QModelIndexList selectedIndex = ui->dbList->selectionModel()->selectedIndexes();
    if (selectedIndex.isEmpty()) {
        QMessageBox::warning(this, "Opening Error", "Database not selected");
        qDebug() << "Warning: Database not selected";
        return;
    }
    QString selectedDB = selectedIndex.first().data().toString();
    QFileInfo info(selectedDB);
    QString connectionName = info.baseName();

    ui->dbOpenedName->setText("OPENED DATABASE: " + selectedDB);
    qDebug() << "Selected database: " << selectedDB;
    qDebug() << "Connection name: " << connectionName;

    db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(selectedDB);

    if (!db.isOpen() && !db.open()) {
        QMessageBox::critical(this, "Database Error", db.lastError().text());
        return;
    }

    QStringList tables = db.tables();
    qDebug() << "Tables in database: " << tables;

    QStringListModel *model = new QStringListModel(this);
    model->setStringList(tables);
    ui->tablesView->setModel(model);
}

void MainWindow::on_showColumnButton_clicked()
{
    QItemSelectionModel *selectionModel = ui->tablesView->selectionModel();
    if (!selectionModel) {
        QMessageBox::warning(this, "Opening Error", "Table not selected");
        qDebug() << "Warning: Table not selected";
        return;
    }
    QModelIndexList selectedIndex = ui->tablesView->selectionModel()->selectedIndexes();
    qDebug() << "Selected table index list: " << selectedIndex;
    if (selectedIndex.isEmpty()) {
        QMessageBox::warning(this, "Opening Error", "Table not selected");
        qDebug() << "Warning: Table not selected";
        return;
    }
    QString selectedTable = selectedIndex.first().data().toString();
    if (!db.isValid()) {
        QMessageBox::critical(this, "Database Error", "Database is not initialized!");
        qDebug() << "Error: Database is not initialized.";
        return;
    }

    if (!db.isOpen()) {
        if (!db.open()) {
            QMessageBox::critical(this, "Database Error", db.lastError().text());
            qDebug() << "Error: Failed to open database: " << db.lastError().text();
            return;
        }
    }

    QString queryText = "PRAGMA table_info(" + selectedTable + ");";
    qDebug() << "Executing query: " << queryText;

    QSqlQuery *query = new QSqlQuery(db);

    if (!query->exec(queryText)) {
        QMessageBox::critical(this, "Query Error", query->lastError().text());
        qDebug() << "Error: Query execution failed: " << query->lastError().text();
        return;
    }

    while (query->next()) {
        QString temp = query->value(0).toString();
        qDebug() << "Column info: " << temp;
    }

    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(std::move(*query));
    ui->columnsView->setModel(model);
    ui->columnsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}
