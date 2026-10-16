#ifndef GE_NSON64_H
#define GE_NSON64_H

#include <SDL.h>

/* Nintendo Switch Online N64 controller. SDL's normalised mapping uses
 * X/Y/Back/right trigger for C-left/up/right/down, and left trigger for Z.
 * Keep those digital buttons separate; opposing C-buttons can be held. */
void nsoN64InitHints(void);
int nsoN64IsController(SDL_GameController *pad);
unsigned nsoN64ReadButtons(SDL_GameController *pad, int triggerThreshold);
int nsoN64OptionsHeld(SDL_GameController *pad);

#endif
