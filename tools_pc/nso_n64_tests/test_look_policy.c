/* Engine-linked, ROM-free regression for the D575 watch option override.
 * Uses real SDL devices, input seat routing, game option accessors and the
 * production watch policy. No game threads, graphics or save files needed. */
#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <ultra64.h>
#include <bondconstants.h>
#include "front.h"
#include "player.h"
#include "options.h"
#include "input.h"
#include "system.h"
#include "watchsettings.h"

extern void set_cur_player_look_vertical_inverted(u32 value);

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); \
    exit(1); \
} } while (0)

static struct player players[2];

static SDL_JoystickID attachPad(int native)
{
    SDL_VirtualJoystickDesc d;
    SDL_zero(d);
    d.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    d.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    d.naxes = 2;
    d.nbuttons = 2;
    d.vendor_id = native ? 0x057e : 0x1234;
    d.product_id = native ? 0x2019 : 0x5678;
    d.name = "Look policy regression";
    int index = SDL_JoystickAttachVirtualEx(&d);
    CHECK(index >= 0);
    char guid[33], mapping[256];
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(index), guid, sizeof(guid));
    snprintf(mapping, sizeof(mapping), "%s,Look policy test,a:b0,b:b1,leftx:a0,lefty:a1,", guid);
    CHECK(SDL_GameControllerAddMapping(mapping) >= 0);
    return SDL_JoystickGetDeviceInstanceID(index);
}

static void detachPad(SDL_JoystickID instance)
{
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_JoystickGetDeviceInstanceID(i) == instance) {
            CHECK(SDL_JoystickDetachVirtual(i) == 0);
            inputRescanPads();
            return;
        }
    }
    CHECK(0);
}

static void checkBothChoices(int native)
{
    CHECK(inputUsesNsoN64Layout() == native);
    /* Exercise re-selection and repeated frame policies, not just one read. */
    const int choices[] = { 1, 0, 1 };
    for (unsigned i = 0; i < sizeof(choices) / sizeof(choices[0]); ++i) {
        set_cur_player_look_vertical_inverted(choices[i]);
        for (int frame = 0; frame < 120; ++frame) {
            watchSettingsApplyLookPolicy();
            CHECK(get_cur_player_look_vertical_inverted() == (native ? choices[i] : 0));
        }
    }
}

int main(int argc, char **argv)
{
    sysSetArgs(argc, argv);
    SDL_SetMainReady();
    /* Keep real hardware out of this deterministic virtual-device test. */
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "0");
    SDL_SetHint(SDL_HINT_DIRECTINPUT_ENABLED, "0");
    SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");
    CHECK(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    CHECK(SDL_NumJoysticks() == 0);
    current_menu = MENU_RUN_STAGE;
    g_playerPointers[0] = g_CurrentPlayer = &players[0];

    inputRescanPads();
    checkBothChoices(0);                         /* keyboard / no native pad */
    SDL_JoystickID n64 = attachPad(1);
    inputRescanPads();
    checkBothChoices(1);                         /* solo N64 */
    g_playerPointers[1] = &players[1];
    inputRescanPads();
    checkBothChoices(1);                         /* keyboard P1, N64 P2 */
    attachPad(0);
    inputRescanPads();
    g_CurrentPlayer = &players[1];
    checkBothChoices(1);                         /* N64 P1, modern P2 current */
    detachPad(n64);
    checkBothChoices(1);                         /* reserved N64 seat stays native */
    n64 = attachPad(1);
    inputRescanPads();
    checkBothChoices(1);                         /* reconnected N64 */
    detachPad(n64);
    attachPad(0);
    inputRescanPads();
    checkBothChoices(0);                         /* replaced by modern pad */

    set_cur_player_look_vertical_inverted(1);
    current_menu = MENU_FILE_SELECT;
    watchSettingsApplyLookPolicy();
    CHECK(get_cur_player_look_vertical_inverted() == 1);
    current_menu = MENU_RUN_STAGE;
    g_CurrentPlayer = NULL;
    watchSettingsApplyLookPolicy();
    CHECK(get_cur_player_look_vertical_inverted() == 1);

    inputDestroy();
    SDL_Quit();
    puts("PASS: Reverse/Upright, repeated ticks, solo, mixed seats, keyboard P1, disconnect/reconnect, replacement, inactive stage");
    return 0;
}
