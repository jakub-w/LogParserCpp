#include "LineWriterCout.h"

#include <iomanip>
#include <iostream>

bool LineWriterCout::WriteLine(const Line& line) {
  std::cout << std::put_time(&line.date, "%m/%d/%y %H:%M:%S  ")
            << line.text << std::endl;
  return true;
}
