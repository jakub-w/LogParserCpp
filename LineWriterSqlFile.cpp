#include "LineWriterSqlFile.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

std::string LineWriterSqlFile::EscapeSingleQuotes(const std::string& str) {
  std::string result{str};

  std::string::size_type curr_pos;
  std::string::size_type last_pos = 0;
  while (result.npos != (curr_pos = result.find('\\', last_pos))) {
    result.insert(curr_pos, "\\");
    last_pos = curr_pos + 2;
  }
  last_pos = 0;
  while (result.npos != (curr_pos = result.find('\'', last_pos))) {
    result.insert(curr_pos, "\\");
    last_pos = curr_pos + 2;
  }

  return result;
}

LineWriterSqlFile::~LineWriterSqlFile(){
  if (mysql_) mysql_close(mysql_);
  mysql_library_end();
};

LineWriterSqlFile::LineWriterSqlFile() : mysql_{nullptr} {}

LineWriterSqlFile::LineWriterSqlFile(const std::string& host,
                                     const std::string& user,
                                     const std::string& password,
                                     const std::string& database,
                                     const std::string& table,
                                     int port)
    : LineWriterSqlFile() {
  table_ = database + '.' + table;

  // initialize mysql
  if (0 != mysql_library_init(0, nullptr, nullptr))
    throw std::runtime_error("Could not initialize MySQL client library.");

  if(nullptr == (mysql_ = mysql_init(nullptr))) {
    throw std::runtime_error(
        "Insufficient memory to initialize MySQL handler.");
  }
  // connect to the database
  if (!mysql_real_connect(mysql_, host.c_str(), user.c_str(),
                          password.c_str(), database.c_str(),
                          port, nullptr, 0)) {
    std::string err_msg = mysql_error(mysql_);
    throw std::runtime_error(err_msg);
  }
}

bool LineWriterSqlFile::WriteLine(const Line& line) {
  if (line.type.length() > TYPE_MAX_LENGTH)
    throw std::length_error("Type: \n  '" + line.type + "'\nis longer than " +
                            std::to_string(TYPE_MAX_LENGTH) + " characters.");

  auto text = EscapeSingleQuotes(line.text);
  if (text.length() > TEXT_MAX_LENGTH)
    throw std::length_error("Text: \n  '" + text + "'\nis longer than " +
                            std::to_string(TEXT_MAX_LENGTH) + " characters.");

  auto name = EscapeSingleQuotes(line.name);
  if (name.length() > NAME_MAX_LENGTH)
    throw std::length_error("Name: \n  '" + name + "'\nis longer than " +
                            std::to_string(NAME_MAX_LENGTH) + " characters.");

  if (!mysql_query(mysql_, "INSERT INTO test (text) value('tralala')")) {
    std::cerr << mysql_error(mysql_) << std::endl;
  }

  // stream_ << std::put_time(&line.date, "'%Y-%m-%d %H:%M:%S'") << ",'"
  //         << line.type << "','" << text << "',"
  //         << (line.name.empty() ? "NULL" : '\'' + name + '\'') << "\n";

  return true;
}

// functions for use with mysql_set_local_infile_handler()
int LineWriterSqlFile::local_infile_init(void **ptr,
                                         const char *filename,
                                         void *userdata) {
  int* error_ptr = new int(0);
  *ptr = (void*)error_ptr;

  filestream_.open(filename);
  if(!filestream_.is_open()) {
    *error_ptr = 1; // couldn't open a file
  return 1;
  }

  return 0;
}

int LineWriterSqlFile::local_infile_read(void *ptr,
                                         char *buf,
                                         unsigned int buf_len) {
  int result;

  try {
    result = filestream_.readsome(buf, buf_len);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    *(int*)ptr = 2; // couldn't read from file
    return -1;
  }

  return result;
}

void LineWriterSqlFile::local_infile_end(void *ptr) {
  filestream_.close();
  delete ((int*)ptr);
}

int LineWriterSqlFile::local_infile_error(void *ptr,
                       char *error_msg,
                       unsigned int error_msg_len) {
  int error_num = *(int*)ptr;

  std::string error_str;
  switch (error_num){
    case 1:
      error_str = "Couldn't open a file.";
      break;
    case 2:
      error_str = "Couln't read to buffer from a file.";
      break;
    default:
      error_str = "Unknown error.";
  }
  uint end = std::min((uint)error_str.length(), error_msg_len-2);
  for (uint i = 0; i < end; ++i) {
    error_msg[i] = error_str[i];
  }
  error_msg[end] = '\0';

  return error_num;
}
