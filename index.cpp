#include "index.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_map>

using namespace std;

Index::Index() {

  ifstream obj_file(INDEX_FILE.string());
  if (!obj_file) {

    throw runtime_error(
        "Couldn't read from file when trying to locate_file_content: " +
        INDEX_FILE.string());
  }
  stringstream buffer;
  buffer << obj_file.rdbuf();
  string index_file_content = buffer.str();

  istringstream stream(index_file_content);
  string line;

  while (getline(stream, line)) {
    istringstream line_words(line);
    filesystem::path new_path;
    line_words >> new_path;
    FileState fs;
    line_words >> fs.index_hash >> fs.repo_hash;
    file_map[filesystem::absolute(new_path)] = fs;
  }
}

void Index::add(vector<filesystem::path> paths) {
  for (auto &relative_path : paths) {
		filesystem::path path = filesystem::absolute(relative_path);
    string file_content = read_file_content_from_path(path);
    Blob file_blob = Blob(file_content);
    file_blob.write_object(); // write to objects dir

    if (file_map.end() == file_map.find(path)) {
      // file doesn't exist
      file_map[path] =
          FileState{file_blob.get_hash(), "0"}; // 0 means untracked by repo
    } else {
      // file doesn't exist to badgit yet
      file_map[path].index_hash = file_blob.get_hash();
    }
  }
}

void Index::write() {
  ofstream index_file(INDEX_FILE);
  for (const auto &path_file_state_pair : file_map) {
    index_file << filesystem::absolute(path_file_state_pair.first) << " "
               << path_file_state_pair.second.index_hash << " "
               << path_file_state_pair.second.repo_hash << '\n';
  }
}

Tree Index::commit() {
  // builds everything into a tree and returns this to VCS to make actual commit
  // also updates Index file to have
  vector<pair<filesystem::path, string>> commit_files;
  for (auto &[path, file_state] : file_map) {
    // update Index to commit staged paths
    if (file_state.repo_hash != file_state.index_hash) {
      file_state.repo_hash = file_state.index_hash;
    }
    commit_files.push_back({path, file_state.repo_hash});
  }
  return Tree(commit_files);
}

CheckFileStatusResult Index::check_file_states(set<filesystem::path> wd_paths) {
  set<filesystem::path> known_paths;
  CheckFileStatusResult cfsr;

  for (const auto &[path, file_state] : file_map) {
    known_paths.insert(path);

    if (wd_paths.find(path) != wd_paths.end()) {
      // path is known by badgit so either modified or staged

      string file_content = read_file_content_from_path(path);
      Blob file_blob = Blob(file_content);
      if (file_blob.get_hash() != file_state.index_hash) {
        cfsr.modified_paths.push_back(path);
      }
      if (file_state.index_hash != file_state.repo_hash) {
        cfsr.staged_paths.push_back(path);
      }
    }
  }
  vector<filesystem::path> unknown_paths;
  for (const auto &path : wd_paths) {
    if (known_paths.find(path) == known_paths.end()) {
      cfsr.untracked_paths.push_back(path);
    }
  }

  return cfsr;
}



