#pragma once

#include <string>

#include "Block.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

class DiodeBlock : public Block
{
public:
    DiodeBlock(const std::string &topPath, const std::string &litTopPath);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnRemoved(Level *level, const BlockPos &pos, uint16_t data) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    bool IsUsable(uint16_t data) const override;
    int GetSignal(uint16_t data, Direction *direction) const override;
    int GetDirectSignal(uint16_t data, Direction *direction) const override;
    bool IsSignalSource() const override;

    Direction *GetOutput(uint16_t data) const;
    bool IsPowered(uint16_t data) const;
    // Torch bases sit below the slab top so the plane tops land where the torch heads belong
    virtual void GetTorches(uint16_t data, std::vector<glm::dvec3> *out, std::vector<bool> *lit) const = 0;
    UVRect GetTorchAtlasUVRect(bool lit) const;

protected:
    struct Face
    {
        int texture;
        glm::vec4 uv;
        int rotation;
    };

    struct Element
    {
        glm::vec3 from;
        glm::vec3 to;
        Face faces[6];
    };

    virtual void GetElements(uint16_t data, std::vector<Element> *out) const = 0;
    virtual void CheckTickOnNeighbor(Level *level, const BlockPos &pos, uint16_t data);
    virtual bool IsLocked(Level *level, const BlockPos &pos, uint16_t data) const;
    virtual bool ShouldTurnOn(Level *level, const BlockPos &pos, uint16_t data) const;
    virtual int GetDelay(uint16_t data) const        = 0;
    virtual int GetOutputSignal(uint16_t data) const = 0;
    virtual bool IsAlternateInput(Block *block) const;
    virtual int GetInputSignal(Level *level, const BlockPos &pos, uint16_t data) const;
    int GetAlternateSignal(Level *level, const BlockPos &pos, uint16_t data) const;
    bool IsFacingTowardsDiode(Level *level, const BlockPos &pos, uint16_t data) const;
    void UpdateNeighborsInFront(Level *level, const BlockPos &pos, uint16_t data) const;
    void SetData(Level *level, const BlockPos &pos, uint16_t data) const;
    void AddBase(std::vector<Element> *out, bool lit) const;
    void AddElement(std::vector<Element> *out, const glm::vec3 &from, const glm::vec3 &to, int texture, const glm::vec4 &northSouthUV,
                    const glm::vec4 &westEastUV, const glm::vec4 &upDownUV, int upDownRotation) const;

    int m_facingProperty;
    int m_poweredProperty;
    int m_topTexture;
    int m_litTopTexture;
    int m_slabTexture;
    int m_torchTexture;
    int m_unlitTorchTexture;

    glm::dvec3 ToWorld(Direction *output, const glm::dvec3 &model) const;

private:
    glm::dvec3 ToModel(Direction *output, const glm::dvec3 &world) const;
    AABB ToWorldBox(Direction *output, const Element &element) const;
    // This finds the model face behind a box face and works out where its texture corners land in the atlas
    void GetFaceUV(uint16_t data, const AABB &box, Direction *direction, int *texture, float *u0, float *v0, float *u1, float *v1) const;
};
