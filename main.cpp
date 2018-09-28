#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <thread>
#include <mutex>
#include "LineParser.h"
#include "LineWriterDatabase.h"
#include "LineWriterFile.h"
#include "LineWriterCout.h"
// #include "LineWriterQtDB.h"
// #include "LineWriterSqlFile.h"

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
    // LineParser::ParseFile("WoWChatLog.txt", &writer);
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
