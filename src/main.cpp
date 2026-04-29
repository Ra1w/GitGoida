// #include "gitcringe.hpp"

#include <span>
#include <set>
#include <string>
#include <iostream>


int help_fn()
{
    std::cout << "No help! Cry about it.\n";

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
    
    for (std::string_view arg : std::span(argv, argc).subspan(1))
    {
        if (arg.size() > 2 && arg[0] == '-' && arg[1] != '-') continue;

        if (arg == "init")
        {
            
        }

        break;
    }
}
