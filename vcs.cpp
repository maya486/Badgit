#include "vcs.h"
#include "utils.h"
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

    // update branch head to new commit
    ofstream branch_head_file(HEADS_DIR / h.content);
    branch_head_file << c.get_hash();
  } else {
    Head new_head = Head(false, c.get_hash());
    new_head.write();
  }

  // ofstream main_branch(MAIN_BRANCH);
  // main_branch << c.get_hash();
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
    status += "No untracked files\n";
  } else {
    status += "Untracked files:\n";
    for (auto &untracked_path : cfsr.untracked_paths) {
      status += '\t' +
                filesystem::relative(untracked_path, filesystem::current_path())
                    .string() +
                '\n';
    }
  }

  if (cfsr.modified_paths.empty()) {
    status += "No modified files\n";
  } else {
    status += "Modified files:\n";
    for (auto &modified_path : cfsr.modified_paths) {
      status += '\t' +
                filesystem::relative(modified_path, filesystem::current_path())
                    .string() +
                '\n';
    }
  }

  if (cfsr.staged_paths.empty()) {
    status += "No staged files\n";
  } else {
    status += "Staged files to be committed:\n";
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
    status = "On branch " + *maybe_branch_name + "\n\n" + status;
  } else {
    status = "Detached head at commit: " + head.get_current_commit_hash() + "\n\n" + status;
  }

  return status;
}

string VCS::log() {
  string log = "";
  Head head = Head();
  optional<string> maybe_branch_name = head.get_branch_name();
  if (maybe_branch_name) {
    log += "On branch " + *maybe_branch_name + '\n';
  } else {
    log = "Detached head\n";
  }

	vector<pair<string, string>> branch_name_and_commit_hash;
  for (auto branch_file : filesystem::directory_iterator(HEADS_DIR)) {
		string branch_name =branch_file.path().filename() ;
		Head temp_head = Head(true, branch_name);
		string commit_hash = temp_head.get_current_commit_hash();
    branch_name_and_commit_hash.push_back({branch_name, commit_hash});
	}

  string current_commit_hash = head.get_current_commit_hash();
  while (!current_commit_hash.empty()) {
    Commit c = Commit::from_hash(current_commit_hash);

		string branch_info = "";
		vector<string> branches_at_commit;
		for (auto& [branch_name, branch_commit_hash] : branch_name_and_commit_hash) {
			if (branch_commit_hash == current_commit_hash) {
				branches_at_commit.push_back(branch_name);
			}
		}
		if (!branches_at_commit.empty()) {
			branch_info = " (";
			for (string& branch_name : branches_at_commit) {
				branch_info += branch_name + ", ";
			}
			branch_info = branch_info.substr(0, branch_info.length()-2);
			branch_info += ")";
		}

    log += "commit " + current_commit_hash + branch_info + '\n';
    log += '\t' + c.get_message() + "\n\n";
    current_commit_hash = c.get_parent_commit_hash();
  }
  return log;
}

void VCS::add_branch(string branch_name) {
  ofstream new_branch_file{HEADS_DIR / branch_name};
  Head head = Head();
  new_branch_file << head.get_current_commit_hash();
}

optional<string> VCS::remove_known_paths() {

  set<filesystem::path> wd_paths = VCS::collect_wd_paths();
  Index index = Index();
  CheckFileStatusResult cfsr = index.check_file_states(wd_paths);
  if (cfsr.modified_paths.size() || cfsr.staged_paths.size()) {
    return "There are currently modified and/or staged files. Please either "
           "commit or reset them. Use the 'badgit status' command to see more "
           "info.\n";
  }

  // get known paths from index
  vector<filesystem::path> tracked_paths = index.get_all_tracked_paths();
  // remove all known paths
  for (auto path : tracked_paths) {
    filesystem::remove(path);
  }
  delete_empty_dirs(filesystem::absolute(DIR.parent_path()));
  return {};
}

string VCS::checkout_branch(string branch_name) {
  optional<string> error_msg = VCS::remove_known_paths();
  if (error_msg) {
    return error_msg.value();
  }

  Head new_head = Head(true, branch_name);
  new_head.write();
  Index new_index = Index::from_commit_hash(new_head.get_current_commit_hash());
  new_index.write();

  // load all files from new index
  new_index.bring_out_files();

  return "Currently on branch: " + branch_name + '\n';
}

string VCS::list_branches() {
  string output = "";
  Head head = Head();
  optional<string> current_branch = head.get_branch_name();
  for (auto branch_file : filesystem::directory_iterator(HEADS_DIR)) {
    string branch_name = branch_file.path().filename();
    if (current_branch && branch_name == current_branch) {
      output += "* ";
    } else {
      output += "  ";
    }
    output += branch_file.path().filename().string() + "\n";
  }
  return output;
}

string VCS::checkout_commit(string hash) {
  optional<string> error_msg = VCS::remove_known_paths();
  if (error_msg) {
    return error_msg.value();
  }

  if (hash.length() < 40) {
    // need to elongate it to full hash
    filesystem::path object_dir = OBJECTS_DIR / hash.substr(0, 2);
    for (auto object_file : filesystem::directory_iterator(object_dir)) {
      string object_file_name = object_file.path().filename().string();
      if (object_file_name.substr(0, hash.length() - 2) == hash.substr(2)) {
        hash = hash.substr(0, 2) + object_file_name;
        break;
      }
    }
  }

  Head new_head = Head(false, hash);
  new_head.write();
  Index new_index = Index::from_commit_hash(hash);
  new_index.write();

  // load all files from new index
  new_index.bring_out_files();

  return "Switched to commit " + hash + '\n';
}
