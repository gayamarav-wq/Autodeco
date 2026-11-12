#include "Catalog.hpp"

#include <Geode/Geode.hpp>
#include <set>

using namespace geode::prelude;

namespace adeco {

std::vector<int> presetObjectIds(Preset const& p) {
    std::vector<int> ids;
    if (p.structure.enabled) ids.push_back(p.structure.id);
    for (auto const& r : p.rules)
        for (auto const& o : r.objects) ids.push_back(o.id);
    if (p.rhythm.marker.id > 0) ids.push_back(p.rhythm.marker.id);
    return ids;
}

std::unordered_map<int, GeodeCatalog::Entry>& GeodeCatalog::cache() {
    static std::unordered_map<int, Entry> c;
    return c;
}

void GeodeCatalog::prime(std::vector<int> const& ids) {
    if (!m_lel) return;
    auto& c = cache();

    std::vector<int> todo;
    std::set<int> seen;
    for (int id : ids)
        if (id >= 1 && id <= 99999 && !c.count(id) && seen.insert(id).second) todo.push_back(id);
    if (todo.empty()) return;

    // One string with every probe, parked in a block of cells high above normal levels.
    std::string str;
    for (size_t i = 0; i < todo.size(); ++i)
        str += fmt::format("1,{},2,{},3,{};", todo[i], 15 + 30 * int(i % 40), 1215 + 30 * int(i / 40));

    // noUndo = true (not recorded), noLimit = true (a level at its object limit must not break the probe).
    CCArray* made = m_lel->createObjectsFromString(str, true, true);
    std::vector<Ref<GameObject>> objs;
    if (made)
        for (auto obj : CCArrayExt<GameObject*>(made))
            if (obj) objs.push_back(obj);

    std::set<int> got;
    for (auto& o : objs) {
        int id = o->m_objectID;
        Entry e;
        if (typeinfo_cast<EffectGameObject*>(o.data())) {
            e.why = "is a trigger";
        } else if (o->m_objectType != GameObjectType::Decoration) {
            e.why = fmt::format("is not a pure decoration (game object type {})", int(o->m_objectType));
        } else {
            CCSize cs = o->getContentSize();
            e.w = cs.width > 1.f ? cs.width : kCell;
            e.h = cs.height > 1.f ? cs.height : kCell;
            e.ok = true;
        }
        c[id] = e;
        got.insert(id);
    }
    // Delete every probe right away.
    for (auto& o : objs) m_lel->removeObject(o, true);

    for (int id : todo)
        if (!got.count(id)) c[id] = Entry{false, 30.f, 30.f, "the editor did not create it (unknown ID in this game version?)"};

    m_requested += int(todo.size());
    m_created += int(objs.size());
    log::info("Auto Decorator: probed {} object IDs, the editor created {}", todo.size(), objs.size());
}

bool GeodeCatalog::usable(int id, float& w, float& h) const {
    auto const& c = cache();
    auto it = c.find(id);
    if (it == c.end()) return false;  // never primed: treat as unusable rather than guessing
    w = it->second.w;
    h = it->second.h;
    return it->second.ok;
}

std::string GeodeCatalog::reason(int id) const {
    auto const& c = cache();
    auto it = c.find(id);
    if (it == c.end()) return "not checked";
    return it->second.ok ? std::string() : it->second.why;
}

}  // namespace adeco
