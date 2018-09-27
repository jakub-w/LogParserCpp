#ifndef LOGPARSER_LINEWRITERFILE_H
#define LOGPARSER_LINEWRITERFILE_H

#include "LineWriterInterface.h"

#include <fstream>
#include <map>

class LineWriterFile : public LineWriterInterface {
  std::map<std::string, std::ofstream> output_streams_;

public:
  bool WriteLine(const Line& line);
};

#endif // LOGPARSER_LINEWRITERFILE_H
