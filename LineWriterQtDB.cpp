#include "LineWriterQtDB.h"

#include <iostream>

#include "qt/QtSql/QSql"
// #include "qt/QtSql/QSqlQuery"
#include "qt/QtSql/qsqlquery.h"
// #include "qt/QtCore/QVariant"
#include "qt/QtCore/qvariant.h"
#include "qt/QtCore/qdatetime.h"

LineWriterQtDB::~LineWriterQtDB() {
  db_.close();
}

LineWriterQtDB::LineWriterQtDB(const QString& host,
                               const QString& user,
                               const QString& password,
                               const QString& database,
                               int port) {
  db_ = QSqlDatabase::addDatabase("QMYSQL");
  db_.setHostName(host);
  db_.setDatabaseName(database);
  db_.setUserName(user);
  db_.setPassword(password);
  bool ok = db_.open();
}

bool LineWriterQtDB::WriteLine(const Line &line) {
  QSqlQuery query;
  query.prepare("INSERT INTO .lines(date, type, text, name) "
                "VALUES(:date,:type,:text,:name)");
  query.bindValue(":type", line.type.data());
  query.bindValue(":text", line.text.data());
  query.bindValue(":name", line.name.data());

  QDateTime date(QDate(line.date.tm_year,
                       line.date.tm_mon,
                       line.date.tm_mday),
                 QTime(line.date.tm_hour,
                       line.date.tm_min,
                       line.date.tm_sec));
  query.bindValue(":date", date);
  // std::chrono::
  query.exec();

  return true;
}
