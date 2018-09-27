#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <set>
#include <chrono>

#include "LineParser.h"
#include "LineWriterDatabase.h"
#include "LineWriterFile.h"
#include "LineWriterCout.h"
// #include "LineWriterQtDB.h"
// #include "LineWriterSqlFile.h"

// NOTE: For reference
// NOTE: For names, we should create weighted vector also
void Temp() {
  std::regex re("(^\\[\\d*\\. .*\\] )");
  std::regex re_name("\\[[\\w\\s.]*\\] (\\w+)");

  std::ifstream test("WoWChatLog.txt");
  std::string text;
  std::smatch results;
  std::set<std::string> names;
  for (std::string line; std::getline(test, line);) {
    text = line.substr(line.find("  ")+2);
    if (std::regex_search(text, re)) {
      std::regex_search(text, results, re_name);
      names.insert(results[1]);
    }
  }

  for (auto name : names) {
    std::cout << name << std::endl;
    if (name == "") std::cout << "PUSTE\n";
  }
}

int main() {
  auto start = std::chrono::high_resolution_clock::now();
  // LineParser::Initialize("regex.json");
  // std::ifstream test("WoWChatLog.txt");
  // for (std::string line; std::getline(test, line);) {
  //   LineParser::ParseLine(line);
  // }

  // std::cout << std::endl;
  // for (auto& message_type : LineParser::message_types) {
  //   std::cout << message_type.type << ": " << message_type.weight << "\n";
  // }

  LineParser::Initialize("regex.json");
  // LineWriterQtDB writer("localhost", "root", "root", "scratch");
  // LineWriterFile writer;
  LineWriterCout writer;
  // LineWriterSqlFile writer("localhost", "root", "", "scratch", "lines2");
  // LineParser::ParseFile("WoWChatLog.txt", &writer);
  // LineParser::ParseFile("test.txt", &writer);
  // writer.WriteLine(*LineParser::ParseLine("9/12 21:26:02.332  Lel says: 'lskdfj;aslkdfjls;d''k'jfls;kdjf'"));


  try {
    // LineWriterDatabase writer("localhost", "root", "", "scratch", "lines");
    // LineParser::ParseFile("test.txt", &writer);
    LineParser::ParseFile("WoWChatLog.txt", &writer);
    // LineParser::ParseFile("/home/lampilelo/rpg/rh/logs/WoWLogs.txt", &writer);
  // writer.WriteLine(*LineParser::ParseLine("9/12 21:26:02.332  Lel says: 'lskdfj;aslkdfjls;d''k'jfls;kdjf'"));
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  // std::ifstream filestream("test.txt");
  // for(std::string line; std::getline(filestream, line);)
  //   std::cout << line << "\n";

  auto stop = std::chrono::high_resolution_clock::now();
  std::cout << "\nTime elapsed: "
            << std::chrono::duration<double>(stop - start).count()
            << "s.\n";
  return 0;
}
