#ifndef LOGPARSER_LINEWRITERQTDB_H
#define LOGPARSER_LINEWRITERQTDB_H

#include "LineWriterInterface.h"

#include "qt/QtSql/QSql"
#include "qt/QtSql/qsqldatabase.h"
#include "qt/QtCore/QString"

class LineWriterQtDB : public LineWriterInterface {
  QSqlDatabase db_;

public:
  ~LineWriterQtDB();

  LineWriterQtDB(const QString& host,
                 const QString& user,
                 const QString& password,
                 const QString& database,
                 int port = 3306);

  bool WriteLine(const Line& line);
};

#endif // LOGPARSER_LINEWRITERQTDB_H
