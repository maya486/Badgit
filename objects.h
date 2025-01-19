#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <optional>
#include <sstream>
#include <variant>
#include <vector>

using namespace std;

const filesystem::path DIR = ".badgit";
const filesystem::path OBJECTS_DIR = DIR / "objects";
const filesystem::path HEADS_DIR = DIR / "refs/heads";
const filesystem::path MAIN_BRANCH = HEADS_DIR / "main";
const filesystem::path HEAD_FILE = DIR / "HEAD";

string read_file_content_from_path(filesystem::path path);
string hash_string(string s);
string locate_file_content(string hash);

class BadgitObject {
public:
  // virtual BadgitObject from_hash(string) = 0;
  //  don't have ^ because then would have to return a pointer and that is too
  //  much effort.
  virtual string get_string_rep() = 0;
  virtual string get_hash() { return hash_string(get_string_rep()); }
  void write_object() {
    string hash = get_hash();
    filesystem::path object_file_dir = OBJECTS_DIR / hash.substr(0, 2);
    if (!filesystem::exists(object_file_dir)) {
      filesystem::create_directories(object_file_dir);
    }
    filesystem::path object_file_name = object_file_dir / hash.substr(2);

    ofstream object_file(object_file_name);
    // if (!object_file) {
    // cout << "couldn't write to file: " << object_file_name << '\n';
    //}
    object_file << get_string_rep();
  }
};
class Tree;
class Entry;

class Blob : public BadgitObject {
  string content;

public:
  Blob(string c) : content(c) {}
  static Blob from_hash(string hash);
  string get_string_rep() override { return content; }
  string get_hash() override;
};

class Entry {
  bool is_tree;
  string t_or_b_hash;
  string name;

public:
  Entry(bool it, string tobh, string nm)
      : is_tree(it), t_or_b_hash(tobh), name(nm) {}
  string get_string_rep();
  static Entry from_string_rep(string sr);
  variant<Tree, Blob> get_inner_obj();
	string get_name() {return name;}
  // doesn't need a get_hash() or from_hash() because it isn't an actual badgit
  // object
};

class Tree : public BadgitObject {
  vector<Entry> entries;

public:
  Tree(vector<Entry> e) : entries(e) {}
  Tree(vector<pair<filesystem::path, string>>);
  static Tree from_hash(string hash);
  string get_string_rep() override;
  string get_hash() override;
	vector<Entry> get_entries() {return entries;}
};

class Commit : public BadgitObject {
  string tree_hash;
  // string user_name;
  // string datetime;
  string message; // cannot contain new lines, must be on one line
  string parent_commit_hash;

public:
  Commit(string th, string pch, string msg)
      : tree_hash(th), parent_commit_hash(pch), message(msg) {}
  static Commit from_hash(string hash);
  string get_string_rep() override;
  string get_hash() override;
  Tree get_tree();
  optional<Commit> get_parent();
  string get_message() { return message; }
  string get_parent_commit_hash() { return parent_commit_hash; }
};

struct Head {
  bool is_branch;
  string content; // either a commit hash or a branch name
  Head(bool ib, string c) : is_branch(ib), content(c) {}
  Head();
  void write();
  string get_current_commit_hash();
	optional<string> get_branch_name();
};
