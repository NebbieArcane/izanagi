#include "nebbie/edit.hpp"
#include "nebbie/validate.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        nebbie::World world;

        nebbie::Room destination;
        destination.vnum = 34020;
        destination.name = "Di fronte alla Roccaforte della Marca Orientale";
        destination.description = "Una piazza ampia.";
        destination.sector_type = 1;
        world.rooms.emplace(destination.vnum, destination);

        nebbie::Room source;
        source.vnum = 34021;
        source.name = "Strada verso est";
        source.description = "Una strada.";
        source.sector_type = 1;
        nebbie::Exit exit;
        exit.direction = 0;
        exit.description = "di fronte al castello";
        exit.to_room = 34020;
        source.exits.push_back(exit);
        world.rooms.emplace(source.vnum, source);

        const std::size_t skipped_custom = nebbie::refresh_inbound_exit_descriptions(
            world, 34020, nebbie::InboundExitAlignPolicy::FillEmptyOnly);
        if (skipped_custom != 0) {
            throw std::runtime_error("fill-empty refresh should not update non-empty labels");
        }
        if (world.find_room(34021)->exits.front().description != "di fronte al castello") {
            throw std::runtime_error("non-empty inbound exit description was modified during fill-empty refresh");
        }

        nebbie::Room empty_source;
        empty_source.vnum = 34022;
        empty_source.name = "Altra sorgente";
        empty_source.description = "Test.";
        empty_source.sector_type = 1;
        nebbie::Exit empty_exit;
        empty_exit.direction = 2;
        empty_exit.description.clear();
        empty_exit.to_room = 34020;
        empty_source.exits.push_back(empty_exit);
        world.rooms.emplace(empty_source.vnum, empty_source);

        const std::size_t filled = nebbie::refresh_inbound_exit_descriptions(
            world, 34020, nebbie::InboundExitAlignPolicy::FillEmptyOnly);
        if (filled != 1) {
            throw std::runtime_error("expected one empty inbound exit to be filled");
        }
        if (world.find_room(34022)->exits.front().description != destination.name) {
            throw std::runtime_error("empty inbound exit description was not filled");
        }

        const std::size_t synced = nebbie::refresh_inbound_exit_descriptions(world, 34020);
        if (synced != 1) {
            throw std::runtime_error("manual sync should update stale room-name labels");
        }
        if (world.find_room(34021)->exits.front().description != destination.name) {
            throw std::runtime_error("manual sync did not update stale room-name label");
        }

        nebbie::RoomEdit edit;
        edit.name = "Nuovo titolo destinazione";
        world.find_room(34021)->exits.front().description = "di fronte al castello";
        if (!nebbie::edit_room(world, 34020, edit)) {
            throw std::runtime_error("edit_room failed");
        }
        if (world.find_room(34021)->exits.front().description != edit.name) {
            throw std::runtime_error("edit_room did not refresh stale inbound exit labels on rename");
        }
        if (world.find_room(34022)->exits.front().description != edit.name) {
            throw std::runtime_error("edit_room did not refresh empty inbound exit on rename");
        }

        world.find_room(34021)->exits.front().description = "etichetta personalizzata";
        const nebbie::ValidationReport report = nebbie::validate_world(world);
        for (const auto& issue : report.issues) {
            if (issue.category == "room" && issue.severity == nebbie::ValidationSeverity::warning
                && issue.message.find("has description") != std::string::npos) {
                throw std::runtime_error("custom exit labels should not trigger validation warnings");
            }
        }

        world.find_room(34021)->exits.front().description = "nuovo titolo destinazione";
        const std::size_t realigned = nebbie::refresh_inbound_exit_descriptions(world, 34020);
        if (realigned != 0) {
            throw std::runtime_error("case-only exit label mismatch should not trigger alignment");
        }

        nebbie::Room mismatch_source;
        mismatch_source.vnum = 34023;
        mismatch_source.name = "Sorgente con etichetta errata";
        mismatch_source.description = "Test.";
        mismatch_source.sector_type = 1;
        nebbie::Exit mismatch_exit;
        mismatch_exit.direction = 1;
        mismatch_exit.description = "etichetta errata";
        mismatch_exit.to_room = 34020;
        mismatch_source.exits.push_back(mismatch_exit);
        world.rooms.emplace(mismatch_source.vnum, mismatch_source);

        nebbie::Room bulk_empty_source;
        bulk_empty_source.vnum = 34024;
        bulk_empty_source.name = "Sorgente con uscita vuota";
        bulk_empty_source.description = "Test.";
        bulk_empty_source.sector_type = 1;
        nebbie::Exit bulk_empty_exit;
        bulk_empty_exit.direction = 3;
        bulk_empty_exit.description.clear();
        bulk_empty_exit.to_room = 34020;
        bulk_empty_source.exits.push_back(bulk_empty_exit);
        world.rooms.emplace(bulk_empty_source.vnum, bulk_empty_source);

        nebbie::Exit& aligned_source_exit = world.find_room(34024)->exits.front();
        aligned_source_exit.keyword = "porta di ferro";
        aligned_source_exit.key = 3120;
        aligned_source_exit.exit_info = 7;
        aligned_source_exit.open_cmd = 42;
        const std::string source_name = world.find_room(34024)->name;
        const std::string source_desc = world.find_room(34024)->description;
        const std::string dest_name = world.find_room(34020)->name;

        const nebbie::ExitAlignmentReport bulk = nebbie::align_all_inbound_exit_descriptions(world);
        if (bulk.exits_checked < 4 || bulk.exits_aligned != 2 || bulk.changes.size() != 2) {
            throw std::runtime_error("bulk alignment should update stale non-custom labels");
        }
        if (world.find_room(34023)->exits.front().description != world.find_room(34020)->name) {
            throw std::runtime_error("bulk alignment did not update stale exit description");
        }
        if (world.find_room(34024)->exits.front().description != world.find_room(34020)->name) {
            throw std::runtime_error("bulk alignment did not fill empty exit description");
        }
        const nebbie::Exit& aligned_exit = world.find_room(34024)->exits.front();
        if (aligned_exit.keyword != "porta di ferro" || aligned_exit.key != 3120 || aligned_exit.exit_info != 7
            || aligned_exit.open_cmd != 42) {
            throw std::runtime_error("bulk alignment modified exit fields other than description");
        }
        if (world.find_room(34024)->name != source_name || world.find_room(34024)->description != source_desc) {
            throw std::runtime_error("bulk alignment modified room fields");
        }
        if (world.find_room(34020)->name != dest_name) {
            throw std::runtime_error("bulk alignment modified destination room");
        }

        const nebbie::ExitAlignmentReport second_pass = nebbie::align_all_inbound_exit_descriptions(world);
        if (second_pass.exits_aligned != 0) {
            throw std::runtime_error("second bulk alignment pass should not modify exits");
        }

        world.rooms.clear();
        nebbie::Room courtyard;
        courtyard.vnum = 34021;
        courtyard.name = "Il cortile interno della roccaforte orientale";
        courtyard.description = "Un cortile.";
        courtyard.sector_type = 1;
        world.rooms.emplace(courtyard.vnum, courtyard);

        nebbie::Room scuderie;
        scuderie.vnum = 34023;
        scuderie.name = "Le scuderie del Marchese";
        scuderie.description = "Scuderie.";
        scuderie.sector_type = 1;
        nebbie::Exit west_exit;
        west_exit.direction = 3;
        west_exit.description = "Il cortile interno";
        west_exit.to_room = 34021;
        scuderie.exits.push_back(west_exit);
        world.rooms.emplace(scuderie.vnum, scuderie);

        nebbie::RoomEdit courtyard_edit;
        courtyard_edit.name = "Cortile orientale rinominato";
        if (!nebbie::edit_room(world, 34021, courtyard_edit)) {
            throw std::runtime_error("courtyard rename failed");
        }
        if (world.find_room(34023)->exits.front().description != courtyard_edit.name) {
            throw std::runtime_error("rename did not update stale inbound label to full room name");
        }

        nebbie::Room door_source;
        door_source.vnum = 34030;
        door_source.name = "Ingresso";
        door_source.description = "Ingresso.";
        door_source.sector_type = 1;
        nebbie::Exit door_exit;
        door_exit.direction = 0;
        door_exit.description = "La porta e' fatta di mithril. Non sembra chiusa a chiave.";
        door_exit.to_room = 34021;
        door_source.exits.push_back(door_exit);
        world.rooms.emplace(door_source.vnum, door_source);

        const std::size_t door_synced = nebbie::refresh_inbound_exit_descriptions(world, 34021);
        if (door_synced != 0) {
            throw std::runtime_error("custom door look text should not be overwritten on manual sync");
        }
        if (world.find_room(34030)->exits.front().description.find("mithril") == std::string::npos) {
            throw std::runtime_error("mithril door description was modified");
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
