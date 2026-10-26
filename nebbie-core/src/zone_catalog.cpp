#include "nebbie/zone_catalog.hpp"

namespace nebbie {

namespace {

const char* kResetModes[] = {
    "0 — Never reset",
    "1 — Reset when deserted",
    "2 — Normal reset",
};

const char* kResetCommands[] = {
    "M — Load mobile",
    "O — Load object",
    "G — Give object to mob",
    "E — Equip mobile",
    "P — Put object in object",
    "D — Set door state",
    "C — Mob command",
    "H — Hour / time trigger",
    "* — Comment line",
};

const char* kEquipPositions[] = {
    "0 light", "1 rfinger", "2 lfinger", "3 neck1", "4 neck2", "5 body", "6 head", "7 legs",
    "8 feet", "9 hands", "10 arms", "11 shield", "12 about", "13 waist", "14 rwrist", "15 lwrist",
    "16 wield", "17 hold", "18 inv",
};

const char* kDoorStates[] = {
    "0 open", "1 closed", "2 closed and locked",
};

} // namespace

std::vector<std::pair<int, std::string>> zone_reset_mode_choices() {
    std::vector<std::pair<int, std::string>> choices;
    const std::size_t count = sizeof(kResetModes) / sizeof(kResetModes[0]);
    for (std::size_t i = 0; i < count; ++i) {
        choices.emplace_back(static_cast<int>(i), kResetModes[i]);
    }
    return choices;
}

std::vector<std::pair<char, std::string>> zone_reset_command_choices() {
    std::vector<std::pair<char, std::string>> choices;
    const char commands[] = {'M', 'O', 'G', 'E', 'P', 'D', 'C', 'H', '*'};
    const std::size_t count = sizeof(commands) / sizeof(commands[0]);
    for (std::size_t i = 0; i < count; ++i) {
        choices.emplace_back(commands[i], kResetCommands[i]);
    }
    return choices;
}

std::vector<std::pair<int, std::string>> zone_equip_position_choices() {
    std::vector<std::pair<int, std::string>> choices;
    const std::size_t count = sizeof(kEquipPositions) / sizeof(kEquipPositions[0]);
    for (std::size_t i = 0; i < count; ++i) {
        choices.emplace_back(static_cast<int>(i), kEquipPositions[i]);
    }
    return choices;
}

std::vector<std::pair<int, std::string>> zone_door_state_choices() {
    std::vector<std::pair<int, std::string>> choices;
    const std::size_t count = sizeof(kDoorStates) / sizeof(kDoorStates[0]);
    for (std::size_t i = 0; i < count; ++i) {
        choices.emplace_back(static_cast<int>(i), kDoorStates[i]);
    }
    return choices;
}

std::string zone_reset_mode_label(const int reset_mode) {
    const std::size_t count = sizeof(kResetModes) / sizeof(kResetModes[0]);
    if (reset_mode >= 0 && static_cast<std::size_t>(reset_mode) < count) {
        return kResetModes[reset_mode];
    }
    return "reset_mode #" + std::to_string(reset_mode);
}

std::string zone_reset_command_legend(const char command) {
    switch (command) {
    case 'M':
        return "Carica mob: if_flag | mob vnum | max MONDO | room vnum | max STANZA\n"
               "0 = senza limite. Max mondo = quante copie del mob esistono in tutto "
               "il mud; max stanza = quante in quella room.";
    case 'O':
        return "Carica oggetto a terra: if_flag | obj vnum | max MONDO | room vnum | max STANZA\n"
               "0 = senza limite. Max mondo = copie LIVE (non il totale storico). "
               "Max stanza = solo a terra in quella room. NICE_LIMITED raddoppia il "
               "max mondo. Arg2 vale anche come tetto stanza. Tetto alto = puo' "
               "ripoppare spesso finche' non si raggiunge il limite live.";
    case 'G':
        return "Dai oggetto all'ultimo mob caricato (M): if_flag | obj vnum | max MONDO\n"
               "0 = senza limite. Solo pezzi live; loot/rent possono liberare slot "
               "e far ripoppare al reset successivo.";
    case 'E':
        return "Equipaggia l'ultimo mob caricato (M): if_flag | obj vnum | max MONDO | "
               "posizione (0-18)\n"
               "0 = senza limite. Serve M riuscita (se il mob e' gia' al max, gli E "
               "con if_flag=1 non partono). Max mondo = live; tetto alto (es. 10) = "
               "ripop frequente. Pos 16=wield, 17=hold.";
    case 'P':
        return "Metti oggetto in contenitore: if_flag | obj vnum | max MONDO | "
               "container obj vnum\n"
               "0 = senza limite. Il contenitore deve gia' esistere (ultimo O/E/G). "
               "Stesse regole di presenza live del max mondo.";
    case 'D':
        return "Stato porta: if_flag | room vnum | direzione (0-5) | "
               "stato (0=aperta, 1=chiusa, 2=chiusa+serratura)";
    case 'C':
        return "Comando mob: if_flag | mob vnum | numero comando";
    case 'H':
        return "Trigger orario: if_flag | room vnum | ora/valore tempo";
    case '*':
        return "Riga commento *<testo> nel .zon (il server non la esegue).";
    default:
        return "Seleziona un comando di reset";
    }
}

ResetArgLabels zone_reset_arg_labels(const char command) {
    ResetArgLabels labels;
    labels.if_flag = "if_flag (0=sempre, 1=solo se il comando precedente e' riuscito)";
    switch (command) {
    case 'M':
        labels.arg1 = "mob vnum";
        labels.arg2 = "max mondo (0=illimitato)";
        labels.arg3 = "room vnum";
        labels.arg4 = "max stanza (0=illimitato)";
        break;
    case 'O':
        labels.arg1 = "obj vnum";
        labels.arg2 = "max mondo (0=illimitato; conta anche come tetto stanza)";
        labels.arg3 = "room vnum";
        labels.arg4 = "max stanza (0=illimitato)";
        break;
    case 'G':
        labels.arg1 = "obj vnum";
        labels.arg2 = "max mondo (0=illimitato)";
        labels.arg3 = "(unused)";
        labels.arg4 = "(unused)";
        break;
    case 'E':
        labels.arg1 = "obj vnum";
        labels.arg2 = "max mondo (0=illimitato)";
        labels.arg3 = "equip position";
        labels.arg4 = "(unused)";
        break;
    case 'P':
        labels.arg1 = "obj vnum";
        labels.arg2 = "max mondo (0=illimitato)";
        labels.arg3 = "container obj vnum";
        labels.arg4 = "(unused)";
        break;
    case 'D':
        labels.arg1 = "room vnum";
        labels.arg2 = "exit direction (0-5)";
        labels.arg3 = "door state (0-2)";
        labels.arg4 = "(unused)";
        break;
    case 'C':
        labels.arg1 = "mob vnum";
        labels.arg2 = "command number";
        labels.arg3 = "(unused)";
        labels.arg4 = "(unused)";
        break;
    case 'H':
        labels.arg1 = "room vnum";
        labels.arg2 = "hour / time";
        labels.arg3 = "(unused)";
        labels.arg4 = "(unused)";
        break;
    default:
        labels.arg1 = "arg1";
        labels.arg2 = "arg2";
        labels.arg3 = "arg3";
        labels.arg4 = "arg4";
        break;
    }
    return labels;
}

} // namespace nebbie
