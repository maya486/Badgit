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
  string checkout_commit(string hash);
  set<filesystem::path> collect_wd_paths();
  string list_branches();
  optional<string> remove_known_paths();
};
