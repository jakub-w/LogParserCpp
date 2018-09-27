#ifndef LOGPARSER_LINEWRITERINTERFACE_H
#define LOGPARSER_LINEWRITERINTERFACE_H

#include "Line.h"

class LineWriterInterface {
public:
  virtual ~LineWriterInterface(){}

  virtual bool WriteLine(const Line& line) = 0;

  virtual inline bool Flush(){ return true; }
};

#endif // LOGPARSER_LINEWRITERINTERFACE_H
