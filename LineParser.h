#ifndef LOGPARSER_LINEPARSER_H
#define LOGPARSER_LINEPARSER_H

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Line.h"
#include "LineWriterInterface.h"
#include "MessageType.h"

class LineParser {
  struct ParsingThreadVars {
    std::ifstream ifstrm;
    std::mutex ifstrm_mutex;

    std::unordered_set<std::string> names;
    std::mutex names_mutex;
  };

  struct MapThreadVars {
    size_t line_count = 0;
    std::mutex line_count_mutex;

    std::unordered_map<size_t, std::shared_ptr<Line>> line_map;
    std::mutex line_map_mutex;

    std::atomic<bool> is_parsing_done = false;
  };

  static std::string regex_file_;

  // it parses raw lines and saves them into map_vars.line_map for more
  // processing with ProcessMapVars().
  static bool ParseLinesAsynchronously(ParsingThreadVars* vars,
                                       MapThreadVars* map_vars);

  // 1. deduces if year in consecutive lines has changed
  // 2. moves Undefined lines to another set for Emote processing
  // 3. pushes all remaining lines to a LineWriter
  static void ProcessMapVarsAsync(MapThreadVars* map_vars,
                                  LineWriterInterface* line_writer);

  static std::mutex message_types_mutex_;
  static std::vector<MessageType> message_types_;
public:

  LineParser() = delete; // disable constructor for a static class

  // Must be called before anything else
  static void Initialize(const std::string& regex_file);

  static std::unique_ptr<Line>
  ParseLine(const std::string& line,
            std::vector<MessageType>& message_types);

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
