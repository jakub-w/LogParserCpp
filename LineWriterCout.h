#ifndef LOGPARSE_LINEWRITERCOUT_H
#define LOGPARSE_LINEWRITERCOUT_H

#include "LineWriterInterface.h"

class LineWriterCout : public LineWriterInterface {
 public:
  bool WriteLine(const Line& line);
};

#endif // LOGPARSE_LINEWRITERCOUT_H
