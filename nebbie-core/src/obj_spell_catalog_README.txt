obj_spell_catalog_data.inc is derived from NebbieArcane/Server spell_parser.cpp (spells[]).
Regenerate after server spell list changes (read-only clone; do not push to Server from izanagi):

  python3 scripts/extract-server-spells.py /path/to/Server/src/spell_parser.cpp \
    > nebbie-core/src/obj_spell_catalog_data.inc
