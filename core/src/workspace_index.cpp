#include "eleutheria/workspace_index.hpp"

#include "eleutheria/area_io.hpp"
#include "eleutheria/world.hpp"

namespace eleutheria {
namespace {

void put_entity(std::map<std::int64_t, IndexedEntity>* into, std::int64_t vnum,
                const std::string& label, const std::string& area_name) {
  if(into->count(vnum) != 0) {
    return;  // primo trovato vince
  }
  IndexedEntity ent;
  ent.vnum = vnum;
  ent.label = label;
  ent.area_name = area_name;
  (*into)[vnum] = std::move(ent);
}

}  // namespace

void WorkspaceEntityIndex::clear() {
  mobiles.clear();
  objects.clear();
  rooms.clear();
}

void WorkspaceEntityIndex::build_from_workspace(const Workspace& workspace,
                                               const std::vector<WorkspaceArea>& areas) {
  clear();
  (void)workspace;
  for(const auto& wa : areas) {
    try {
      const auto loaded = load_area(wa.path);
      for(const auto& [vnum, mob] : loaded.area.mobiles) {
        put_entity(&mobiles, vnum, strip_mud_colors(mob_list_label(mob)), wa.folder_name);
      }
      for(const auto& [vnum, obj] : loaded.area.objects) {
        put_entity(&objects, vnum, strip_mud_colors(obj_list_label(obj)), wa.folder_name);
      }
      for(const auto& [vnum, room] : loaded.area.rooms) {
        put_entity(&rooms, vnum, strip_mud_colors(room_list_label(room)), wa.folder_name);
      }
    } catch(...) {
      // Area illeggibile: salta
    }
  }
}

void WorkspaceEntityIndex::overlay_area(const Area& area, const std::string& area_name) {
  for(const auto& [vnum, mob] : area.mobiles) {
    mobiles[vnum] = IndexedEntity{vnum, strip_mud_colors(mob_list_label(mob)), area_name};
  }
  for(const auto& [vnum, obj] : area.objects) {
    objects[vnum] = IndexedEntity{vnum, strip_mud_colors(obj_list_label(obj)), area_name};
  }
  for(const auto& [vnum, room] : area.rooms) {
    rooms[vnum] = IndexedEntity{vnum, strip_mud_colors(room_list_label(room)), area_name};
  }
}

const IndexedEntity* WorkspaceEntityIndex::find_mob(std::int64_t vnum) const {
  const auto it = mobiles.find(vnum);
  return it == mobiles.end() ? nullptr : &it->second;
}

const IndexedEntity* WorkspaceEntityIndex::find_obj(std::int64_t vnum) const {
  const auto it = objects.find(vnum);
  return it == objects.end() ? nullptr : &it->second;
}

const IndexedEntity* WorkspaceEntityIndex::find_room(std::int64_t vnum) const {
  const auto it = rooms.find(vnum);
  return it == rooms.end() ? nullptr : &it->second;
}

}  // namespace eleutheria
