/*
Copyright (c) 2019-2022 Andreas T Jonsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <libretro.h>

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 200

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;

static void no_log(enum retro_log_level level, const char *fmt, ...) {
   (void)level; (void)fmt;
}

void retro_init(void) {
}

void retro_deinit(void) {
}

unsigned retro_api_version(void) {
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
   log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info) {
   unsigned char *ptr = (unsigned char*)info;
   for (unsigned long i = 0; i < sizeof(struct retro_system_info); i++)
      ptr[i] = 0;

   info->library_name     = "VirtualXT";
   info->library_version  = "0.6.2";
   info->valid_extensions = "img|bin";
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
   info->geometry.base_width   = VIDEO_WIDTH;
   info->geometry.base_height  = VIDEO_HEIGHT;
   info->geometry.max_width    = VIDEO_WIDTH;
   info->geometry.max_height   = VIDEO_HEIGHT;
   info->geometry.aspect_ratio = 4.0f / 3.0f;
}

void retro_set_environment(retro_environment_t cb) {
   environ_cb = cb;
   struct retro_log_callback logging;
   log_cb = cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging) ? logging.log : no_log;

   static const struct retro_controller_description controllers[] = {
      { "Gamepad", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
   };

   static const struct retro_controller_info ports[] = {
      { controllers, 1 },
      { NULL, 0 },
   };

   cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

   bool no_rom = true;
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
   video_cb = cb;
}

void retro_reset(void) {
}

static void update_input(void) {
}

static void check_variables(void) {
}

/*
static void audio_callback(void) {
   audio_cb();
}

static void audio_set_state(bool enable) {
  (void)enable;
}
*/

void retro_run(void) {
   update_input();

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();

   static unsigned fb[VIDEO_WIDTH*VIDEO_HEIGHT*4] = {0};
   video_cb(fb, VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH*4);
}

void retro_keyboard_event(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers) {
   (void)down; (void)keycode; (void)character; (void)key_modifiers;
}

bool retro_load_game(const struct retro_game_info *info) {
   (void)info;

    /*
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0 },
   };
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
   */

   struct retro_keyboard_callback kbdesk = {&retro_keyboard_event};
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kbdesk);

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
      log_cb(RETRO_LOG_ERROR, "XRGB8888 is not supported!\n");
      return false;
   }

   //struct retro_audio_callback audio_cb = { audio_callback, audio_set_state };
   //use_audio_cb = environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_cb);

   check_variables();
   return true;
}

void retro_unload_game(void) {
}

unsigned retro_get_region(void) {
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
   (void)type; (void)info; (void)num;
   return false;
}

size_t retro_serialize_size(void) {
   return 0;
}

bool retro_serialize(void *data, size_t size) {
   (void)data; (void)size;
   return false;
}

bool retro_unserialize(const void *data, size_t size) {
   (void)data; (void)size;
   return false;
}

void *retro_get_memory_data(unsigned id) {
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id) {
   (void)id;
   return 0;
}

void retro_cheat_reset(void) {
}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {
   (void)index; (void)enabled; (void)code;
}
