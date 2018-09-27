#include "LineWriterDatabase.h"

#include <iomanip>
#include <iostream>
#include <chrono>

LineWriterDatabase::LineWriterDatabase() noexcept
    : mysql_(nullptr),
      stmt_(nullptr) {}

LineWriterDatabase::LineWriterDatabase(const std::string& host,
                                       const std::string& user,
                                       const std::string& password,
                                       const std::string& database,
                                       const std::string& table,
                                       int port) : LineWriterDatabase()
    // : host_(host),
    //   user_(user),
    //   password_(password),
    //   database_(database),
    //   port_(port)
{
  query_ = std::string("INSERT INTO .") + table +
      "(date, type, text, name) VALUE(?,?,?,?)";

  if (0 != mysql_library_init(0, NULL, NULL))
    throw std::runtime_error("Could not initialize MySQL client library.");

  if(NULL == (mysql_ = mysql_init(NULL))) {
    throw std::runtime_error(
        "Insufficient memory to initialize MySQL handler.");
  }
  if (!mysql_real_connect(mysql_, host.c_str(), user.c_str(),
                          password.c_str(), database.c_str(),
                          port, NULL, 0)) {
    std::string err_msg = mysql_error(mysql_);
    throw std::runtime_error(err_msg);
  }

  // Initialize prepared statement
  stmt_ = mysql_stmt_init(mysql_);
  if (!stmt_)
    throw std::runtime_error("mysql_stmt_init(), out of memory.");

  if (0 != mysql_stmt_prepare(stmt_, query_.c_str(), query_.size()))
    throw std::runtime_error(
        std::string("mysql_stmt_prepare(), INSERT failed\n") +
        mysql_stmt_error(stmt_));

  // // Initialize bind
  // // date
  // bind_[0].buffer_type = MYSQL_TYPE_DATETIME;
  // bind_[0].buffer = (char*) &date_;

  // // // type
  // bind_[1].buffer_type = MYSQL_TYPE_STRING;
  // bind_[1].buffer = (char*) type_buffer_.data();
  // bind_[1].buffer_length = TYPE_MAX_LENGTH;
  // bind_[1].is_null = 0;
  // bind_[1].length = &type_length_;
  // // // text
  // bind_[2].buffer_type = MYSQL_TYPE_STRING;
  // bind_[2].buffer = (char*) text_buffer_.data();
  // bind_[2].buffer_length = TEXT_MAX_LENGTH;
  // bind_[2].is_null = 0;
  // bind_[2].length = &text_length_;
  // // // name
  // bind_[3].buffer_type = MYSQL_TYPE_STRING;
  // bind_[3].buffer = (char*) name_buffer_.data();
  // bind_[3].buffer_length = NAME_MAX_LENGTH;
  // bind_[3].length = &name_length_;
  // bind_[3].is_null = &is_name_empty_;
  mysql_autocommit(mysql_, false);
}

LineWriterDatabase::~LineWriterDatabase() {
  if (stmt_) mysql_stmt_close(stmt_);
  if (mysql_) mysql_close(mysql_);
  mysql_library_end();

  if (true == was_writeline_called_ &&
      false == was_flush_called_)
    std::cerr << "Warning: " << __PRETTY_FUNCTION__
              << ":\n  Never called Flush(). Database unchanged.\n";
}

bool LineWriterDatabase::WriteLine(const Line &line) {
  date_.year = line.date.tm_year; // tm_year is 1900-based
  date_.month = line.date.tm_mon + 1; // tm_mon is 0-based
  date_.day = line.date.tm_mday;
  date_.hour = line.date.tm_hour;
  date_.minute = line.date.tm_min;
  date_.second = line.date.tm_sec;

  // char* type = (char*)line.type.c_str();
  type_length_ = line.type.length();
  if (type_length_ > TYPE_MAX_LENGTH)
    throw std::length_error("Type: \n  '" + line.type +
                            "'\nis longer than 32 characters.");
  // char* text = (char*)line.text.c_str();
  text_length_ = line.text.length();
  if (text_length_ > TEXT_MAX_LENGTH)
    throw std::length_error("Text: \n  '" + line.text +
                            "'\nis longer than 256 characters.");
  // char* name = (char*)line.name.c_str();
  name_length_ = line.name.length();
  if (name_length_ > NAME_MAX_LENGTH)
    throw std::length_error("Name \n  '" + line.name +
                            "'\nis longer than 64 characters.");

  // date
  bind_[0].buffer_type = MYSQL_TYPE_DATETIME;
  bind_[0].buffer = (char*) &date_;

  // type
  bind_[1].buffer_type = MYSQL_TYPE_STRING;
  bind_[1].buffer = (char*) line.type.data();
  bind_[1].buffer_length = TYPE_MAX_LENGTH;
  bind_[1].is_null = 0;
  bind_[1].length = &type_length_;

  // text
  bind_[2].buffer_type = MYSQL_TYPE_STRING;
  bind_[2].buffer = (char*) line.text.data();
  bind_[2].buffer_length = TEXT_MAX_LENGTH;
  bind_[2].is_null = 0;
  bind_[2].length = &text_length_;

  // name
  bind_[3].buffer_type = MYSQL_TYPE_STRING;
  bind_[3].buffer = (char*) line.name.data();
  bind_[3].buffer_length = NAME_MAX_LENGTH;
  bind_[3].length = &name_length_;
  is_name_empty_ = (line.name.empty() ? 1 : 0);
  bind_[3].is_null = &is_name_empty_;

  if (0 != mysql_stmt_bind_param(stmt_, bind_))
    throw std::runtime_error(
        std::string("mysql_stmt_bind_param() failed\n") +
        mysql_stmt_error(stmt_));

  // auto before = std::chrono::high_resolution_clock::now();
  if (0 != mysql_stmt_execute(stmt_))
    throw std::runtime_error(
        std::string("mysql_stmt_execute() failed\n") +
        mysql_stmt_error(stmt_));
  // auto after = std::chrono::high_resolution_clock::now();
  // std::cout << std::chrono::duration<double>(after - before).count() << "s\n";

  was_writeline_called_ = true;
  return true;
}

bool LineWriterDatabase::Flush() {
  if (!mysql_)
    throw std::logic_error("Flush()\nDatabase not initialized.");
  if (0 != mysql_commit(mysql_)) {
      throw std::runtime_error(
        std::string("mysql_commit() failed\n") +
        mysql_error(mysql_));
    }
  was_flush_called_ = true;
  return true;
}
