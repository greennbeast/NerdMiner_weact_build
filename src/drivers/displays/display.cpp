#include "display.h"
#include <Arduino.h>
// Use time constants like SECOND_MS
#include "../../timeconst.h"

#ifdef NO_DISPLAY
DisplayDriver *currentDisplayDriver = &noDisplayDriver;
#endif

#ifdef M5STACK_DISPLAY
DisplayDriver *currentDisplayDriver = &m5stackDisplayDriver;
#endif

#ifdef WT32_DISPLAY
DisplayDriver *currentDisplayDriver = &wt32DisplayDriver;
#endif

#ifdef LED_DISPLAY
DisplayDriver *currentDisplayDriver = &ledDisplayDriver;
#endif

#ifdef OLED_042_DISPLAY
DisplayDriver *currentDisplayDriver = &oled042DisplayDriver;
#endif

#ifdef OLED_096_DISPLAY
DisplayDriver *currentDisplayDriver = &oled096DisplayDriver;
#endif

#ifdef T_DISPLAY
DisplayDriver *currentDisplayDriver = &tDisplayDriver;
#endif

#ifdef AMOLED_DISPLAY
DisplayDriver *currentDisplayDriver = &amoledDisplayDriver;
#endif

#ifdef DONGLE_DISPLAY
DisplayDriver *currentDisplayDriver = &dongleDisplayDriver;
#endif

#ifdef ESP32_2432S028R
DisplayDriver *currentDisplayDriver = &esp32_2432S028RDriver;
#endif

#ifdef ESP32_2432S028_2USB
DisplayDriver *currentDisplayDriver = &esp32_2432S028RDriver;
#endif

#ifdef T_QT_DISPLAY
DisplayDriver *currentDisplayDriver = &t_qtDisplayDriver;
#endif

#ifdef V1_DISPLAY
DisplayDriver *currentDisplayDriver = &tDisplayV1Driver;
#endif

#ifdef M5STICKC_DISPLAY
DisplayDriver *currentDisplayDriver = &m5stickCDriver;
#endif

#ifdef M5STICKCPLUS_DISPLAY
DisplayDriver *currentDisplayDriver = &m5stickCPlusDriver;
#endif

#ifdef T_HMI_DISPLAY
DisplayDriver *currentDisplayDriver = &t_hmiDisplayDriver;
#endif

#ifdef ST7735S_DISPLAY
DisplayDriver *currentDisplayDriver = &sp_kcDisplayDriver;
#endif


// Initialize the display
void initDisplay()
{
  currentDisplayDriver->initDisplay();
}

// Alternate screen state
void alternateScreenState()
{
  currentDisplayDriver->alternateScreenState();
}

// Alternate screen rotation
void alternateScreenRotation()
{
  currentDisplayDriver->alternateScreenRotation();
}

// Draw the loading screen
void drawLoadingScreen()
{
  currentDisplayDriver->loadingScreen();
}

// Draw the setup screen
void drawSetupScreen()
{
  currentDisplayDriver->setupScreen();
}

// Reset the current cyclic screen to the first one
void resetToFirstScreen()
{
  currentDisplayDriver->current_cyclic_screen = 0;
}

// Switches to the next cyclic screen without drawing it
void switchToNextScreen()
{
  currentDisplayDriver->current_cyclic_screen = (currentDisplayDriver->current_cyclic_screen + 1) % currentDisplayDriver->num_cyclic_screens;
  // reset the auto-rotate timer so manual switches won't be immediately overridden
  extern void _resetAutoRotateTimer(unsigned long nowMs);
  _resetAutoRotateTimer(millis());
}

// Draw the current cyclic screen
void drawCurrentScreen(unsigned long mElapsed)
{
  currentDisplayDriver->cyclic_screens[currentDisplayDriver->current_cyclic_screen](mElapsed);
}

// Animate the current cyclic screen
void animateCurrentScreen(unsigned long frame)
{
  currentDisplayDriver->animateCurrentScreen(frame);
}

// Do LED stuff
void doLedStuff(unsigned long frame)
{
  currentDisplayDriver->doLedStuff(frame);
}

// Internal auto-rotate state
static unsigned long s_lastScreenSwitchMs = 0;
static const unsigned long s_screenRotateMs = 30 * SECOND_MS; // rotate every 30 seconds by default

// Called by switchToNextScreen to reset the timer
void _resetAutoRotateTimer(unsigned long nowMs)
{
  s_lastScreenSwitchMs = nowMs ? nowMs : millis();
}

// Public helper to auto-rotate screens when needed. Call regularly (e.g. once per second)
void autoRotateScreensIfNeeded(unsigned long nowMs)
{
  if (!currentDisplayDriver) return;
  if (currentDisplayDriver->num_cyclic_screens <= 1) return;
  unsigned long now = nowMs ? nowMs : millis();
  // initialize timer on first use
  if (s_lastScreenSwitchMs == 0) s_lastScreenSwitchMs = now;
  if ((now - s_lastScreenSwitchMs) >= s_screenRotateMs)
  {
    // switch and immediately redraw
    currentDisplayDriver->current_cyclic_screen = (currentDisplayDriver->current_cyclic_screen + 1) % currentDisplayDriver->num_cyclic_screens;
    s_lastScreenSwitchMs = now;
    // redraw current screen
    currentDisplayDriver->cyclic_screens[currentDisplayDriver->current_cyclic_screen](0);
  }
}
