#ifndef LOGPARSER_LINEPARSER_H
#define LOGPARSER_LINEPARSER_H

#include <memory>
#include <vector>

#include "Line.h"
#include "LineWriterInterface.h"
#include "MessageType.h"

class LineParser {
  LineParser(){} // disable constructor for a static class

  static std::string regex_file_;

public:
  static std::vector<MessageType> message_types;

  // Must be called before anything else
  static void Initialize(const std::string& regex_file);
  static std::unique_ptr<Line> ParseLine(const std::string& line);

  // Parses all of the lines in a file_path and writes them to a storage using
  // line_writer.
  // Warning: Lines of type Undefined and Emote will be written last, so if
  //          saved to one file or a database, order will be incorrect.
  //          The lines should be reordered according to date afterwards,
  //          if necessary.
  static bool ParseFile(const std::string& file_path,
                        LineWriterInterface* line_writer);
};

#endif // LOGPARSER_LINEPARSER_H
