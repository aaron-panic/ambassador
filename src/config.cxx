#include "config.hxx"

const char* amb::config::APP_TITLE = "Ambassador";
const char* amb::config::APP_VERSION = "0.1";
const char* amb::config::APP_IDENTIFIER = "io.scavengers.ambassador";

const int amb::config::DEFAULT_APP_WIDTH = 800;
const int amb::config::DEFAULT_APP_HEIGHT = 600;

const u64 amb::config::GAME_SPEED = 60;
const u64 amb::config::UPDATE_SPEED = 1000 / GAME_SPEED;

const u8 amb::game::MAP_TILE_SIZE = 50;

const u8 amb::data::CHUNK_TYPE_LENGTH = 4;
const u8 amb::data::MAGIC_LENGTH = 8;
