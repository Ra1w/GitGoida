#include <set>
#include <vector>
#include <print>
#include "gitcringe.hpp"


int cringe::cmd_log(const std::set<char> &singles, const std::vector<std::string_view> &args)
{
    (void)singles;
    // bool one_line = std::find(args.begin(), args.end(), "--oneline") != args.end();
    // bool show_all = std::find(args.begin(), args.end(), "--all") != args.end();

    cringe::Repo repo(std::filesystem::current_path());
    
    std::optional<cringe::Commit> base = repo.GetHead();
    if (args.size() == 1)
    {
        auto result = repo.GetCommit(args[0]);
        if (result.size() == 2)
        {
            std::println("Call is ambigous between {} and {}.", result[0].GetId(), result[1].GetId());
            return 1;
        }
        if (result.size() == 0)
        {
            std::println("Cannot find any commit matches {}", args[0]);
            return 1;
        }
        base.emplace(result[0]);
    }

    (void)args;

    std::println("Run from base id {}", base->GetId());

    /* create tree from strings */
    // std::map<int64_t, >
    // for ()

    return 0;
}
