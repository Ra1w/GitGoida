#include "gitcringe.hpp"
#include <set>
#include <print>

int cringe::cmd_add(const std::set<char> &singles, const std::vector<std::string_view> &args)
{
    cringe::Repo repo(std::filesystem::current_path());

    // load files to index
    cringe::Commit head = repo.GetHead();
    cringe::Commit old_index = repo.GetIndex();

    cringe::Transaction trn = repo.StartCommit();
    
    if (old_index.GetId() != 0)
    {
        trn.AddParent(old_index);
    }

    if (singles.contains('A'))
    {
        try 
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(repo.RootPath())) 
            {
                if (entry.is_regular_file() && entry.path().string().find(".cvcs") == std::string::npos) 
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

    cringe::Commit new_index = trn.Apply("<index>");

    repo.UpdateIndex(new_index);

    if (old_index.GetId() != 0 && old_index.GetId() != head.GetId())
    {
        repo.Squash(new_index, old_index);
    }


    std::println("Index updated");
    
    return 0;
}
