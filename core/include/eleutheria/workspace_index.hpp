#pragma once

#include "eleutheria/types.hpp"
#include "eleutheria/workspace.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace eleutheria {

struct IndexedEntity {
  std::int64_t vnum = 0;
  std::string label;
  std::string area_name;  // cartella area nel workspace
};

/**
 * Indice vnum mob/obj/room su tutto il workspace.
 * Serve per picker cross-area e validazione "esterno vs placeholder".
 */
struct WorkspaceEntityIndex {
  std::map<std::int64_t, IndexedEntity> mobiles;
  std::map<std::int64_t, IndexedEntity> objects;
  std::map<std::int64_t, IndexedEntity> rooms;

  void clear();
  void build_from_workspace(const Workspace& workspace,
                            const std::vector<WorkspaceArea>& areas);
  /** Sovrascrive/aggiunge entita' dell'area aperta (anche con edit non salvati). */
  void overlay_area(const Area& area, const std::string& area_name);

  const IndexedEntity* find_mob(std::int64_t vnum) const;
  const IndexedEntity* find_obj(std::int64_t vnum) const;
  const IndexedEntity* find_room(std::int64_t vnum) const;
};

}  // namespace eleutheria
