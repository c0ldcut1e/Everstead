#include "Socket.h"

Socket::Socket()
{
    m_name = "socket";
    m_open = true;
}

Socket::Socket(const std::string &name)
{
    m_name = name;
    m_open = true;
}

Socket::~Socket()
{
}

bool Socket::IsOpen() const
{
    return m_open;
}

void Socket::Close()
{
    m_open = false;
}

void Socket::SetName(const std::string &name)
{
    m_name = name;
}

const std::string &Socket::GetName() const
{
    return m_name;
}
