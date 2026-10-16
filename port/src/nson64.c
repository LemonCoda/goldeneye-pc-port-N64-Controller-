#include "nson64.h"

void nsoN64InitHints(void)
{
    /* Before SDL_INIT_GAMECONTROLLER. The classic driver handles Bluetooth
     * initialisation, stick calibration, reconnects and rumble. Older SDL2
     * releases share the Switch driver. Environment overrides still win. */
    SDL_SetHint("SDL_JOYSTICK_HIDAPI_NINTENDO_CLASSIC", "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH, "1");
}

int nsoN64IsController(SDL_GameController *pad)
{
    /* SDL reports this pad as UNKNOWN, so controller type is insufficient.
     * Match Nintendo's VID/PID, never a generic name or every Switch pad. */
    return pad && SDL_GameControllerGetVendor(pad) == 0x057e &&
                  SDL_GameControllerGetProduct(pad) == 0x2019;
}

unsigned nsoN64ReadButtons(SDL_GameController *pad, int triggerThreshold)
{
    /* N64 OSContPad button bits; no game headers or ROM required here.
     * SDL2 HIDAPI mapping, cross-checked with libretro's N64 SDL2 profile:
     * https://github.com/libretro/retroarch-joypad-autoconfig/blob/master/sdl2/Nintendo%20N64%20Controller.cfg
     * ZR (SDL left-stick click), Home and Capture have no N64 equivalents. */
    static const struct { SDL_GameControllerButton source; unsigned bit; } map[] = {
        { SDL_CONTROLLER_BUTTON_A,             0x8000 },
        { SDL_CONTROLLER_BUTTON_B,             0x4000 },
        { SDL_CONTROLLER_BUTTON_START,         0x1000 },
        { SDL_CONTROLLER_BUTTON_DPAD_UP,       0x0800 },
        { SDL_CONTROLLER_BUTTON_DPAD_DOWN,     0x0400 },
        { SDL_CONTROLLER_BUTTON_DPAD_LEFT,     0x0200 },
        { SDL_CONTROLLER_BUTTON_DPAD_RIGHT,    0x0100 },
        { SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  0x0020 },
        { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 0x0010 },
        { SDL_CONTROLLER_BUTTON_Y,             0x0008 },
        { SDL_CONTROLLER_BUTTON_X,             0x0002 },
        { SDL_CONTROLLER_BUTTON_BACK,          0x0001 },
    };
    unsigned buttons = 0;
    if (!pad) return 0;
    for (unsigned i = 0; i < sizeof(map) / sizeof(map[0]); ++i)
        if (SDL_GameControllerGetButton(pad, map[i].source)) buttons |= map[i].bit;
    if (SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > triggerThreshold)
        buttons |= 0x2000;   /* Z */
    if (SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > triggerThreshold)
        buttons |= 0x0004;   /* C-down */
    return buttons;
}

int nsoN64OptionsHeld(SDL_GameController *pad)
{
    /* Capture opens PC options. SDL's BACK is C-right on this controller. */
#if SDL_VERSION_ATLEAST(2, 0, 14)
    return pad && SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_MISC1);
#else
    (void)pad;
    return 0;   /* F10 remains available on builds with older SDL headers. */
#endif
}
