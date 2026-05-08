#include "gitcringe.hpp"
#include <set>
#include <print>

int cringe::cmd_add(const std::set<char> &singles, const std::vector<std::string_view> &args)
{
    cringe::Repo repo(std::filesystem::current_path());    
    // load files to index
    // TODO: squash commit with previous index
    cringe::Transaction trn = repo.StartCommit();
    trn.AddParent(repo.GetIndex()); // parent is prev index
    if (singles.contains('A'))
    {
        try 
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(repo.RootPath())) 
            {
                if (entry.is_regular_file()) 
                {
                    trn.LoadFile(entry.path());
                }
            }
        } 
        catch (const std::filesystem::filesystem_error& e) 
        {
            std::println("Access error {}", e.what());
        }
    }
    else
    {
        for (std::string_view arg : args)
        {
            trn.LoadFile(arg);
        }
    }
    cringe::Commit commit = trn.Apply("<index>");
    repo.UpdateIndex(commit);

    std::println("Index updated");
    
    return 0;
}
