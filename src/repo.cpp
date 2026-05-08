#include "gitcringe.hpp"

#include <vector>
#include <string>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <print>
#include <cinttypes>
#include <stdexcept>
#include <generator>

namespace cringe
{
    static std::filesystem::path ensure_dir(std::filesystem::path p) 
    {
        std::filesystem::create_directories(p);
        return p;
    }

    Repo::Repo(std::filesystem::path path)
    try : db((ensure_dir(path /".cvcs") / "cringe.db").u8string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE),
          root(path.lexically_normal()),
          fs_storage_path(ensure_dir(path / ".cvcs") / "fs")
    {
        db.exec("PRAGMA foreign_keys = ON;");
        db.exec("PRAGMA case_sensitive_like = OFF;");
        db.exec(R"Request(
            CREATE TABLE IF NOT EXISTS blobs
            (   
                id INTEGER PRIMARY KEY, 
                data BLOB
            );
            CREATE TABLE IF NOT EXISTS files
            (
                id INTEGER PRIMARY KEY, 
                hash BLOB(32) UNIQUE, 
                in_filesystem BOOL, 
                blob_id INTEGER REFERENCES blobs(id), 
                filesystem_path TEXT
            );
            CREATE TABLE IF NOT EXISTS commits 
            (
                id INTEGER PRIMARY KEY, 
                hash BLOB(32), 
                author TEXT,
                message TEXT
            );
            CREATE TABLE IF NOT EXISTS commit_links 
            (
                child_id INTEGER REFERENCES commits(id) ON DELETE CASCADE, 
                parent_id INTEGER REFERENCES commits(id) ON DELETE CASCADE, 
                PRIMARY KEY (child_id, parent_id)
            );
            CREATE TABLE IF NOT EXISTS labels
            (
                commit_id INTEGER REFERENCES commits(id) ON DELETE CASCADE,
                name TEXT, 
                PRIMARY KEY (commit_id, name)
            );
            CREATE TABLE IF NOT EXISTS commit_fs 
            (
                commit_id INTEGER REFERENCES commits(id) ON DELETE CASCADE, 
                path TEXT, 
                file_id INTEGER REFERENCES files(id),
                PRIMARY KEY (commit_id, path)
            );
            CREATE TABLE IF NOT EXISTS vhead 
            (
                id INTEGER PRIMARY KEY CHECK (id = 1), -- Only one entry
                commit_id INTEGER REFERENCES commits(id)
            );
            CREATE TABLE IF NOT EXISTS vindex
            (
                id INTEGER PRIMARY KEY CHECK (id = 1), -- Only one entry
                commit_id INTEGER REFERENCES commits(id)
            );
        )Request");
        
        db.exec("INSERT OR IGNORE INTO vhead (id, commit_id) VALUES (1, NULL);");
        db.exec("INSERT OR IGNORE INTO vindex (id, commit_id) VALUES (1, NULL);");
    }
    catch (std::exception& e) 
    {
        throw CringeError(std::format("Error: can't open database: {}", e.what()), {});
    }


    Repo::~Repo() = default;

    bool Repo::UpdateIndex(std::optional<Commit> commit)
    {
        int64_t commit_id = commit.has_value() ? commit->GetId() : GetHead().GetId();
        SQLite::Statement query(db, R"Request(
            UPDATE vindex SET commit_id = ? WHERE id = 1
        )Request");
        query.bind(1, commit_id);
        return query.exec() > 0;
    }

    bool Repo::UpdateHead(Commit commit)
    {
        SQLite::Statement query(db, R"Request(
            UPDATE vhead SET commit_id = ? WHERE id = 1
        )Request");
        query.bind(1, commit.GetId());
        return query.exec() > 0;
    }

    Commit Repo::GetIndex()
    {
        SQLite::Statement query(db, R"Request(
            SELECT commit_id FROM vindex WHERE id = 1
        )Request");
        if (query.executeStep()) 
        {
            return Commit(*this, query.getColumn(0).getInt64());
        }
        throw CringeError("?No Index commit found?", {});
    }

    Commit Repo::GetHead()
    {
        SQLite::Statement query(db, R"Request(
            SELECT commit_id FROM vhead WHERE id = 1
        )Request");
        if (query.executeStep()) 
        {
            return Commit(*this, query.getColumn(0).getInt64());
        }
        throw CringeError("?No Index commit found?", {});
    }
    
    std::vector<Commit> Repo::GetCommit(std::string_view identifier) 
    {
        std::string argument = "%" + std::string(identifier) + "%";
        
        SQLite::Statement query(db, R"Request(
            SELECT commit_id, name FROM labels WHERE name LIKE ? LIMIT 2
        )Request");
        query.bind(1, argument);

        if (!query.executeStep()) 
        {
            return {};
        }
        int64_t first_id = query.getColumn(0).getInt64();
        if (query.getColumn(1).getString() == identifier)
        {
            return {Commit(*this, first_id)};
        }
        if (query.executeStep()) 
        {
            return {Commit(*this, first_id), Commit(*this, query.getColumn(0).getInt64())};
        }
        return {Commit(*this, first_id)};
    }

    std::filesystem::path Repo::RootPath()
    {
        return root;
    }

    Transaction Repo::StartCommit()
    {
        return Transaction(*this, "NoName");
    }

    void Repo::CollectGarbage()
    {
        // TODO:
        // db.exec(R"Request(
        //     
        // )Request")
    }

    Transaction::Transaction(Repo &repo, std::string authorName)
             : repo(repo), authorName(authorName), tn(repo.db)
    {
    }

    Transaction::~Transaction() = default;

    void Transaction::AddParent(Commit commit)
    {
        parents.push_back(commit);
    }

    void Transaction::LoadFile(std::filesystem::path path)
    {
        // update path
        path = std::filesystem::absolute(path);
        
        // calculate hash
        std::array<uint8_t, 32> hash;
        // TODO: this
        
        // insert file entry
        updates.emplace_back(hash,
                             (std::filesystem::exists(path) ? PendingUpdateActionData
                                                            : PendingUpdateActionDelete), 
                             path.lexically_normal().lexically_relative(repo.RootPath()));
    }

    int64_t Transaction::GetFileId(Transaction::PendingUpdate update)
    {
        // is there file with same content
        SQLite::Statement query(repo.db, R"Request(
            SELECT id FROM files WHERE hash = ?
        )Request");
        query.bind(1, update.hash.data(), update.hash.size());
        if (query.executeStep())
        {
            // found same file, use it's id
            return query.getColumn(0).getInt64();
        }

        // need to create new file.
        uintmax_t size = std::filesystem::file_size(update.file);
        if (size > repo.Config.FilesystemSizeThreshold)
        {
            // copy and compress file to directory
            // TODO copy
            std::filesystem::path result_path = "...";
            
            // insert new entry into files
            SQLite::Statement query(repo.db, R"Request(
                INSERT INTO files (hash, in_filesystem, blob_id, filesystem_path) VALUES (?, true, null, ?)
            )Request");
            query.bind(1, update.hash.data(), update.hash.size());
            query.bind(2, result_path);
            query.exec();
            return repo.db.getLastInsertRowid();
        }
        else
        {
            // read file
            std::ifstream file(update.file, std::ios::binary);
            if (!file.is_open()) 
            {
                throw CringeError(std::format("Can't open file {}.", update.file.string()), {});
            }
            std::vector<std::byte> buffer(size);
            if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) 
            {
                throw CringeError(std::format("Can't open file {}.", update.file.string()), {});
            }
            // insert new entry into blobs
            SQLite::Statement queryBlob(repo.db, R"Request(
                INSERT INTO blobs (data) VALUES (?)
            )Request");
            queryBlob.bind(1, buffer.data(), static_cast<int>(buffer.size()));
            queryBlob.exec();
            int64_t blob_id = repo.db.getLastInsertRowid();
            // insert new entry into files
            SQLite::Statement query(repo.db, R"Request(
                INSERT INTO files (hash, in_filesystem, blob_id, filesystem_path) VALUES (?, false, ?, null)
            )Request");
            query.bind(1, update.hash.data(), update.hash.size());
            query.bind(2, blob_id);
            query.exec();
            return repo.db.getLastInsertRowid();
        }
    }

    std::generator<std::string> Transaction::GetDiffrentFiles()
    {
        if (parents.size() <= 1) 
        {
            co_return;
        }
    
        std::string queryParams = "?";
        for (size_t i = 1; i < parents.size(); ++i) 
        {
            queryParams += ", ?";
        }
    
        SQLite::Statement query(repo.db, R"Request(
            SELECT path 
            FROM commit_fs 
            WHERE commit_id IN ()Request" + queryParams + R"Request() 
            GROUP BY path
            HAVING MIN(file_id) != MAX(file_id) -- if there is 2 diffrent file_id's
                   OR COUNT(commit_id) < ?      -- or if in some parents id isn't presented
        )Request");
        
        int bindIndex = 1;
        for (const Commit &id : parents) 
        {
            query.bind(bindIndex++, id.GetId());
        }
        query.bind(bindIndex, static_cast<int64_t>(parents.size()));
    
        while (query.executeStep()) 
        {
            co_yield query.getColumn(0).getText();
        }
    }
    
    std::generator<std::pair<int64_t, std::string>> Transaction::GetCommonFiles()
    {
        if (parents.size() == 0) 
        {
            co_return;
        }
    
        std::string queryParams = "?";
        for (size_t i = 1; i < parents.size(); ++i) 
        {
            queryParams += ", ?";
        }
    
        SQLite::Statement query(repo.db, R"Request(
            SELECT MIN(file_id) as file_id, path 
            FROM commit_fs 
            WHERE commit_id IN ()Request" + queryParams + R"Request() 
            GROUP BY path
            HAVING MIN(file_id) = MAX(file_id) AND COUNT(commit_id) = ? -- have one id and exists in all parents
        )Request");
        
        int bindIndex = 1;
        for (const Commit &id : parents) 
        {
            query.bind(bindIndex++, id.GetId());
        }
        query.bind(bindIndex, static_cast<int64_t>(parents.size()));        
    
        while (query.executeStep()) 
        {
            co_yield {query.getColumn(0).getInt64(), query.getColumn(1).getText()};
        }
    }


    Commit Transaction::Apply(std::string commitMessage)
    {
        // check if all diffrent files was changed, if there is more than 1 parent
        if (parents.size() > 1)
        {
            std::unordered_set<std::string> loaded;
            for (auto update : updates)
            {
                loaded.insert(update.file);
            }
            std::vector<std::string> error_msg;
            for (auto path : GetDiffrentFiles())
            {
                if (!loaded.contains(path))
                {
                    error_msg.emplace_back(std::format("File {} is different in parents, but wasn't added to transaction.", path));
                }
            }
            if (!error_msg.empty())
            {
                throw CringeError("Not all changed files was added to transaction.", std::move(error_msg));
            }
        }

        // calculate commit hash, as all updates hash + parent hash.
        // TODO: do we need hashes for commits at all?
        std::array<uint8_t, 32> buffer;
        
        // inset new commit id
        SQLite::Statement query(repo.db, R"Request(
            INSERT INTO commits (hash, author, message) VALUES (?, ?, ?)
        )Request");
        query.bind(1, buffer.data(), static_cast<int>(buffer.size()));
        query.bind(2, authorName);
        query.bind(3, commitMessage);
        query.exec();
        int64_t commit_id = repo.db.getLastInsertRowid();

        // insert all new and old files
        // add all new files
        std::map<std::string, int64_t> path2id;
        for (auto update : updates)
        {
            path2id[update.file] = GetFileId(update);
        }
        // add old files
        for (auto [id, path] : GetCommonFiles())
        {
            path2id[path] = id;
        }
        
        SQLite::Statement fsInsertQuery(repo.db, R"Request(
            INSERT INTO commit_fs (commit_id, path, file_id) VALUES (?, ?, ?)
        )Request");

        for (const auto& [path, file_id] : path2id) 
        {
            fsInsertQuery.reset();
            
            fsInsertQuery.bind(1, commit_id);
            fsInsertQuery.bind(2, path);
            fsInsertQuery.bind(3, file_id);
            
            fsInsertQuery.executeStep();
        }

        
        SQLite::Statement linkInsertQuery(repo.db, R"Request(
            INSERT INTO commit_links (child_id, parent_id) VALUES (?, ?)
        )Request");

        for (const Commit &parent : parents) 
        {
            linkInsertQuery.reset();
            
            linkInsertQuery.bind(1, commit_id);
            linkInsertQuery.bind(2, parent.GetId());
            
            linkInsertQuery.executeStep();
        }

        tn.commit();
        
        return Commit(repo, commit_id);
    }

    Commit::Commit(Repo &repo, commit_id_t id) 
        : repo(repo),
          id(id)
    {}
    
    Commit::Commit(const Commit& other) 
        : repo(other.repo), id(other.id) 
    {}
    
    Commit::Commit(Commit&& other) noexcept 
        : repo(other.repo), id(other.id) 
    {
        other.id = -1;
    }

    Commit::~Commit() = default;

    std::vector<std::string> Commit::ListFiles()
    {
        SQLite::Statement query(repo.db, R"Request(
            SELECT path -- DISTINCT isn't required
            FROM commit_fs 
            WHERE commit_id = ?
        )Request");
        query.bind(1, id);
        std::vector<std::string> results;
        results.reserve(256); // for average fs
        while (query.executeStep()) 
        {
            results.emplace_back(query.getColumn(0).getText());
        }
        return results;
    }

    bool Commit::IsChildOf(Commit other) 
    {
        if (this->GetId() == other.GetId()) return true;
        SQLite::Statement query(repo.db, R"Request(
            WITH RECURSIVE ancestors(id) AS (
                SELECT ? 
                UNION
                SELECT parent_id
                FROM commit_links
                JOIN ancestors ON ancestors.id = commit_links.child_id
            )
            SELECT EXISTS(SELECT 1 FROM ancestors WHERE id = ?)
        )Request");

        query.bind(1, id);
        query.bind(2, other.GetId());

        if (query.executeStep()) 
        {
            return query.getColumn(0).getInt() > 0;
        }
        return false;
    }
    
    bool Commit::IsDirectChildOf(Commit other) 
    {
        SQLite::Statement query(repo.db, R"Request(
            SELECT 1 FROM commit_links WHERE child_id = ? AND parent_id = ?
        )Request");
        
        query.bind(1, id);
        query.bind(2, other.GetId());
    
        return query.executeStep();
    }

    std::vector<Commit> Commit::GetParents() 
    {
        SQLite::Statement query(repo.db, R"Request(
            SELECT parent_id FROM commit_links WHERE child_id = ?
        )Request");
        query.bind(1, id);
        
        std::vector<Commit> parents;
        parents.reserve(4);
        while (query.executeStep()) 
        {
            commit_id_t parentId = query.getColumn(0).getInt64();    
            parents.emplace_back(repo, parentId);
        }

        return parents;
    }

    bool Commit::RestoreFile(std::filesystem::path path) 
    {
        std::string relativePath = path.lexically_normal().lexically_relative(repo.RootPath()).string();
        
        SQLite::Statement query(repo.db, R"Request(
            SELECT f.in_filesystem, f.filesystem_path, b.data
            FROM commit_fs cfs
            JOIN files f ON cfs.file_id = f.id
            LEFT JOIN blobs b ON f.blob_id = b.id
            WHERE cfs.commit_id = ? AND cfs.path = ?
        )Request");

        query.bind(1, id);
        query.bind(2, relativePath);

        if (!query.executeStep()) // if file doesn't exists in commit's fs, remove it.
        {
            if (!std::filesystem::exists(path)) 
            {
                return false;
            }
            std::filesystem::remove(path);
            return true;
        }

        bool inFilesystem = query.getColumn(0).getInt() > 0;

        if (inFilesystem) 
        {
            // TODO:
            // std::string fsSourcePath = query.getColumn(1).getText();
            // std::filesystem::copy_file(repo.RootPath() / fsSourcePath, path, 
            //                            std::filesystem::copy_options::overwrite_existing);
            return false; 
        } 
        else 
        {
            auto blob = query.getColumn(2);
            std::ofstream out(path, std::ios::binary);
            out.write(static_cast<const char*>(blob.getBlob()), blob.getBytes());
            return true;
        }
    }

    commit_id_t Commit::GetId() const
    {
        return id;
    }
    
}
