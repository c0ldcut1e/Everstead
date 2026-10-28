#pragma once

#include <atomic>
#include <string>

class Socket
{
public:
    Socket();
    explicit Socket(const std::string &name);
    virtual ~Socket();

    virtual bool IsOpen() const;
    virtual void Close();

    void SetName(const std::string &name);
    const std::string &GetName() const;

private:
    std::string m_name;
    std::atomic<bool> m_open;
};
