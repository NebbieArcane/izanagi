#include "nebbie/english_lexicon.hpp"

#include <cctype>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace nebbie {
namespace {

// Curated stock-MUD English tokens (length >= 3). Expand as needed.
const std::unordered_set<std::string>& english_lexicon() {
    static const std::unordered_set<std::string> words = {
        // Common English function / prose words
        "the",      "you",      "are",      "and",      "for",      "with",
        "from",     "this",     "that",     "here",     "there",    "your",
        "have",     "has",      "was",      "were",     "will",     "into",
        "onto",     "upon",     "over",     "under",    "through",  "around",
        "about",    "above",    "below",    "some",     "many",     "much",
        "very",     "only",     "also",     "just",     "like",     "look",
        "looks",    "looking",  "see",      "seem",     "seems",    "feel",
        "feels",    "smell",    "smells",   "hear",     "hears",    "stand",
        "stands",   "standing", "sitting",  "lying",    "leads",    "leading",
        "open",     "opens",    "closed",   "close",    "locked",   "unlock",
        // Directions as look-text / keywords (not dir commands)
        "north",    "south",    "east",     "west",     "upward",   "downward",
        "upwards",  "downwards","northwest","northeast","southwest","southeast",
        "inside",   "outside",  "beyond",   "farther",  "further",  "nearby",
        // Architecture / places
        "door",     "doors",    "gate",     "gates",    "portal",   "portals",
        "exit",     "exits",    "entrance", "hallway",  "corridor", "passage",
        "passages", "room",     "rooms",    "chamber",  "chambers", "hall",
        "halls",    "tower",    "towers",   "castle",   "forest",   "grove",
        "shadow",   "shadows",  "shadowy",  "mist",     "mists",    "fog",
        "dark",     "darkness", "light",    "bright",   "dim",      "stone",
        "stones",   "metal",    "wooden",   "wood",     "iron",     "steel",
        "bronze",   "silver",   "golden",   "gold",     "glass",    "wall",
        "walls",    "floor",    "ceiling",  "stair",    "stairs",   "steps",
        "bridge",   "river",    "water",    "lake",     "mountain", "mountains",
        "cave",     "caves",    "cavern",   "temple",   "church",   "city",
        "town",     "village",  "house",    "houses",   "shop",     "shops",
        "tavern",   "inn",      "kitchen",  "cellar",   "basement", "dungeon",
        "prison",   "cell",     "cells",    "office",   "library",  "school",
        "academy",  "guild",    "market",   "street",   "road",     "path",
        "paths",    "trail",    "trails",   "garden",   "field",    "fields",
        "plain",    "plains",   "desert",   "swamp",    "marsh",    "ocean",
        "sea",      "shore",    "beach",    "island",   "ship",     "ships",
        "boat",     "boats",    "moat",     "drawbridge","portcullis","battlement",
        "turret",   "turrets",  "spire",    "spires",   "vault",    "vaults",
        "dome",     "domes",    "pillar",   "pillars",  "column",   "columns",
        "arch",     "arches",   "hinge",    "hinges",   "handle",   "handles",
        "latch",    "latches",  "padlock",  "keyhole",  "frame",    "frames",
        // Creatures / roles
        "wizard",   "wizards",  "mage",     "mages",    "priest",   "priests",
        "warrior",  "guard",    "guards",   "soldier",  "soldiers", "king",
        "queen",    "lord",     "lady",     "master",   "mistress", "apprentice",
        "student",  "teacher",  "creature", "creatures","monster",  "monsters",
        "beast",    "beasts",   "animal",   "animals",  "human",    "humans",
        "elf",      "elves",    "dwarf",    "dwarves",  "orc",      "orcs",
        "dragon",   "dragons",  "ghost",    "ghosts",   "undead",   "skeleton",
        "zombie",   "giant",    "giants",   "troll",    "trolls",   "goblin",
        "goblins",  "kobold",   "kobolds",  "ogre",     "ogres",    "demon",
        "demons",   "devil",    "devils",   "angel",    "angels",   "spirit",
        "spirits",  "thief",    "thieves",  "rogue",    "assassin", "knight",
        "knights",  "paladin",  "cleric",   "druid",    "ranger",   "bard",
        "merchant", "shopkeeper","bartender","innkeeper","blacksmith","captain",
        "general",  "army",     "traveler", "traveller","visitor",  "visitors",
        "keeper",   "keepers",  "warden",   "jailer",   "prisoner", "prisoners",
        "servant",  "servants", "witch",    "witches",  "warlock",  "sorcerer",
        "sorceress","enchanter","alchemist","healer",   "healers",
        // Items
        "sword",    "swords",   "dagger",   "daggers",  "axe",      "axes",
        "mace",     "shield",   "shields",  "armor",    "armour",   "helmet",
        "boots",    "cloak",    "robe",     "robes",    "ring",     "rings",
        "amulet",   "potion",   "potions",  "scroll",   "scrolls",  "book",
        "books",    "chest",    "chests",   "box",      "boxes",    "bag",
        "bags",     "key",      "keys",     "torch",    "torches",  "lantern",
        "candle",   "table",    "tables",   "chair",    "chairs",   "bed",
        "beds",     "desk",     "desks",    "shelf",    "shelves",  "window",
        "windows",  "curtain",  "curtains", "carpet",   "rug",      "painting",
        "statue",   "statues",  "fountain", "altar",    "throne",   "crown",
        "jewel",    "jewels",   "treasure", "coins",    "money",    "food",
        "drink",    "wine",     "beer",     "bread",    "meat",     "fruit",
        "weapon",   "weapons",  "gloves",   "belt",     "cape",     "hood",
        "mask",     "mirror",   "crystal",  "orb",      "staff",    "wand",
        "rod",      "bow",      "arrow",    "arrows",   "quiver",   "crossbow",
        "spear",    "spears",   "lance",    "whip",     "chain",    "chains",
        "rope",     "ropes",    "ladder",   "trap",     "traps",    "sign",
        "signs",    "notice",   "message",  "messages",
        // Nature / atmosphere
        "tree",     "trees",    "leaf",     "leaves",   "branch",   "branches",
        "root",     "roots",    "flower",   "flowers",  "grass",    "moss",
        "vine",     "vines",    "thorn",    "thorns",   "bush",     "bushes",
        "rock",     "rocks",    "boulder",  "sand",     "mud",      "dust",
        "ash",      "ashes",    "smoke",    "flame",    "flames",   "fire",
        "ice",      "snow",     "rain",     "storm",    "wind",     "blood",
        "bone",     "bones",    "skull",    "flesh",    "skin",     "hair",
        "eyes",     "eye",      "hand",     "hands",    "foot",     "feet",
        "head",     "body",     "bodies",   "voice",    "voices",   "sound",
        "sounds",   "noise",    "noises",   "echo",     "echoes",
        // Adjectives common in stock rooms
        "small",    "large",    "huge",     "tiny",     "narrow",   "wide",
        "long",     "short",    "high",     "tall",     "deep",     "shallow",
        "old",      "ancient",  "new",      "young",    "broken",   "cracked",
        "dusty",    "dirty",    "clean",    "empty",    "full",     "strange",
        "weird",    "odd",      "silent",   "quiet",    "noisy",    "loud",
        "cold",     "hot",      "warm",     "wet",      "dry",      "magic",
        "magical",  "arcane",   "secret",   "hidden",   "invisible","visible",
        "obvious",  "clear",    "faint",    "strong",   "weak",     "thick",
        "thin",     "heavy",    "soft",     "hard",     "smooth",   "rough",
        "sharp",    "dull",     "black",    "white",    "grey",     "gray",
        "red",      "blue",     "green",    "yellow",   "orange",   "purple",
        "brown",    "pink",     "crimson",  "scarlet",  "azure",    "emerald",
        "amber",    "ivory",    "ebony",    "copper",   "brass",    "rusty",
        "ruined",   "destroyed","damaged",  "intact",   "beautiful","ugly",
        "hideous",  "lovely",   "pleasant", "dangerous","deadly",   "lethal",
        "poisonous","cursed",   "blessed",  "holy",     "unholy",   "evil",
        "good",     "neutral",  "glowing",  "shining",  "sparkling","burning",
        "frozen",   "molten",   "rotten",   "rotting",  "decayed",  "filthy",
        "pristine", "polished", "shiny",    "weathered","faded",    "worn",
        // Misc stock verbs / nouns
        "kill",     "killed",   "fight",    "fighting", "attack",   "attacks",
        "escape",   "flee",     "running",  "walking",  "enter",    "entering",
        "leave",    "leaving",  "arrive",   "arrived",  "waiting",  "sleeping",
        "dream",    "dreams",   "awake",    "alive",    "afraid",   "fear",
        "angry",    "happy",    "sad",      "hungry",   "thirsty",  "tired",
        "powerful", "power",    "powers",   "spell",    "spells",   "enchanted",
        "quest",    "quests",   "mission",  "missions", "treasure", "glory",
        "honor",    "honour",   "destiny",  "fate",     "enemy",    "enemies",
        "friend",   "friends",  "ally",     "allies",   "war",      "battle",
        "weapon",   "armor",    "magic",    "scroll",   "potion",
    };
    return words;
}

std::string to_lower_ascii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool is_ascii_alpha(const unsigned char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

} // namespace

std::vector<std::string> find_english_tokens(const std::string& text) {
    const auto& lexicon = english_lexicon();
    std::set<std::string> found;
    std::string token;
    token.reserve(32);

    const auto flush = [&]() {
        if (token.size() < 3) {
            token.clear();
            return;
        }
        const std::string lower = to_lower_ascii(token);
        token.clear();
        if (lexicon.find(lower) != lexicon.end()) {
            found.insert(lower);
        }
    };

    for (std::size_t i = 0; i < text.size();) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        // Skip mud color codes $c#### / $$c####
        if (ch == '$') {
            flush();
            if (i + 1 < text.size() && text[i + 1] == '$') {
                ++i;
            }
            if (i + 1 < text.size() && (text[i + 1] == 'c' || text[i + 1] == 'C')) {
                i += 2;
                int digits = 0;
                while (i < text.size() && digits < 4
                       && std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
                    ++i;
                    ++digits;
                }
                continue;
            }
            ++i;
            continue;
        }
        if (is_ascii_alpha(ch)) {
            token.push_back(static_cast<char>(ch));
            ++i;
            continue;
        }
        flush();
        ++i;
    }
    flush();

    return {found.begin(), found.end()};
}

std::string join_english_tokens(const std::vector<std::string>& tokens) {
    std::string out;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += tokens[i];
    }
    return out;
}

} // namespace nebbie
