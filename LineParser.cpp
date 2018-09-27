#include "LineParser.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
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

  // TODO: Change it to throw, because someone could dereference nullptr
  //       and not know why there's segmentation fault
  // check if starting with date
  if (false == std::isdigit(line[0])) {
    std::cout << "WARNING:  Line: '" << line
	      << "' doesn't start with a date.\n";
    return nullptr;
  }

  // Parse date from a line; using ctime because of a bug in c++ version
  auto date = std::tm{};
  auto conversion_result = strptime(line.substr(0, line.find(".")).c_str(),
				    "%m/%d %H:%M:%S", &date);
  if (!conversion_result) {
    std::cout << "WARNING:  Date in a line failed to parse.\n"
	      << "  " << line;
    return nullptr;
  }

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

  std::ifstream filestream(file_path);
  std::shared_ptr<Line> output_line;
  std::vector<std::shared_ptr<Line>> undefined_type_lines;
  std::set<std::string> names;
  std::shared_ptr<Line> last_line{new Line()};
  // last_line->date = {0};
  for (std::string line; std::getline(filestream, line);) {
    output_line = LineParser::ParseLine(line);
    if (nullptr == output_line) continue;

    names.insert(output_line->name);

    // if day is lower than before and month is not greater, or if month
    // is lower, we can assume that the year changed
    if ((output_line->date.tm_mday < last_line->date.tm_mday &&
         output_line->date.tm_mon <= last_line->date.tm_mon)
        || (output_line->date.tm_mon < last_line->date.tm_mon)){
      output_line->date.tm_year = last_line->date.tm_year + 1;
    } else {
      output_line->date.tm_year = last_line->date.tm_year;
    }
    last_line = output_line;

    if ("Undefined" == output_line->type) {
      undefined_type_lines.push_back(std::move(output_line));
    } else {
      try {
        line_writer->WriteLine(*output_line);
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
      }
    }
  }
  names.insert("You");
  // remove magic strings that are not names
  names.erase("%s");
  names.erase("");

  // check if name existing in names set is the first word of line->text
  // if so, it is an emote
  std::string name;
  for (auto& line : undefined_type_lines) {
    name = line->text.substr(0, line->text.find(' '));
    if (names.end() != names.find(name)) {
      line->type = "Emote";
      line->name = name;
    }
    try {
      line_writer->WriteLine(*line);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
  }

  line_writer->Flush();
  return true;
}
