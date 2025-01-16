#include "objects.h"
#include <filesystem>
#include <unordered_map>
#include <set>

const filesystem::path INDEX_FILE = DIR / "index";

struct FileState {
  string index_hash;
  string repo_hash;
};

struct CheckFileStatusResult {
	vector<filesystem::path> untracked_paths;
	vector<filesystem::path> modified_paths;
	vector<filesystem::path> staged_paths;
};
	

class Index {
  unordered_map<filesystem::path, FileState> file_map;

public:
  Index(); // read from index file and create index object
  void add(vector<filesystem::path> paths); // only modify object state
  Tree commit();                            // only modify object state
  void write();                             // write to index file
  CheckFileStatusResult check_file_states(
      set<filesystem::path> wd_paths); // returns {not found paths, staged paths}
};
