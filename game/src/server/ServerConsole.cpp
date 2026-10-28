#ifdef EVERSTEAD_DEDICATED_SERVER

#include "ServerConsole.h"

#include <iostream>
#include <thread>

void ServerConsole::Start()
{
    // We detach because getline can't be interrupted, and the console outlives the server loop anyway
    std::thread(&ServerConsole::Run, this).detach();
}

bool ServerConsole::PollLine(std::string *outPtr)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_lines.empty())
    {
        return false;
    }

    *outPtr = m_lines.front();
    m_lines.erase(m_lines.begin());
    return true;
}

void ServerConsole::Run()
{
    std::string line;
    while (std::getline(std::cin, line))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lines.push_back(line);
    }
}

#endif
