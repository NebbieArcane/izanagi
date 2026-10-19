#include "nebbie/edit.hpp"
#include "nebbie/english_lexicon.hpp"
#include "nebbie/validate.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool contains_token(const std::vector<std::string>& tokens, const std::string& needle) {
    return std::find(tokens.begin(), tokens.end(), needle) != tokens.end();
}

bool has_english_warning(const nebbie::ValidationReport& report,
                         nebbie::ValidationTarget target,
                         long vnum) {
    for (const auto& issue : report.issues) {
        if (issue.severity == nebbie::ValidationSeverity::warning && issue.category == "english_text"
            && issue.target == target && issue.target_vnum == vnum) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    try {
        const auto door_tokens = nebbie::find_english_tokens("porta metallo door metal");
        expect(contains_token(door_tokens, "door"), "expected door");
        expect(contains_token(door_tokens, "metal"), "expected metal");
        expect(!contains_token(door_tokens, "porta"), "Italian porta must not match");
        expect(!contains_token(door_tokens, "metallo"), "Italian metallo must not match");

        const auto italian = nebbie::find_english_tokens(
            "Una stanza buia con una porta di legno. Il pavimento e' di pietra.");
        expect(italian.empty(), "typical Italian text should not match lexicon");

        const auto colored = nebbie::find_english_tokens("$$c0001door$$c0007");
        expect(contains_token(colored, "door"), "color codes should not block token match");

        const auto short_tokens = nebbie::find_english_tokens("to be or me");
        expect(short_tokens.empty(), "tokens shorter than 3 must be ignored");

        const auto numbers = nebbie::find_english_tokens("room 1550 key 12");
        expect(contains_token(numbers, "room"), "room should match");
        expect(contains_token(numbers, "key"), "key should match");
        expect(!contains_token(numbers, "1550"), "numbers must not match");

        expect(nebbie::join_english_tokens({"door", "metal"}) == "door, metal", "join tokens");

        nebbie::World english_world;
        expect(nebbie::create_room(english_world, 1550), "create english room");
        nebbie::Room* room = english_world.find_room(1550);
        expect(room != nullptr, "english room missing");
        room->name = "A dark tower room";
        room->description = "You are standing in a shadowy corridor.";
        nebbie::Exit exit;
        exit.direction = 3; // west
        exit.keyword = "door metal";
        exit.description = "A heavy metal door.";
        exit.to_room = 1550;
        room->exits.push_back(exit);

        expect(nebbie::create_mob(english_world, 1551), "create english mob");
        nebbie::Mobile* mob = english_world.find_mobile(1551);
        expect(mob != nullptr, "english mob missing");
        mob->name = "guard warrior";
        mob->short_descr = "a tall guard";
        mob->long_descr = "A tall guard stands here.\n";
        mob->description = "The guard looks dangerous.\n";

        expect(nebbie::create_object(english_world, 1552), "create english object");
        nebbie::GameObject* obj = english_world.find_object(1552);
        expect(obj != nullptr, "english object missing");
        obj->name = "sword blade";
        obj->short_descr = "a sharp sword";
        obj->description = "A sharp sword lies here.";

        const nebbie::ValidationReport en_report = nebbie::validate_world(english_world);
        expect(has_english_warning(en_report, nebbie::ValidationTarget::room, 1550),
               "expected english_text warning on room");
        expect(has_english_warning(en_report, nebbie::ValidationTarget::mob, 1551),
               "expected english_text warning on mob");
        expect(has_english_warning(en_report, nebbie::ValidationTarget::object, 1552),
               "expected english_text warning on object");

        bool found_exit_keyword = false;
        for (const auto& issue : en_report.issues) {
            if (issue.category == "english_text" && issue.target_vnum == 1550
                && issue.message.find("exit ovest keyword") != std::string::npos
                && issue.message.find("door") != std::string::npos) {
                found_exit_keyword = true;
                break;
            }
        }
        expect(found_exit_keyword, "expected exit ovest keyword english_text warning");

        nebbie::World italian_world;
        expect(nebbie::create_room(italian_world, 2000), "create italian room");
        nebbie::Room* it_room = italian_world.find_room(2000);
        expect(it_room != nullptr, "italian room missing");
        it_room->name = "Una stanza buia";
        it_room->description = "Il pavimento e' di pietra e una porta di legno chiude l'uscita.\n";
        nebbie::Exit it_exit;
        it_exit.direction = 3;
        it_exit.keyword = "porta legno";
        it_exit.description = "Una porta di legno massiccio.";
        it_exit.to_room = 2000;
        it_room->exits.push_back(it_exit);

        expect(nebbie::create_mob(italian_world, 2001), "create italian mob");
        nebbie::Mobile* it_mob = italian_world.find_mobile(2001);
        expect(it_mob != nullptr, "italian mob missing");
        it_mob->name = "guardia soldato";
        it_mob->short_descr = "una guardia";
        it_mob->long_descr = "Una guardia vigila l'ingresso.\n";
        it_mob->description = "Indossa un'armatura scura.\n";

        expect(nebbie::create_object(italian_world, 2002), "create italian object");
        nebbie::GameObject* it_obj = italian_world.find_object(2002);
        expect(it_obj != nullptr, "italian object missing");
        it_obj->name = "spada lama";
        it_obj->short_descr = "una spada affilata";
        it_obj->description = "Una spada affilata giace qui.";

        const nebbie::ValidationReport it_report = nebbie::validate_world(italian_world);
        expect(!has_english_warning(it_report, nebbie::ValidationTarget::room, 2000),
               "Italian room should not warn english_text");
        expect(!has_english_warning(it_report, nebbie::ValidationTarget::mob, 2001),
               "Italian mob should not warn english_text");
        expect(!has_english_warning(it_report, nebbie::ValidationTarget::object, 2002),
               "Italian object should not warn english_text");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
