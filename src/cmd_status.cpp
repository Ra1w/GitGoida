#include <set>
#include <vector>
#include <print>
#include "gitcringe.hpp"


void dump_changes(cringe::Repo &repo, cringe::Commit commit)
{
    bool any = false;
    
    const char* RED = "\033[31m";
    const char* GREEN = "\033[32m";
    const char* YELLOW = "\033[33m";
    const char* RESET = "\033[0m";

    for (auto [type, path] : repo.ListChangedFiles(commit))
    {   
        any = true;
        const char *stype = "";
        const char *color = RESET;

        if (type == cringe::UPDATE_CREATE) 
        {
            stype = "created";
            color = GREEN;
        }
        else if (type == cringe::UPDATE_CHANGE) 
        {
            stype = "updated";
            color = YELLOW;
        }
        else if (type == cringe::UPDATE_DELETE) 
        {
            stype = "deleted";
            color = RED;
        }

        std::println("{}file was {}: {}{}", color, stype, path.string(), RESET);
    }

    if (!any)
    {
        std::println("No files was modified.");
    }
}



int cringe::cmd_status(const std::set<char> &singles, const std::vector<std::string_view> &args)
{
    (void)singles;
    (void)args;

    cringe::Repo repo(std::filesystem::current_path());
    
    std::println("On repo {}, head on {}, index on {}", repo.RootPath().string(), repo.GetHead().GetId(), repo.GetIndex().GetId());

    std::println();
    
    std::println("Changes from HEAD:");

    dump_changes(repo, repo.GetHead());
    
    std::println();
    
    std::println("Changes from INDEX:");

    dump_changes(repo, repo.GetHead());
        
    return 0;
}
