#include "empress.hpp"

using namespace empress::logging;

void enable_protection() {
  empress::protection::enable() ?
	log(log_level::info, "Protection active!") :
	log(log_level::error, "Failed to set protection!");
}

int main() {
  enable_protection();
  
  log(log_level::info, "Press ENTER to close program.");
  return std::cin.get();
}
