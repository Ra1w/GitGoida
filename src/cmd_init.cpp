#include "gitcringe.hpp"
#include <set>
#include <print>

int cringe::cmd_init(const std::set<char> &singles, const std::vector<std::string_view> &args)
{
    (void)singles;
    (void)args;
    
    cringe::Repo repo(std::filesystem::current_path());    
    cringe::Transaction trn = repo.StartCommit();
    cringe::Commit commit = trn.Apply("initial commit");

    repo.CreateBranch("main", commit);
    repo.AttachHead("main");

    repo.UpdateHead(commit);
    repo.UpdateIndex(std::nullopt);

    std::println("Init repository. Created branch 'main'. Top commit id is {}", commit.GetId());
    
    return 0;
}
