#include "vcs.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

void VCS::init() {
  filesystem::remove_all(DIR);
  filesystem::create_directories(DIR);
  filesystem::create_directories(OBJECTS_DIR);
  filesystem::create_directories(HEADS_DIR);
  // do nothing for index dir?
  ofstream _index_file(INDEX_FILE);
  ofstream _head_file(HEAD_FILE);
  Head head = Head(true, "main");
  head.write();
  ofstream _main_branch(MAIN_BRANCH);
}
void VCS::commit(string message) {
  Index i = Index();
  Tree commit_tree = i.commit();
  commit_tree.write_object();
  i.write();
  Head h = Head();
  string current_commit_hash = h.get_current_commit_hash();

  Commit c = Commit(commit_tree.get_hash(), current_commit_hash, message);
  // if head is commit, need to update it
  if (h.is_branch) {

    ofstream branch_file(HEADS_DIR / h.content);
    branch_file << c.get_hash();
  } else {
    Head new_head = Head(false, c.get_hash());
    new_head.write();
  }
  ofstream main_branch(MAIN_BRANCH);
  main_branch << c.get_hash();
  c.write_object();
}

void VCS::add(vector<filesystem::path> paths) {
  Index i = Index();
  i.add(paths);
  i.write();
}
set<filesystem::path> VCS::collect_wd_paths() {

  set<filesystem::path> wd_paths;
  for (auto it =
           filesystem::recursive_directory_iterator(filesystem::current_path());
       it != filesystem::end(it); ++it) {
    auto file = *it;

    filesystem::path relative_path = filesystem::relative(file.path(), DIR);

    if (!relative_path.empty() && relative_path.string().find("..") != 0) {
      it.disable_recursion_pending();
      continue;
    }
    if (filesystem::is_regular_file(file) &&
        file.path().extension() == ".txt") {
      wd_paths.insert(file);
    }
  }
	return wd_paths;
}

string VCS::status() {
  // read all files in current directory that are .txt and check if they have
  // been modified from what is in repo and what is in staging
	set<filesystem::path> wd_paths = VCS::collect_wd_paths();
  Index i = Index();
  CheckFileStatusResult cfsr = i.check_file_states(wd_paths);

  string status = "";

  if (cfsr.untracked_paths.empty()) {
    status += "No untracked paths\n";
  } else {
    status += "Untracked paths:\n";
    for (auto &untracked_path : cfsr.untracked_paths) {
      status += '\t' +
                filesystem::relative(untracked_path, filesystem::current_path())
                    .string() +
                '\n';
    }
  }

  if (cfsr.modified_paths.empty()) {
    status += "No modified paths\n";
  } else {
    status += "Modified paths:\n";
    for (auto &modified_path : cfsr.modified_paths) {
      status += '\t' +
                filesystem::relative(modified_path, filesystem::current_path())
                    .string() +
                '\n';
    }
  }

  if (cfsr.staged_paths.empty()) {
    status += "No staged changes\n";
  } else {
    status += "Staged changes to be committed:\n";
    for (auto &staged_path : cfsr.staged_paths) {
      status += '\t' +
                filesystem::relative(staged_path, filesystem::current_path())
                    .string() +
                '\n';
    }
  }

	Head head = Head();
	optional<string> maybe_branch_name = head.get_branch_name();
	if (maybe_branch_name) {
		status = "On branch " + *maybe_branch_name + '\n' + status;
	}

  return status;
}

string VCS::log() {
  Head head = Head();
  string current_commit_hash = head.get_current_commit_hash();
  string log = "badgit commit log:";
  while (!current_commit_hash.empty()) {
    Commit c = Commit::from_hash(current_commit_hash);
    log += '\n' + c.get_message();
    current_commit_hash = c.get_parent_commit_hash();
  }
  return log + '\n';
}

void VCS::add_branch(string branch_name)
{ 
	ofstream new_branch_file{HEADS_DIR / branch_name};
	Head head = Head();
	new_branch_file << head.get_current_commit_hash();
}

string VCS::checkout_branch(string branch_name){
	set<filesystem::path> wd_paths = VCS::collect_wd_paths();
	Index index = Index();
	CheckFileStatusResult cfsr = index.check_file_states(wd_paths);
	if (cfsr.modified_paths.size() || cfsr.staged_paths.size()) {
		return "There are currently modified and/or staged files. Please either commit or reset them. Use the 'badgit status' command to see more info.\n";
	}

	Head new_head = Head(true, branch_name);
	new_head.write();

	
	// get known branches from index
	// remove all known branches
	// switch to new index from new commit
	// load all files from new index

	return "Currently on branch: " +branch_name +'\n';
}

