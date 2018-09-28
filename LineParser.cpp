#include "LineParser.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/reader.h"

#include "LineWriterFile.h"

std::string LineParser::regex_file_ = std::string();
std::vector<MessageType> LineParser::message_types = std::vector<MessageType>();

void LineParser::Initialize(const std::string& regex_file) {
  if (regex_file == regex_file_) return;

  message_types.clear();

  // open regex file and load it into rapidjson::Document document.
  std::ifstream regex_stream(regex_file);
  rapidjson::IStreamWrapper regex_stream_wrap(regex_stream);

  rapidjson::Document regex_doc;
  regex_doc.ParseStream(regex_stream_wrap);

  assert(regex_doc.IsObject());

  // load read regexes into Regex multiset
  for(auto it = regex_doc["Regex"].MemberBegin();
      it != regex_doc["Regex"].MemberEnd();
      ++it) {
    std::regex regex;
    try {
      regex.assign(it->value.GetString());
    } catch (std::regex_error& e) {
      std::cout << "Regex for '" << it->name.GetString()
    		<< "' could not load.\n";
    }

    try {
      message_types.push_back({0,
			       it->name.GetString(),
			       regex});
    } catch (std::regex_error& e) {
      std::cout << "Regex for '" << it->name.GetString()
		<< "' could not load.\n";
    }
  }

  regex_file_ = regex_file;
}


std::unique_ptr<Line> LineParser::ParseLine(const std::string& line) {
  if (message_types.empty())
    throw std::logic_error(std::string("In function '") + __FUNCTION__ +
			   std::string("': Call LineParser::Initialize() first."));

  // check if starting with a date
  if (false == std::isdigit(line[0]))
    throw std::runtime_error(std::string("WARNING:  Line: '") + line +
                             "' doesn't start with a date.\n");

  // Parse date from a line; using ctime because of a bug in c++ version
  auto date = std::tm{};
  auto conversion_result = strptime(line.substr(0, line.find(".")).c_str(),
				    "%m/%d %H:%M:%S", &date);
  if (!conversion_result)
    throw std::runtime_error(
        std::string("WARNING:  Date in a line failed to parse.\n  ") + line);

  // Derive line type based on a regex
  auto text = line.substr(line.find("  ")+2);
  // erase all trailing whitespaces
  text.erase(std::find_if_not(text.rbegin(), text.rend(),
                              [](int ch){ return std::isspace(ch); })
             .base(), text.end());
  std::string name;
  std::string type;
  std::smatch matches;
  for (auto& message_type : message_types) {
    if (std::regex_search(text, matches, message_type.regex)) {
      ++message_type.weight;
      type = message_type.type;
      break;
    }
  }

  // Names are stored as regex groups and always only one has a name in it.
  // The rest is empty.
  for (std::smatch::size_type i = 1; i < matches.size(); ++i)
        name += matches[i];

  // If line's type was found, it's weight increased, so we sort them
  // for faster processing in the future.
  if (false == type.empty()) {
    std::sort(message_types.begin(), message_types.end());
  }
  else type = "Undefined";

  return std::unique_ptr<Line>(new Line{std::move(date),
                                        std::move(text),
                                        std::move(name),
                                        std::move(type)});
}

bool LineParser::ParseFile(const std::string& file_path,
                           LineWriterInterface* line_writer) {
  if (message_types.empty())
    throw std::logic_error(std::string("In function '") + __FUNCTION__ +
			   std::string("': Call LineParser::Initialize() first."));

  // std::shared_ptr<Line> output_line; // thread_local
  // std::shared_ptr<Line> last_line{new Line()}; // specialized thread

  // NEW STUFF

  ParsingThreadVars thread_vars;
  thread_vars.ifstrm.open(file_path);

  MapThreadVars map_thread_vars;

  std::thread t1(ParseNextLine, &thread_vars, &map_thread_vars);
  // END OF NEW STUFF

  // for (std::string line; std::getline(filestream, line);) {
  //   output_line = LineParser::ParseLine(line);
  //   if (nullptr == output_line) continue;

  //   // names.insert(output_line->name);

  //   // if day is lower than before and month is not greater, or if month
  //   // is lower, we can assume that the year changed
  //   if ((output_line->date.tm_mday < last_line->date.tm_mday &&
  //        output_line->date.tm_mon <= last_line->date.tm_mon)
  //       || (output_line->date.tm_mon < last_line->date.tm_mon)){
  //     output_line->date.tm_year = last_line->date.tm_year + 1;
  //   } else {
  //     output_line->date.tm_year = last_line->date.tm_year;
  //   }
  //   last_line = output_line;

  //   if ("Undefined" == output_line->type) {
  //     undefined_type_lines.push_back(std::move(output_line));
  //   } else {
  //     try {
  //       line_writer->WriteLine(*output_line);
  //     } catch (std::exception& e) {
  //       std::cerr << e.what() << std::endl;
  //     }
  //   }
  // }
  // names.insert("You");
  // // remove magic strings that are not names
  // names.erase("%s");
  // names.erase("");

  // check if name existing in names set is the first word of line->text
  // if so, it is an emote
  // std::string name;
  // for (auto& line : undefined_type_lines) {
  //   name = line->text.substr(0, line->text.find(' '));
  //   if (names.end() != names.find(name)) {
  //     line->type = "Emote";
  //     line->name = name;
  //   }
  //   try {
  //     line_writer->WriteLine(*line);
  //   } catch (std::exception& e) {
  //       std::cerr << e.what() << std::endl;
  //   }
  // }

  // line_writer->Flush();
  return true;
}

bool LineParser::ParseNextLine(ParsingThreadVars* vars,
                               MapThreadVars* map_vars) {
  std::string line;
  size_t line_num;

  try {
  // Read line from a file and increment read line count.
    {
      std::unique_lock<std::mutex> lck1{vars->ifstrm_mutex,
                                        std::defer_lock};
      std::unique_lock<std::mutex> lck2{map_vars->line_count_mutex,
                                        std::defer_lock};
      std::lock(lck1, lck2);

      std::getline(vars->ifstrm, line);
      line_num = ++(map_vars->line_count);
    }

    // Parse line
    std::shared_ptr<Line> parsed_line = ParseLine(line);

    // Put name from parsed line to a name set.
    {
      std::lock_guard<std::mutex> lck{vars->names_mutex};
      vars->names.insert(parsed_line->name);
    }

    // Put the line with its number to a year-heuristic map for more
    // (sequential, not async) processing.
    {
      std::lock_guard<std::mutex> lck{map_vars->line_map_mutex};
      map_vars->line_map.insert({std::move(line_num),
                                 std::move(parsed_line)});
    }

    // Notify a thread that continues parsing lines that there is new line
    // in the map
    map_vars->map_cv.notify_all();
  }
  catch (std::exception& e) {
    std::cerr << "In function '" << __PRETTY_FUNCTION__ << "':\n  "
              << e.what();
    return false;
  }

  return true;
}
