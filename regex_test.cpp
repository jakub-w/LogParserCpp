#include <iostream>
#include <regex>
#include <chrono>
#include <vector>

void DoRegexes(const std::regex& re,
               const std::vector<std::string>& texts) {
  for (auto& text : texts) {
    std::cout << text << "\n";
    auto result = std::regex_search(text, re);
    long double sum = 0;
    std::smatch matches;
    std::string name;
    for (int i = 0; i < 100000; ++i) {
      auto before = std::chrono::high_resolution_clock::now();
      std::regex_search(text, matches, re);
      name = "";
      for (long unsigned int i = 1; i < matches.size(); ++i) name += matches[i];
      auto after = std::chrono::high_resolution_clock::now();
      sum += std::chrono::duration<double>(after - before).count();
    }

    std::cout << result << ": " << sum / 100000 << "--"
              << (name == "" ? std::string("NONE") : name)
              << "--" << matches.size() << "\n\n";
  }
}

int main() {
  std::regex re("^\\|Hchannel:(?:raid|RAID|Raid)\\||(?:Legacy )?Raid Difficulty set to|has (?:left|joined) the raid group|^\\[Raid Warning\\]");

  std::vector<std::string> texts = {
    "|Hchannel:raid|h[Raid]|h Selane: Caerthir, Aeltharian, trzyma",
    "|Hchannel:RAID|h[Raid]|h Selane: Caerthir, Aeltharian, trzyma",
    "|Hchannel:Raid|h[Raid]|h Selane: Caerthir, Aeltharian, trzyma",
    "Legacy Raid Difficulty set to 10 Player.",
    "Raid Difficulty set to 10 Player.",
    "Милэлса-Голдринн has left the raid group.",
    "Syslei-DunModr has joined the raid group.",
    "|Hchannel:raid|h[Raid Leader]|h Aeltharian: Pffm.",
    "[Raid Warning] Lìandra-Outland: WAIT FOR THE DAMN 2 TOWE,",
  };

  DoRegexes(re, texts);

  // re.assign("^\\|Hchannel:o\\|");
  // DoRegexes(re, texts);

  return 0;
}
