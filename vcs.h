#include "index.h"
#include <filesystem>

using namespace std;

class VCS {
public:
  VCS() {}

  void init();
  void add(vector<filesystem::path> paths);
  string status();
  void commit(string message);
	string log();
	void add_branch(string branch_name);
	string checkout_branch(string branch_name);
	set<filesystem::path> collect_wd_paths();
};
