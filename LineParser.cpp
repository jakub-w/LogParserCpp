#include "LineParser.h"

#include <algorithm>
#include <future>
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
std::vector<MessageType> LineParser::message_types_ =
    std::vector<MessageType>();
std::mutex LineParser::message_types_mutex_;

void LineParser::Initialize(const std::string& regex_file) {
  if (regex_file == regex_file_) return;

  message_types_.clear();

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
      message_types_.push_back({0,
			       it->name.GetString(),
			       regex});
    } catch (std::regex_error& e) {
      std::cout << "Regex for '" << it->name.GetString()
		<< "' could not load.\n";
    }
  }

  regex_file_ = regex_file;
}


std::unique_ptr<Line>
LineParser::ParseLine(const std::string& line,
                      std::vector<MessageType>& message_types) {
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
  if (message_types_.empty())
    throw std::logic_error(std::string("In function '") + __FUNCTION__ +
			   std::string("': Call LineParser::Initialize() first."));

  // std::shared_ptr<Line> output_line; // thread_local
  // std::shared_ptr<Line> last_line{new Line()}; // specialized thread

  // NEW STUFF

  ParsingThreadVars thread_vars;
  thread_vars.ifstrm.open(file_path);

  MapThreadVars map_thread_vars;

  const int THREAD_COUNT = 4; // TODO: make static or global or something

  std::array<std::future<bool>, THREAD_COUNT> parsing_thread_futures;
  for (uint i = 0; i < THREAD_COUNT; ++i) {
    parsing_thread_futures[i] = std::async(ParseLinesAsynchronously,
                                           &thread_vars, &map_thread_vars);
  }

  // here goes map-processing thread creation
  std::thread map_thread(ProcessMapVarsAsync, &map_thread_vars, line_writer);

  for (uint i = 0; i < THREAD_COUNT; ++i) {
    parsing_thread_futures[i].wait();
  }

  {
    std::lock_guard<std::mutex> lck(map_thread_vars.line_map_mutex);
    std::cout << "\nMap length: " << map_thread_vars.line_map.size();
  }
  std::cout << "\nParsing threads finished.\n";

  // notify the map-processing thread that it can exit
  map_thread_vars.is_parsing_done.store(true);
  map_thread.join();

  std::cout << "\nFinished\n";


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

bool LineParser::ParseLinesAsynchronously(ParsingThreadVars* vars,
                                          MapThreadVars* map_vars) {
  std::string line;
  size_t line_num = 0;

  // copy message_types_ to prevent conflicts with other threads
  std::vector<MessageType> message_types;
  {
    std::lock_guard<std::mutex> lck{message_types_mutex_};
    message_types = message_types_;
  }

  std::shared_ptr<Line> parsed_line;

  while (true) {
      // Read line from a file and increment read line count.
      {
        std::unique_lock<std::mutex> lck1{vars->ifstrm_mutex,
                                          std::defer_lock};
        std::unique_lock<std::mutex> lck2{map_vars->line_count_mutex,
                                          std::defer_lock};
        std::lock(lck1, lck2);

        // if nothing was read, break the thread loop
        if (! std::getline(vars->ifstrm, line)) break;
        line_num = ++(map_vars->line_count);
      }

      // std::stringstream ss;
      // ss << std::string("Thread ") << std::this_thread::get_id()
      //    << " Parsing line " << line_num << std::endl;
      // std::cout << ss.str();

      try {
        // Parse line
        parsed_line = ParseLine(line, message_types);

        // Put the name from a parsed line to a name set.
        {
          std::lock_guard<std::mutex> lck{vars->names_mutex};
          vars->names.insert(parsed_line->name);
        }
      } catch (std::exception& e) {
        std::cerr << "In function '" << __FUNCTION__
                  << "' while parsing line #" << line_num <<":\n  "
                  << e.what();
        // despite it couldn't parse, it'll add empty line to the map
        // so ProcessMapVarsAsync() wouldn't end up in the infinite loop
        parsed_line = nullptr;
      }

      // ss.str(std::string());
      // ss << std::string("Thread ") << std::this_thread::get_id()
      //    << " Parsed line " << line_num << std::endl;
      // std::cout << ss.str();

      // std::stringstream ss;
      // ss << std::to_string(line_num) + ' ' << std::this_thread::get_id()
      //    << ' ' + parsed_line->text + "\n";
      // std::cout << ss.str();

      // ss.str(std::string());
      // ss << std::string("Thread ") << std::this_thread::get_id()
      //    << " blocking line_map, saving line " << line_num << std::endl;
      // Put the line with its number to a year-heuristic map for more
      // (sequential, not async) processing.
      {
        std::lock_guard<std::mutex> lck{map_vars->line_map_mutex};
        map_vars->line_map.insert({std::move(line_num),
                                   std::move(parsed_line)});
        // std::cout << "Map length: " << map_vars->line_map.size() << std::endl;
        // std::cout << ss.str();
      }
  }

  return true;
}


void LineParser::ProcessMapVarsAsync(MapThreadVars* map_vars,
                                     LineWriterInterface* line_writer) {
  size_t max_map_len = 0;

  size_t current_line_num = 1;
  std::unordered_map<size_t, std::shared_ptr<Line>>::iterator
      current_line_it;
  std::shared_ptr<Line> current_line, previous_line{new Line()};
  std::vector<std::shared_ptr<Line>> undefined_type_lines;

  // thread loop
  while (true) {
    {
      std::lock_guard<std::mutex> lck(map_vars->line_map_mutex);
      // break thread loop if parsing threads finished their work
      // and line_map is empty
      if (map_vars->is_parsing_done.load() &&
          map_vars->line_map.empty())
        break;

      // check if the next line is in the map, and continue if not
      if (map_vars->line_map.end() ==
          (current_line_it = map_vars->line_map.find(current_line_num))) {
        continue;
      }
      max_map_len = std::max(max_map_len, map_vars->line_map.size());
    }

    // found line with number == current_line_num, so we can already
    // increment it for the next iteration
    ++current_line_num;
    // std::cout << "Waiting for line: " << current_line_num << std::endl;

    // Operating on lines that are already in the map is safe because
    // parsing threads only store them, nothing aside this function modifies
    // them.
    // current_line can be nullptr if it couldn't be parsed
    if (!(current_line = (*current_line_it).second)) {
      std::lock_guard<std::mutex> lck(map_vars->line_map_mutex);
      map_vars->line_map.erase(current_line_it);
      continue;
    }

    // Deduce year:
    // if the day is lower than before and month is not greater, or if month
    // is lower, we can assume that the year changed
    if ((current_line->date.tm_mday < previous_line->date.tm_mday &&
         current_line->date.tm_mon <= previous_line->date.tm_mon)
        || (current_line->date.tm_mon < previous_line->date.tm_mon)){
      current_line->date.tm_year = previous_line->date.tm_year + 1;
    } else {
      current_line->date.tm_year = previous_line->date.tm_year;
    }
    previous_line = current_line;

    // Move Undefined lines to undefined_type_lines
    // Write the rest with the line_writer
    if ("Undefined" == current_line->type)
      undefined_type_lines.push_back(current_line);
    else
      try {
        line_writer->WriteLine(*current_line);
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
      }

    // Remove current_line from the line_map to free memory.
    // current_line, as a std::shared_ptr, won't be destroyed
    {
      std::lock_guard<std::mutex> lck(map_vars->line_map_mutex);
      map_vars->line_map.erase(current_line_it);
    }
  }

  line_writer->Flush();
  std::cout << "Max map length: " << max_map_len << std::endl;
}
