#include "vcs.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char *argv[]) {
  VCS vcs = VCS();
  if (argc > 0) {
    string arg = argv[1];
    if (arg == "init") {
      vcs.init();
    } else if (arg == "add") {
			vector<filesystem::path> paths;
			for (int i=2; i<argc; ++i) {
				paths.push_back(filesystem::relative(argv[i], DIR.parent_path()));
			}
			vcs.add(paths);
		} else if (arg == "status") {
			cout << vcs.status();
		} else if (arg == "commit") {
			string message;
			if (argc>1) message = argv[2];
			vcs.commit(message);
		} else if (arg == "log") {
			cout << vcs.log();
		} else if (arg == "add-branch") {
			vcs.add_branch(argv[2]);
		} else if (arg == "checkout-branch") {
			cout << vcs.checkout_branch(argv[2]);
		}
  }
  return 0;
}
