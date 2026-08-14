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

        const std::size_t skipped_fill = nebbie::refresh_inbound_exit_descriptions(
            world, 34020, nebbie::InboundExitAlignPolicy::FillEmptyOnly);
        if (skipped_fill != 0) {
            throw std::runtime_error("fill-empty policy should not update descriptions");
        }
        if (world.find_room(34021)->exits.front().description != "di fronte al castello") {
            throw std::runtime_error("custom look text was modified during fill-empty refresh");
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
        if (filled != 0) {
            throw std::runtime_error("empty descriptions should stay empty");
        }
        if (!world.find_room(34022)->exits.front().description.empty()) {
            throw std::runtime_error("empty inbound exit description was filled");
        }

        const std::size_t synced = nebbie::refresh_inbound_exit_descriptions(world, 34020);
        if (synced != 0) {
            throw std::runtime_error("manual sync should not overwrite custom look text");
        }
        if (world.find_room(34021)->exits.front().description != "di fronte al castello") {
            throw std::runtime_error("custom look text was modified during manual sync");
        }

        nebbie::RoomEdit edit;
        edit.name = "Nuovo titolo destinazione";
        world.find_room(34021)->exits.front().description = destination.name;
        if (!nebbie::edit_room(world, 34020, edit)) {
            throw std::runtime_error("edit_room failed");
        }
        if (world.find_room(34021)->exits.front().description != edit.name) {
            throw std::runtime_error("edit_room did not refresh inbound exit that matched old destination name");
        }
        if (!world.find_room(34022)->exits.front().description.empty()) {
            throw std::runtime_error("edit_room should not fill empty inbound exit on rename");
        }

        world.find_room(34021)->exits.front().description = "nome destinazione errato";
        const nebbie::ValidationReport mismatch_report = nebbie::validate_world(world);
        for (const auto& issue : mismatch_report.issues) {
            if (issue.message.find("does not match destination name") != std::string::npos) {
                throw std::runtime_error("validation should not warn on custom exit descriptions");
            }
        }

        world.find_room(34021)->exits.front().description = "Nuovo titolo destinazione\n";
        const std::size_t normalized = nebbie::refresh_inbound_exit_descriptions(world, 34020);
        if (normalized != 1) {
            throw std::runtime_error("whitespace-only mismatch should be normalized");
        }
        if (world.find_room(34021)->exits.front().description != edit.name) {
            throw std::runtime_error("whitespace-only mismatch was not normalized to destination name");
        }

        nebbie::Room bracket_source;
        bracket_source.vnum = 34023;
        bracket_source.name = "Sorgente legacy";
        bracket_source.description = "Test.";
        bracket_source.sector_type = 1;
        nebbie::Exit bracket_exit;
        bracket_exit.direction = 1;
        bracket_exit.description = "[003013 (strada commerciale)";
        bracket_exit.to_room = 34020;
        bracket_source.exits.push_back(bracket_exit);
        world.rooms.emplace(bracket_source.vnum, bracket_source);

        if (!nebbie::is_legacy_bracket_exit_description(bracket_exit.description)) {
            throw std::runtime_error("bracket legacy description not detected");
        }

        const nebbie::ExitAlignmentReport bulk = nebbie::align_all_inbound_exit_descriptions(world);
        if (bulk.exits_aligned != 0) {
            throw std::runtime_error("bulk alignment should not modify custom or empty descriptions");
        }
        if (world.find_room(34023)->exits.front().description != "[003013 (strada commerciale)") {
            throw std::runtime_error("bulk alignment modified legacy bracket description");
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
        if (world.find_room(34023)->exits.front().description != "Il cortile interno") {
            throw std::runtime_error("rename should preserve custom look text that is not the old room name");
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
        door_exit.keyword = "porta";
        door_exit.exit_info = 1;
        door_source.exits.push_back(door_exit);
        world.rooms.emplace(door_source.vnum, door_source);

        const std::size_t door_synced = nebbie::refresh_inbound_exit_descriptions(world, 34021);
        if (door_synced != 0) {
            throw std::runtime_error("custom prose in exit description should be preserved on sync");
        }
        if (world.find_room(34030)->exits.front().description
            != "La porta e' fatta di mithril. Non sembra chiusa a chiave.") {
            throw std::runtime_error("custom prose was modified during sync");
        }
        if (world.find_room(34030)->exits.front().keyword != "porta") {
            throw std::runtime_error("sync should not modify door keyword");
        }

        nebbie::Room outbound_source;
        outbound_source.vnum = 34040;
        outbound_source.name = "Sorgente uscita singola";
        outbound_source.description = "Test.";
        outbound_source.sector_type = 1;
        nebbie::Exit outbound_exit;
        outbound_exit.direction = 2;
        outbound_exit.description = "etichetta corta";
        outbound_exit.to_room = 34021;
        outbound_source.exits.push_back(outbound_exit);
        world.rooms.emplace(outbound_source.vnum, outbound_source);

        const nebbie::ExitLabelAlignResult single =
            nebbie::align_room_exit_description(world, 34040, 2);
        if (!single.updated) {
            throw std::runtime_error("per-exit alignment should force destination name");
        }
        if (world.find_room(34040)->exits.front().description != world.find_room(34021)->name) {
            throw std::runtime_error("per-exit alignment did not set exact destination name");
        }

        const nebbie::ExitLabelAlignResult second_single =
            nebbie::align_room_exit_description(world, 34040, 2);
        if (!second_single.already_ok) {
            throw std::runtime_error("second per-exit alignment should report already_ok");
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
