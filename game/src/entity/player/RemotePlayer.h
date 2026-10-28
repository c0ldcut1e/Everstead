#pragma once

#include "Player.h"

class RemotePlayer : public Player
{
public:
    explicit RemotePlayer(Level *level, const std::wstring &name);
    ~RemotePlayer() override;

    eENTITY_TYPE GetType() override;
    void SetSilent(bool silent);
    void SetRiding(bool riding);
    bool IsRiding() const;
    void ApplyMoveEntity(int xa, int ya, int za, bool hasPosition, int yaw, int pitch, bool hasRotation, bool onGround) override;
    void ApplyTeleportEntity(int64_t x, int64_t y, int64_t z, int yaw, int pitch, bool onGround) override;
    void Tick() override;

    float GetRenderBob(float partialTicks) const;

    static const eENTITY_TYPE s_type = eEntityType_RemotePlayer;

private:
    bool m_silent;
    bool m_riding;
};
