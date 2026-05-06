#include "gitcringe.hpp"

#include <vector>
#include <string>
#include <filesystem>
#include <print>

namespace cringe
{
    Repo::Repo(std::filesystem::path)
        try : db(path.u8string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) 
        { } 
        catch (std::exception& e) 
        {
            std::print("Error: can't open database: {}", e);
            throw;
        }
    {
        db.exec("CREATE TABLE IF NOT EXISTS blobs (id INTEGER PRIMARY KEY, in_filesystem BOOL, blob_id INTEGER)");
        db.exec("CREATE TABLE IF NOT EXISTS commits (id INTEGER PRIMARY KEY, message TEXT)");
        db.exec("CREATE TABLE IF NOT EXISTS commit_links (child INTEGER PRIMARY KEY, parent INTEGER)");
        db.exec("CREATE TABLE IF NOT EXISTS head (id INTEGER PRIMARY KEY, commit_id INTEGER)");
    }
}
