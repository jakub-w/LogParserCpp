#ifndef LOGPARSER_MESSAGETYPE_H
#define LOGPARSER_MESSAGETYPE_H

#include <regex>

// it's sorted in std::(multi)set by weight (more is prioritized)
struct MessageType {
  uint weight;
  std::string type; // type of chat it belongs to
  std::regex regex;

  // for std::less<Regex> to order the set
  inline friend bool operator<(const MessageType& lhs,
                               const MessageType& rhs) {
    return lhs.weight > rhs.weight;
  }
};


#endif // LOGPARSER_MESSAGETYPE_H
