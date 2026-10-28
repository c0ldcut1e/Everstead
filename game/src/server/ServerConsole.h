#pragma once

#include <mutex>
#include <string>
#include <vector>

class ServerConsole
{
public:
    void Start();
    bool PollLine(std::string *outPtr);

private:
    void Run();

    std::mutex m_mutex;
    std::vector<std::string> m_lines;
};
