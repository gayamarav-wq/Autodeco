// Tells the planner which object IDs can be used in THIS game build and how big their sprites are.
//
// How: for every ID we have not seen yet we ask the editor itself to create one real object (via the editor's
// own createObjectsFromString pipeline, so the object is fully initialised), read its true type and size, and
// delete it again immediately - all in the same call, before anything is drawn or simulated, so a probe can never
// affect a level. Only IDs whose real object is a non-colliding decoration are ever placed.
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/Planner.hpp"

class LevelEditorLayer;

namespace adeco {

// Every object ID a preset can place (rules, structure tile, beat marker).
std::vector<int> presetObjectIds(Preset const& p);

class GeodeCatalog final : public ObjectCatalog {
public:
    explicit GeodeCatalog(LevelEditorLayer* lel) : m_lel(lel) {}

    // Probes every not-yet-known ID in `ids`. Results are cached for the whole game session.
    void prime(std::vector<int> const& ids);

    bool usable(int id, float& w, float& h) const override;
    std::string reason(int id) const;  // "" if usable, otherwise why it was rejected

    int probeRequested() const { return m_requested; }  // IDs probed by this instance
    int probeCreated() const { return m_created; }      // of those, how many the editor actually created

private:
    struct Entry {
        bool ok = false;
        float w = 30, h = 30;
        std::string why;
    };
    static std::unordered_map<int, Entry>& cache();

    LevelEditorLayer* m_lel;
    int m_requested = 0, m_created = 0;
};

}  // namespace adeco
