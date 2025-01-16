#include "objects.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <optional>
#include <sstream>
#include <unordered_map>

using namespace std;

string hash_string(string s) {
  unsigned char hash[SHA_DIGEST_LENGTH];

  SHA1(reinterpret_cast<const unsigned char *>(s.c_str()), s.length(), hash);
  stringstream ss;

  // Convert each byte in the hash array to its hexadecimal representation
  for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
    ss << std::setw(2) << std::setfill('0') << std::hex << (int)hash[i];
  }

  return ss.str();
}

string read_file_content_from_path(filesystem::path path) {

  ifstream obj_file(path.string());
  if (!obj_file) {

    throw runtime_error(
        "Couldn't read from file when trying to locate_file_content: " +
        path.string());
  }
  stringstream buffer;
  buffer << obj_file.rdbuf();
  return buffer.str();
}
string locate_file_content(string hash) {
  filesystem::path hash_dir = hash.substr(0, 2) + "/" + hash.substr(2);
  filesystem::path full_hash_dir = OBJECTS_DIR / hash_dir;

  return read_file_content_from_path(full_hash_dir);
}

Blob Blob::from_hash(string hash) {
  string content = locate_file_content(hash);
  return Blob{content};
}

string Blob::get_hash() { return hash_string(get_string_rep()); }

string Entry::get_string_rep() {
  string s = "";
  if (is_tree)
    s += "t";
  else
    s += "b";

  s += " " + t_or_b_hash + " " + name;
  return s;
}

Entry Entry::from_string_rep(string sr) {
  bool is_tree = false;
  string t_or_b_hash = sr.substr(2, 40);
  if (sr[0] == 't') {
    is_tree = true;
  } else if (sr[0] == 'b') {
  } else {
    throw runtime_error(
        "Malformatted string when trying to transform into Entry: " + sr);
  }
  string name = sr.substr(43);
  return Entry(is_tree, t_or_b_hash, name);
}

variant<Tree, Blob> Entry::get_inner_obj() {
  if (is_tree) {
    return Tree::from_hash(t_or_b_hash);
  } else {
    return Blob::from_hash(t_or_b_hash);
  }
}

// Tree::Tree(vector<Entry> e) {
// entries = e;
// for (Entry &entry : entries) {
// string_rep += entry.get_string_rep() + '\n';
//}
// hash = hash_string(string_rep);
//}

Tree Tree::from_hash(string hash) {
  string content = locate_file_content(hash);
  vector<Entry> entries;
  istringstream stream(content);
  string line;

  while (getline(stream, line)) {
    entries.push_back(Entry::from_string_rep(line));
  }
  return Tree(entries);
}

string Tree::get_string_rep() {
  string string_rep = "";
  for (Entry &e : entries) {
    string_rep += e.get_string_rep() + '\n';
  }
  return string_rep;
}

string Tree::get_hash() { return hash_string(get_string_rep()); }

Tree::Tree(vector<pair<filesystem::path, string>> paths_and_hashes) {
  unordered_map<string, vector<pair<filesystem::path, string>>>
      dir_name_to_child_files;

  for (auto &[path, hash] : paths_and_hashes) {
    filesystem::path relative =
        filesystem::relative(path, filesystem::current_path());
    if (relative.parent_path().empty()) {
      entries.push_back(Entry(false, hash, path.string()));
    } else {
      // string front_dir = relative.front().string();
      auto it = relative.begin();
      filesystem::path one_up_relative = "";
      string first_dir = *it;
      ++it;
      for (; it != relative.end(); ++it) {
        one_up_relative /= *it;
      }
      dir_name_to_child_files[first_dir].push_back({one_up_relative, hash});
    }
  }

  // if (path.has_parent_path()) {
  //// will need recursive tree call
  // filesystem::path first_dir = path;
  // cout << "first_dir" << first_dir << '\n';
  // cout << "current path" << filesystem::current_path() << '\n';
  // while (first_dir.has_parent_path()) {
  ////cout << "first_dir" << first_dir << '\n';
  // first_dir = first_dir.parent_path();
  //}
  // cout << "after while loop\n";
  //// Remove the first directory from the path
  // filesystem::path new_path = filesystem::relative(path, first_dir);
  // dir_name_to_child_files[first_dir.string()].push_back({new_path, hash});
  //} else {
  //// shallow filename, no recursion needed, just add as entry
  // entries.push_back(Entry(false, hash, path.string()));
  //}
  //}

  for (auto &[dir_name, child_files] : dir_name_to_child_files) {
    Tree child_tree = Tree(child_files);
    entries.push_back(Entry(true, child_tree.get_hash(), dir_name));
  }
}

Commit Commit::from_hash(string hash) {
  string content = locate_file_content(hash);
  istringstream stream(content);
  string tree_hash, message, parent_commit_hash;
  getline(stream, tree_hash);
  getline(stream, message);
  getline(stream, parent_commit_hash);
  return Commit(tree_hash, parent_commit_hash, message);
}

string Commit::get_string_rep() {
  return tree_hash + "\n" + message + "\n" + parent_commit_hash;
}

string Commit::get_hash() { return hash_string(get_string_rep()); }

Tree Commit::get_tree() { return Tree::from_hash(tree_hash); }

optional<Commit> Commit::get_parent() {
  if (parent_commit_hash.empty()) {
    return {};
  } else {
    return Commit::from_hash(parent_commit_hash);
  }
}

Head::Head() {
  string file_contents = read_file_content_from_path(HEAD_FILE);
  istringstream file_contents_stream(file_contents);
  string is_branch_char;
  file_contents_stream >> is_branch_char;
  file_contents_stream >> content;
  if (is_branch_char == "b") {
    is_branch = true;
  } else {
    is_branch = false;
  }
}

void Head::write() {
  ofstream head_file_stream(HEAD_FILE);
  string string_rep = "";
  if (is_branch) {
    string_rep += "b";
  } else {
    string_rep += "c";
  }
  string_rep += " " + content;
  head_file_stream << string_rep;
}

string Head::get_current_commit_hash() {
  if (is_branch) {
    return read_file_content_from_path(HEADS_DIR / content);
  } else {
    return content;
  }
}

optional<string> Head::get_branch_name() {
	if (is_branch) {
		return content;
	} else {
		return {};
	}
}
