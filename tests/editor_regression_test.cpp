#include "eleutheria/area_io.hpp"
#include "eleutheria/mob_damage_estimate.hpp"
#include "eleutheria/source_blocks.hpp"
#include "eleutheria/world.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string read_all(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

int fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  return 1;
}

bool almost_equal(double a, double b) {
  const double d = a - b;
  return d < 0.05 && d > -0.05;
}

int test_damage_estimate() {
  eleutheria::DiceValues dam{2, 6, 3};
  const auto sim =
      eleutheria::simulate_mob_round_damage(eleutheria::kActWarrior | eleutheria::kActMagicUser, 'A',
                                            20, 2, dam);
  if(sim.components.size() != 2) {
    return fail("damage components expected 2 (Fighter+Mage)");
  }
  if(sim.components[0].min_damage != 10 || sim.components[0].max_damage != 30) {
    return fail("fighter range wrong for 2x 2d6+3");
  }
  if(!almost_equal(sim.components[0].avg_damage, 20.0)) {
    return fail("fighter avg wrong");
  }
  if(sim.components[1].min_damage != 20 || sim.components[1].max_damage != 200) {
    return fail("mage disintegrate range wrong");
  }
  std::cout << "OK damage estimate\n";
  return 0;
}

int test_area_roundtrip_and_edit(const std::filesystem::path& area_path,
                                 const std::filesystem::path& out_dir) {
  auto loaded = eleutheria::load_area(area_path);
  if(loaded.area.mobiles.empty() && loaded.area.rooms.empty()) {
    return fail("area empty: " + area_path.string());
  }

  const auto base = out_dir / area_path.stem();
  std::filesystem::create_directories(out_dir);
  auto untouched = loaded.area;
  for(const auto& [vnum, _] : untouched.mobiles) {
    untouched.dirty_mobs.insert(vnum);
  }
  for(const auto& [vnum, _] : untouched.rooms) {
    untouched.dirty_rooms.insert(vnum);
  }
  for(const auto& [vnum, _] : untouched.objects) {
    untouched.dirty_objects.insert(vnum);
  }
  for(const auto& [num, _] : untouched.zones) {
    untouched.dirty_zones.insert(num);
  }
  untouched.specials_dirty = true;
  eleutheria::save_area(untouched, base);

  // Confronta conteggi dopo reload (non byte-identici: preserve/format).
  const auto reloaded = eleutheria::load_area(base);
  if(reloaded.area.rooms.size() != loaded.area.rooms.size() ||
     reloaded.area.mobiles.size() != loaded.area.mobiles.size() ||
     reloaded.area.objects.size() != loaded.area.objects.size() ||
     reloaded.area.zones.size() != loaded.area.zones.size()) {
    return fail("roundtrip size mismatch for " + area_path.string());
  }

  // Simula edit maschera: merge-on-save cambia solo exp di un mob, resto intatto.
  if(!loaded.area.mobiles.empty()) {
    const auto vnum = loaded.area.mobiles.begin()->first;
    const auto previous = loaded.area.mobiles.at(vnum);
    eleutheria::Mobile edited = previous;
    const auto old_exp = edited.exp;
    edited.exp = old_exp < 0 ? -12345 : 7;
    edited.gold = previous.gold;
    edited.race = previous.race;
    edited.has_spellpower = previous.has_spellpower;
    edited.spellpower = previous.spellpower;
    edited.has_immunities = previous.has_immunities;
    edited.immune = previous.immune;
    edited.m_immune = previous.m_immune;
    edited.susc = previous.susc;
    edited.hit_dice_n = previous.hit_dice_n;
    edited.hit_dice_s = previous.hit_dice_s;
    edited.hit_dice_b = previous.hit_dice_b;

    auto mutated = loaded.area;
    mutated.mobiles[vnum] = edited;
    mutated.dirty_mobs.insert(vnum);
    const auto edit_base = out_dir / (area_path.stem().string() + "_edit");
    eleutheria::save_area(mutated, edit_base);
    const auto after = eleutheria::load_area(edit_base);
    const auto& m = after.area.mobiles.at(vnum);
    if(m.exp != edited.exp) {
      return fail("edited exp not persisted");
    }
    if(m.gold != previous.gold || m.race != previous.race || m.level != previous.level ||
       m.act != previous.act || m.dam_dice_n != previous.dam_dice_n) {
      return fail("merge-on-save lost unrelated mob fields");
    }
    if(previous.has_spellpower && (!m.has_spellpower || m.spellpower != previous.spellpower)) {
      return fail("spellpower lost after edit");
    }
    if(previous.has_immunities &&
       (!m.has_immunities || m.immune != previous.immune || m.m_immune != previous.m_immune ||
        m.susc != previous.susc)) {
      return fail("immunities lost after edit");
    }
  }

  // Stanza: edit name, preserva teleport/river se presenti.
  if(!loaded.area.rooms.empty()) {
    std::int64_t room_vnum = -1;
    for(const auto& [vnum, room] : loaded.area.rooms) {
      if(room.has_teleport || room.has_river || room.has_bright || room.has_tunnel_limit) {
        room_vnum = vnum;
        break;
      }
    }
    if(room_vnum < 0) {
      room_vnum = loaded.area.rooms.begin()->first;
    }
    const auto previous = loaded.area.rooms.at(room_vnum);
    eleutheria::Room edited = previous;
    edited.name = previous.name + " X";
    auto mutated = loaded.area;
    mutated.rooms[room_vnum] = edited;
    mutated.dirty_rooms.insert(room_vnum);
    const auto edit_base = out_dir / (area_path.stem().string() + "_room_edit");
    eleutheria::save_area(mutated, edit_base);
    const auto after = eleutheria::load_area(edit_base);
    const auto& r = after.area.rooms.at(room_vnum);
    if(r.name != edited.name) {
      return fail("room name edit not persisted");
    }
    if(r.has_teleport != previous.has_teleport || r.tele_targ != previous.tele_targ ||
       r.has_river != previous.has_river || r.river_speed != previous.river_speed ||
       r.has_bright != previous.has_bright || r.has_tunnel_limit != previous.has_tunnel_limit) {
      return fail("room advanced fields lost after name edit");
    }
  }

  std::cout << "OK area " << area_path.filename().string() << " rooms=" << loaded.area.rooms.size()
            << " mobs=" << loaded.area.mobiles.size() << " objs=" << loaded.area.objects.size()
            << " spe=" << loaded.area.specials.size() << "\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if(test_damage_estimate() != 0) {
    return 1;
  }

  std::vector<std::filesystem::path> areas;
  if(argc >= 2) {
    for(int i = 1; i < argc; ++i) {
      areas.emplace_back(argv[i]);
    }
  } else {
#ifdef IZANAGI_SOURCE_DIR
    areas = {std::filesystem::path{IZANAGI_SOURCE_DIR} / "sample" / "B1"};
#else
    areas = {"sample/B1"};
#endif
  }

  const auto out_dir = std::filesystem::temp_directory_path() / "izanagi_editor_regression";
  std::filesystem::remove_all(out_dir);
  std::filesystem::create_directories(out_dir);

  int failures = 0;
  for(const auto& area : areas) {
    try {
      if(test_area_roundtrip_and_edit(area, out_dir / area.stem()) != 0) {
        ++failures;
      }
    } catch(const std::exception& e) {
      std::cerr << "FAIL exception on " << area << ": " << e.what() << "\n";
      ++failures;
    }
  }

  if(failures != 0) {
    std::cerr << failures << " area test(s) failed\n";
    return 1;
  }
  std::cout << "OK all regression tests\n";
  return 0;
}
