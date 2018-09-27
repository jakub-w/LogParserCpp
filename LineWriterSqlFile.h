#ifndef LOGPARSE_LINEWRITERSQLFILE_H
#define LOGPARSE_LINEWRITERSQLFILE_H

#include "LineWriterInterface.h"

#include <fstream>

#include "mysql/mysql.h"

class LineWriterSqlFile : public LineWriterInterface {
  const static size_t TYPE_MAX_LENGTH = 32;
  const static size_t TEXT_MAX_LENGTH = 65535;
  const static size_t NAME_MAX_LENGTH = 64;

  std::string table_;
  std::ifstream filestream_;

  MYSQL* mysql_;

  std::string EscapeSingleQuotes(const std::string& str);

  // functions for use with mysql_set_local_infile_handler()
  int local_infile_init(void **ptr, const char *filename, void *userdata);
  int local_infile_read(void *ptr, char *buf, unsigned int buf_len);
  void local_infile_end(void *ptr);
  int local_infile_error(void *ptr,
                         char *error_msg,
                         unsigned int error_msg_len);



  // thanks to this constructor, we can throw an exception in the second one
  // and it calls the destructor, because object is already created by this
  // emtpy one
  LineWriterSqlFile();

public:
  ~LineWriterSqlFile();

  LineWriterSqlFile(const std::string& host,
                    const std::string& user,
                    const std::string& password,
                    const std::string& database,
                    const std::string& table,
                    int port = 3306);

  bool WriteLine(const Line& line);
};

#endif // LOGPARSE_LINEWRITERSQLFILE_H
