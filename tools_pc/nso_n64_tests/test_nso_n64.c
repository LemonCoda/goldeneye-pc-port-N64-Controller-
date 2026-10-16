/* ROM-free regression tests against SDL's virtual joystick driver.
 * SDL 2.24+ is needed for virtual devices with vendor/product identifiers.
 * The mapping reproduces SDL2's HIDAPI N64 controller mapping, so these
 * exercise SDL's button/axis normalisation as well as our adapter. */
#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include "nson64.h"

#if !SDL_VERSION_ATLEAST(2, 24, 0)
#error These tests require SDL 2.24 or newer
#endif

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "%s:%d: %s (SDL: %s)\n", __FILE__, __LINE__, #expr, SDL_GetError()); \
    exit(1); \
} } while (0)

static int attachPad(Uint16 vendor, Uint16 product)
{
    SDL_VirtualJoystickDesc d;
    SDL_zero(d);
    d.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    d.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    d.naxes = 6;
    d.nbuttons = 16;
    d.vendor_id = vendor;
    d.product_id = product;
    d.name = "NSO N64 regression test";
    int index = SDL_JoystickAttachVirtualEx(&d);
    CHECK(index >= 0);
    char guid[33], mapping[1024];
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(index), guid, sizeof(guid));
    snprintf(mapping, sizeof(mapping),
        "%s,NSO N64 regression test,a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,"
        "start:b6,leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,"
        "dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,misc1:b15,"
        "leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,", guid);
    CHECK(SDL_GameControllerAddMapping(mapping) >= 0);
    CHECK(SDL_IsGameController(index));
    return index;
}

static void clearPad(SDL_Joystick *joy)
{
    for (int b = 0; b < 16; ++b) CHECK(SDL_JoystickSetVirtualButton(joy, b, 0) == 0);
    for (int a = 0; a < 6; ++a)
        CHECK(SDL_JoystickSetVirtualAxis(joy, a, a < 4 ? 0 : -32768) == 0);
    SDL_GameControllerUpdate();
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
}

int main(void)
{
    SDL_SetMainReady();
    nsoN64InitHints();
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    CHECK(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    CHECK(!nsoN64IsController(NULL));
    CHECK(nsoN64ReadButtons(NULL, 10000) == 0);
    CHECK(!nsoN64OptionsHeld(NULL));

    int index = attachPad(0x057e, 0x2019);
    SDL_GameController *pad = SDL_GameControllerOpen(index);
    CHECK(pad != NULL);
    CHECK(nsoN64IsController(pad));
    SDL_Joystick *joy = SDL_GameControllerGetJoystick(pad);
    clearPad(joy);
    CHECK(nsoN64ReadButtons(pad, 10000) == 0);

    /* Physical button order: A, B, Start, D-up/down/left/right, L, R,
     * C-up/left/right, Z, C-down. The expected bits are the N64 wire format.
     * All 16,384 combinations cover diagonals, opposing C-buttons, held
     * A+Z, aim+C-down, and absence of accidental use/fire/menu actions. */
    static const int sources[12] = { 0, 1, 6, 11, 12, 13, 14, 9, 10, 3, 2, 4 };
    static const unsigned bits[14] = {
        0x8000, 0x4000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
        0x0020, 0x0010, 0x0008, 0x0002, 0x0001, 0x2000, 0x0004
    };
    for (unsigned sample = 0; sample < (1u << 14); ++sample) {
        unsigned expected = 0;
        for (int b = 0; b < 14; ++b) {
            int down = (sample >> b) & 1;
            if (b < 12) CHECK(SDL_JoystickSetVirtualButton(joy, sources[b], (Uint8)down) == 0);
            else CHECK(SDL_JoystickSetVirtualAxis(joy, b - 8, down ? 32767 : -32768) == 0);
            if (down) expected |= bits[b];
        }
        SDL_GameControllerUpdate();
        CHECK(nsoN64ReadButtons(pad, 10000) == expected);
        CHECK(nsoN64ReadButtons(pad, 10000) == expected);   /* holds survive repeated polls */
        CHECK(!nsoN64OptionsHeld(pad));                    /* C-right never opens options */
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    }
    clearPad(joy);
    CHECK(nsoN64ReadButtons(pad, 10000) == 0);

    /* Capture is reserved for PC options. ZR and Home don't leak into the
     * original game. In particular ZR must not act as a modern stick-click. */
    CHECK(SDL_JoystickSetVirtualButton(joy, 15, 1) == 0);
    CHECK(SDL_JoystickSetVirtualButton(joy, 7, 1) == 0);
    CHECK(SDL_JoystickSetVirtualButton(joy, 5, 1) == 0);
    SDL_GameControllerUpdate();
    CHECK(nsoN64OptionsHeld(pad));
    CHECK(nsoN64ReadButtons(pad, 10000) == 0);
    clearPad(joy);
    CHECK(!nsoN64OptionsHeld(pad));

    CHECK(SDL_JoystickSetVirtualAxis(joy, 4, 32767) == 0);
    SDL_GameControllerUpdate();
    CHECK(nsoN64ReadButtons(pad, 32767) == 0);   /* strict trigger threshold */
    CHECK(nsoN64ReadButtons(pad, 32766) == 0x2000);
    SDL_GameControllerClose(pad);
    CHECK(SDL_JoystickDetachVirtual(index) == 0);

    /* Detection must survive reconnects and must not catch a Switch Pro,
     * a third-party device with the same product id, or an Xbox pad. */
    static const Uint16 devices[][2] = {
        { 0x057e, 0x2009 }, { 0x1234, 0x2019 }, { 0x045e, 0x028e }, { 0x057e, 0x2019 }
    };
    for (unsigned i = 0; i < sizeof(devices) / sizeof(devices[0]); ++i) {
        index = attachPad(devices[i][0], devices[i][1]);
        pad = SDL_GameControllerOpen(index);
        CHECK(pad != NULL);
        CHECK(nsoN64IsController(pad) == (i == 3));
        SDL_GameControllerClose(pad);
        CHECK(SDL_JoystickDetachVirtual(index) == 0);
    }
    SDL_Quit();
    puts("PASS: 16384 N64 button combinations, held input, Capture isolation, triggers, device identity and reconnects");
    return 0;
}
