#include "string"
using namespace std;

const string HELP_STRING = R"(
badgit is a basic (and kinda bad) version control manager (like git). It supports staging, committing, and branching.

Usage: badgit <COMMAND> <ARGUMENTS> 
(note: badgit must always be called from its rootdir (where badgit init was initially called))

Commands:
	init				inits badgit repo
	status				reports status of files as relates to index
	add <FILES>			listed file paths
	commit <MESSAGE>		commits staged files with message
	log				lists all previous commits from HEAD
	add-branch <BRANCH>		creates new branch
	list-branches			lists all known branches
	checkout-branch <BRANCH>	checks out branch
	checkout-commit <HASH>		checkout out commit (hash may be truncated as long as it is still identifiable)
	help				displays help message	

)";
