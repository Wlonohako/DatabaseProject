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
    //zmień to na list widget chyba
    QStringListModel *model = new QStringListModel(this);
    model->setStringList(fileList);
    ui->dbList->setModel(model);
}

void MainWindow::on_dbNameButton_clicked()
{
    showDatabseNames();
    QString dbName = ui->dbNameEdit->text();
    dbName = dbName +  + ".sqlite";
    if (QSqlDatabase::contains(dbName)) {
        db = QSqlDatabase::database(dbName);
        QMessageBox::warning(this, "Creating Error", "Database with this name arleady exist!");
        qDebug() << "Warning: Database exist";

    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", dbName);
        db.setDatabaseName(dbName);
        qDebug() << "Info: Database created";
    }

    if (!db.isOpen()) {
        if (!db.open()) {
            QMessageBox::critical(this, "Database Error", db.lastError().text());
            qDebug() << "Failed to reopen database: " << db.lastError().text();
            return;
        }
    }
    qDebug() << "Database successfully opened: " << db.databaseName();



    QSqlQuery query(db);
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
