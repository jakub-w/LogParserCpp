#include "LineWriterFile.h"

#include <iostream>
#include <iomanip>

bool LineWriterFile::WriteLine(const Line& line) {
  if (output_streams_.end() == output_streams_.find(line.type)) {
    // output_streams_.insert({line.type, std::ofstream(line.type + ".txt")});
    output_streams_.emplace(line.type, std::ofstream(line.type + ".txt"));
  }

  // TODO: This doesn't work. I wanted to use it instead of above, but no.
  //       I have no idea why it doesn't work. INVESTIGATE!
  // output_streams_.try_emplace(line.type, std::ofstream(line.type + ".txt"));
  output_streams_[line.type] <<
      std::put_time(&line.date, "%m/%d/%y %H:%M:%S  ") << line.text << '\n';
  return true;
}
