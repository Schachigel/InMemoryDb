//#include <QtSql/QSqlDatabase>
//#include <QtSql/QSqlQuery>
#include <QtSql>
#include <iso646.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->leConnectionName->setText("imCon");
    ui->leDbName->setText ("file::memory:");
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pbGo_clicked()
{
    QString imCon =ui->leConnectionName->text (); // "imCon"
    QString imDbName =ui->leDbName->text();       // "file::memory:

    {
        // open in memory database
        QSqlDatabase imDb =QSqlDatabase::addDatabase ("QSQLITE", imCon);
        imDb.setConnectOptions ("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE");
        imDb.setDatabaseName (imDbName);
        if( not imDb.open())
            qCritical() << "failed to open inmemory DB " << imDb.lastError ();

        // open source db
        QSqlDatabase srcDb =QSqlDatabase::addDatabase ("QSQLITE", "source");
        srcDb.setConnectOptions ("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE");
        srcDb.setDatabaseName ("source.db");
        if( not srcDb.open ())
            qCritical() << "failed to open source db " << srcDb.lastError ();

        // copy data to im db
        QSqlQuery qSourceToMemory (srcDb);
        if( not qSourceToMemory.exec(QString("VACUUM INTO '%1'").arg(imDbName)))
            qCritical() << "failed to copy data to inmemory db " << imDbName << "\n" << qSourceToMemory.lastError ();

        // change data
        QSqlQuery qModifyImDb(imDb);
        if( not qModifyImDb.exec ("INSERT INTO t (f) VALUES ('BrandNewData')"))
            qCritical() << "failed to modify in memory db " << qModifyImDb.lastError ();

        // save imdb to disc
        QSqlQuery qMemoryToDisc(imDb);
        QFile::remove("dest.db");
        if( not qMemoryToDisc.exec(QString("VACUUM INTO '%1'").arg("dest.db")))
            qCritical() << "failed to vacuum into dest db " << qMemoryToDisc.lastError ();

        // cleanup
        srcDb.close ();
        srcDb.close();
    }
    QSqlDatabase::removeDatabase (imCon);
    QSqlDatabase::removeDatabase ("source");
}


void MainWindow::on_pbexit_2_clicked()
{
    {
        QSqlDatabase db =QSqlDatabase::addDatabase ("QSQLITE", "source");
        db.setDatabaseName ("source.db");
        if( not db.open ())
            qCritical() << "failed to open source db for creation";

        // WAL mode is not necessary// switch to WAL mode for not having to have shared cache mode
        QSqlQuery wal =db.exec ("PRAGMA journal_mode=WAL");
        wal.next ();
        if( wal.record ().value (0).toString() not_eq "wal")
            qCritical() << "failed to switch to WAL mode: " << wal.lastError ();

        QSqlQuery q(db);
        if( not q.exec ("CREATE TABLE IF NOT EXISTS t (f TEXT PRIMARY KEY)"))
            qCritical() << "failed to create table" << q.lastError ();
        if( not q.exec ("REPLACE INTO t (f) VALUES ('Hallo'), ('World'), ('Welcome'), ('Aliens')"))
            qCritical() << "failed to write data"<< q.lastError ();
        db.close ();
    }   // let QSqlQuery and QSqlDatabase run out of scope to close

    QSqlDatabase::removeDatabase ("source");
}

