# Nintendo Switch Online Nintendo 64 controller

This fork recognises Nintendo's controller by vendor/product ID `057e:2019`
and automatically uses its original N64 layout. This is per controller:
Xbox, PlayStation and Switch Pro controllers retain their existing mappings.
The game's saved control style is respected without changing the global
`Input.ControlScheme` setting or another player's layout.
The last N64 layout is retained during a disconnect so Bluetooth reconnects
do not overwrite the chosen game style with a modern preset.

## Windows 11 over Bluetooth

1. Pair the controller in Windows Bluetooth settings using its sync button.
2. Use this fork's executable with its accompanying SDL2 library. SDL 2.24
   or newer is recommended; this change is tested with SDL 2.32.10.
3. Launch the game directly. If launching through Steam, disable Steam Input
   for this game so SDL receives the actual Nintendo device. Other software
   that presents an emulated Xbox controller also hides its native identity.
4. Choose a **1.x** controller style in the watch or PC Controller options.
   Start with **1.1 Honey** for the original default layout. Existing saves
   may retain **1.2 Solitaire** from the upstream port; select the style you
   prefer. The upstream limitations of 2.x dual-controller styles remain.

You still supply your own game ROM as described in the main README. No ROM
or extracted assets are needed to build the code or run the input tests.

## Controls

| Physical control | Game input |
| --- | --- |
| Stick | N64 analogue stick, with the left-stick deadzone setting |
| A / B / Start | N64 A / B / Start, including held-button combinations |
| C-up / down / left / right | Corresponding independent N64 C-buttons |
| D-pad | N64 D-pad |
| Z / L / R | N64 Z / L / R |
| Capture | Open/close PC options (F10 also works) |
| Home / ZR | No original N64 game input |

The game assigns actions according to the chosen original style. For 1.1
Honey, Z fires, R aims, A changes weapons, and B interacts or reloads.
The original crouch, zoom, weapon-cycle and tank controls remain game-owned.
In PC options, use the stick or D-pad to navigate, A to select, B to go back,
and Start or Capture to close. SDL's X alias (physical C-left) also selects.

The controller page shows the game's original layouts for this device.
Modern action rebinding and twin-stick presets do not apply to this physical
N64 layout; their settings are hidden. Use the game's own **Look up/down**
option for stick direction: **Reverse** and **Upright** are both honored.
The port leaves this setting unchanged while an N64 layout is in use,
including during a controller disconnect. Rumble uses the existing SDL Rumble Pak path when the driver
reports support; it still needs a real Bluetooth hardware test.

## Why a dedicated mapping is needed

SDL's N64 HIDAPI mapping exposes C-left as X, C-up as Y, C-right as Back,
C-down as the right trigger, Z as the left trigger, and ZR as a left-stick
click. Feeding these through the upstream Xbox mapping assigns unintended
actions and lets C-right open PC options. This fork reads each input
independently and reserves Capture for the PC menu.

References: [SDL2 controller mapping](https://github.com/libsdl-org/SDL/blob/SDL2/src/joystick/SDL_gamecontroller.c)
and [libretro's SDL2 N64 controller profile](https://github.com/libretro/retroarch-joypad-autoconfig/blob/master/sdl2/Nintendo%20N64%20Controller.cfg).

## Automated checks

The standalone SDL virtual-controller test needs SDL 2.24+ and no ROM:

```sh
cmake -S tools_pc/nso_n64_tests -B build-nso-tests
cmake --build build-nso-tests
ctest --test-dir build-nso-tests --output-on-failure
```

On Windows, use the same MinGW/SDL development environment as the game and
put its `SDL2.dll` beside the test executable (or on PATH). The tests cover
all 16,384 combinations of the original buttons, held input, complete release,
Capture isolation, trigger boundaries, device identity, and reconnects.
They verify the SDL adapter, not the Bluetooth transport or physical rumble.

The watch-option regression links the actual Windows engine objects without
starting the game or using a ROM. Configure a full x86-64 MinGW Makefiles game
build with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, build it, and place `SDL2.dll`
in that build directory. Then run:

```sh
python tools_pc/nso_n64_tests/run_look_policy_windows.py build-nso-n64
```

It checks both look directions over repeated policy ticks, mixed controllers,
keyboard/player routing, disconnect/reconnect, replacement by a modern pad,
and the inactive-stage guards. Generated test files stay inside the build
directory; the game executable and saves are unchanged.

## Physical checks still needed

- Detect the controller when paired before launch and when connected later.
- Verify every button, stick centering, full travel, and diagonal movement.
- Test 1.1 Honey and 1.2 Solitaire, then confirm a selected style survives
  opening the watch, closing PC options, and returning to a level.
- Select Look up/down: Upright and Reverse in the watch, verify the pitch
  direction, then close/reopen the watch and reload the save.
- Hold and combine C-buttons; C-right must never open PC options. Test
  Z fire, R aim, A weapon cycling, A+Z reverse cycling, and B use/reload.
- Verify aiming, crouching, sniper zoom, tank entry/exit, and rumble.
- Open/close PC options with Capture and navigate using only the controller.
- Disconnect/reconnect during play and test alongside a modern controller
  in split-screen, including keyboard/mouse as player 1.
