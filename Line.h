#ifndef LOGPARSER_LINE_H
#define LOGPARSER_LINE_H

#include <ctime>
#include "MessageType.h"

struct Line {
  std::tm date = {0};
  std::string text;
  std::string name;
  /* MessageType* type; */
  std::string type;
};

#endif // LOGPARSER_LINE_H
