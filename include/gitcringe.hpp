#ifndef GITCRINGE_H
#define GITCRINGE_H


#include <iostream>
#include <span>
#include <set>
#include <string_view>
#include <vector>
#include <filesystem>
#include <optional>



namespace cringe
{
    enum GitCommandTypes
    {
        GitCommandInit,
        GitCommandCommit,
        GitCommandMerge,
    };

    class Repo;


    using commit_id_t = int64_t;


    class Commit
    {
    public:
        Commit(Repo repo, commit_id_t id);
        ~Commit();


        // restores File sytem to state of this commit
        void RestoreFS();

        bool IsChildOf(Commit other);

        bool IsDirectChildOf(Commit other);

        int64_t GetId();
        
        std::span<Commit> GetParents();
    };

    class Transaction
    {
    public:
        Transaction(Repo repo);
        ~Transaction();

        // Applyes changes and returnining new commit
        Commit Apply();
        // Add changes
        void LoadFile(std::filesystem::path path);
    };

    
    class Repo
    {
    public:
        
        Repo(std::filesystem::path path);
        ~Repo();

        // updates current index to this commit.
        bool UpdateIndex(std::optional<Commit> commit);
        
        Commit GetIndex();
        
        Commit GetHead();

        bool MoveHead(Commit commit);
        
        Commit GetCommit(std::string_view identifer);

        // Get root path
        std::string_view RootPath();

        // Start new commit
        Transaction StartCommit();
    };

    int help_fn();
    int cmd_init(const std::set<char> &singles, const std::vector<std::string_view> &args);
    int cmd_add(const std::set<char> &singles, const std::vector<std::string_view> &args);
    int cmd_log(const std::set<char> &singles, const std::vector<std::string_view> &args);
    int cmd_commit(const std::set<char> &singles, const std::vector<std::string_view> &args);
}


#endif
