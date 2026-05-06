#include "../include/gitcringe.hpp"

#include <span>
#include <set>
#include <string>
#include <iostream>
#include <filesystem>
#include <cassert>
#include <print>


int help_fn()
{
    std::cout << "No help! Cry about it.\n";

    return 0;
}


int cmd_init(const std::set<char> &singles, const std::vector<std::string_view> &args)
{
    (void)singles;
    (void)args;
    
    cringe::Repo repo(std::filesystem::current_path());    
    cringe::Transaction trn = repo.StartCommit();
    cringe::Commit commit = trn.Apply();

    std::print("Init repository. Top commit id is {}", commit.GetId());
    
    return 0;
}

int cmd_add(const std::set<char> &singles, const std::vector<std::string_view> &args)
{
    cringe::Repo repo(std::filesystem::current_path());    
    // load files to index
    // TODO: squash commit with previous index
    cringe::Transaction trn = repo.StartCommit();
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
            std::cerr << "Ошибка доступа: " << e.what() << std::endl;
        }
    }
    else
    {
        for (std::string_view arg : args)
        {
            trn.LoadFile(arg);
        }
    }
    cringe::Commit commit = trn.Apply();
    repo.UpdateIndex(commit);

    std::print("Index updated\n");
    
    return 0;
}

int cmd_commit(const std::set<char> &singles, const std::vector<std::string_view> &args)
{
    (void)singles;
    (void)args;
    
    cringe::Repo repo(std::filesystem::current_path());
    
    // move index commit to branch
    // set index to null
    cringe::Commit head = repo.GetHead();
    cringe::Commit index = repo.GetIndex();
    // it is Index, so it's parent is 100% head.
    assert(index.IsDirectChildOf(head));
    repo.MoveHead(index);
    repo.UpdateIndex(std::nullopt);
    
    std::print("New commit with id {}\n", index.GetId());

    return 0;
}

int cmd_log(const std::set<char> &singles, const std::vector<std::string_view> &args)
{
    (void)singles;
    // bool one_line = std::find(args.begin(), args.end(), "--oneline") != args.end();
    // bool show_all = std::find(args.begin(), args.end(), "--all") != args.end();

    cringe::Repo repo(std::filesystem::current_path());
    
    cringe::Commit base = repo.GetHead();
    if (args.size() == 1)
    {
        base = repo.GetCommit(args[0]);
    }

    (void)args;

    /* create tree from strings */
    // std::map<int64_t, >
    // for ()

    return 0;
}


int main(int argc, const char **argv)
{
    std::set<char> singles;
    for (std::string_view arg : std::span(argv, argc).subspan(1))
    {
        if (arg.size() > 2 && arg[0] == '-' && arg[1] != '-')
        {
            for (char c : arg.substr(1))
            {
                singles.insert(c);
            }
        }
        else if (arg == "--help")
        {
            return help_fn();
        }
    }

    if (singles.contains('h'))
    {
        return help_fn();
    }

    /* parse cmd args */
    bool first = true;
    std::string_view command;
    std::vector<std::string_view> args;
    
    for (std::string_view arg : std::span(argv, argc).subspan(1))
    {
        if (arg.size() > 2 && arg[0] == '-' && arg[1] != '-') continue;

        if (first)
        {
            first = false;
            command = arg;
        }
        else
        {
            args.push_back(arg);
        }
    }

    if (command == "init")
    {
        return cmd_init(singles, args);
    }
    else if (command == "add")
    {
        return cmd_add(singles, args);
    }
    else if (command == "commit")
    {
        return cmd_commit(singles, args);
    }
    else if (command == "log")
    {
        return cmd_log(singles, args);
    }
}
