#ifndef LOGPARSER_LINEWRITERDATABASE_H
#define LOGPARSER_LINEWRITERDATABASE_H

#include "LineWriterInterface.h"

#include "mysql/mysql.h"

class LineWriterDatabase : public LineWriterInterface {
  const static size_t TYPE_MAX_LENGTH = 32;
  const static size_t TEXT_MAX_LENGTH = 65535;
  const static size_t NAME_MAX_LENGTH = 64;

  MYSQL* mysql_ = nullptr;
  MYSQL_STMT* stmt_ = nullptr;
  MYSQL_BIND bind_[4] = {{0}};
  std::string query_;

  MYSQL_TIME date_ = {0};
  size_t type_length_;
  size_t text_length_;
  size_t name_length_;
  my_bool is_name_empty_;

  bool was_writeline_called_ = false;
  bool was_flush_called_ = false;

  // thanks to this constructor, we can throw an exception in the second one
  // and it calls the destructor, because object is already created by this
  // emtpy one
  LineWriterDatabase() noexcept;

public:
  ~LineWriterDatabase();

  LineWriterDatabase(const std::string& host,
                     const std::string& user,
                     const std::string& password,
                     const std::string& database,
                     const std::string& table,
                     int port = 3306);

  // After finished operation, Flush() must be called to commit changes to the
  // database. Without Flush() writer will have no effect at all.
  bool WriteLine(const Line& line);

  bool Flush();
};

#endif // LOGPARSER_LINEWRITERDATABASE_H
