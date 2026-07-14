# m5stack/M5GFX issue #152: papers3 v0.2.12 eInk display stops working

- URL: https://github.com/m5stack/M5GFX/issues/152
- State: `closed`
- Created: 2025-09-19T15:57:01Z
- Updated: 2025-09-24T23:52:55Z
- Author: `LindezaGrey`

## Issue body

When i update to v0.2.12 the screen of the paperS3 starts to behave erratic (inverted colors, smearing etc.). Downgrade to v0.2.11 fixes this

## Comments

### Comment 1: lovyan03 at 2025-09-21T03:47:59Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3315477158

Hello, @LindezaGrey
I apologize for the inconvenience.
I have released version 0.2.13 which fixes this issue.

### Comment 2: LindezaGrey at 2025-09-22T09:12:12Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3317729825

Thank you, this fixes the problem!

### Comment 3: LindezaGrey at 2025-09-22T13:51:58Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3319176612

Sorry @lovyan03 i have to reopen again. I noticed that the paperS3 now shows heavy ghosting. Here you see pictures of v0.2.11 vs v.0.2.13

**Version 2.11**

![Image](https://github.com/user-attachments/assets/b69f0656-432f-49cc-9f2d-2f9796f581b9)

![Image](https://github.com/user-attachments/assets/989c2315-20ad-415d-9704-7f2963f7bc98)

-> no ghosting after switching scenes



**Version 2.13**

![Image](https://github.com/user-attachments/assets/6bd152b7-6f1a-4bc1-98ee-7329566d3358)

![Image](https://github.com/user-attachments/assets/632a1b2f-15bb-4359-866a-993e02ae89e0)

-> visible "residue"

### Comment 4: lovyan03 at 2025-09-22T14:04:54Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3319245884

Is there any simple code to reproduce this issue?
I would like to know the epd mode and functions used for drawing, etc.

### Comment 5: lovyan03 at 2025-09-22T14:54:03Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3319551138

I've updated the develop branch with some adjustments that may improve this issue.
I hope you'll try them out and see if they work.

### Comment 6: lovyan03 at 2025-09-23T05:13:26Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3322438981

I noticed that the display was off by one pixel overall since the last update, so I've pushed a fix to the develop branch.
I hope this will improve your issue.

### Comment 7: LindezaGrey at 2025-09-24T10:33:03Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3327679423

Dear @lovyan03 ,

i tried to create a minimal reproducible sketch, which you see below. I tested the v0.2.11 against the v0.2.14

this is v0.2.14 after approx. 1 minute (note the smearing on the bottom right)
![Image](https://github.com/user-attachments/assets/3ecd2d2a-9469-4264-a61a-93e41d321590)

in contrast this is the v0.2.11

![Image](https://github.com/user-attachments/assets/074da009-1003-4113-b0d4-596b7e6a068e)

to reproduce you can use the following:

```c++
#include <M5Unified.h>
#include <M5GFX.h>

// Screen layout constants from config.h
static const int SCREEN_WIDTH = 960;
static const int SCREEN_HEIGHT = 540;

bool showTopLeft = true;

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    M5.Display.setRotation(1); // landscape
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    M5.Display.clear(WHITE);

    M5.Display.setTextColor(BLACK, WHITE);
    M5.Display.setTextSize(5);
}

void loop()
{
    if (showTopLeft)
    {
        int prevTextWidth = 15 * 6 * 5;
        int prevTextHeight = 8 * 5;
        int prevX = SCREEN_WIDTH - prevTextWidth;
        int prevY = SCREEN_HEIGHT - prevTextHeight;
        M5.Display.setClipRect(prevX, prevY, prevTextWidth, prevTextHeight);
        M5.Display.fillRect(prevX, prevY, prevTextWidth, prevTextHeight, WHITE);
        M5.Display.clearClipRect();

        int textWidth = 12 * 6 * 5;
        int textHeight = 8 * 5;
        M5.Display.setClipRect(0, 0, textWidth, textHeight);
        M5.Display.fillRect(0, 0, textWidth, textHeight, WHITE);

        M5.Display.setCursor(0, 0);
        M5.Display.print("Top Left Text");

        M5.Display.clearClipRect();
    }
    else
    {
        int prevTextWidth = 12 * 6 * 5;
        int prevTextHeight = 8 * 5;
        M5.Display.setClipRect(0, 0, prevTextWidth, prevTextHeight);
        M5.Display.fillRect(0, 0, prevTextWidth, prevTextHeight, WHITE);
        M5.Display.clearClipRect();

        int textWidth = 15 * 6 * 5;
        int textHeight = 8 * 5;
        int x = SCREEN_WIDTH - textWidth;
        int y = SCREEN_HEIGHT - textHeight;
        M5.Display.setClipRect(x, y, textWidth, textHeight);
        M5.Display.fillRect(x, y, textWidth, textHeight, WHITE);

        M5.Display.setCursor(x, y);
        M5.Display.print("Bottom Right Text");

        M5.Display.clearClipRect();
    }

    M5.Display.display();

    showTopLeft = !showTopLeft; // Toggle

    delay(5000);
}
```

Sorry that i did not have the time to test the development branch

### Comment 8: lovyan03 at 2025-09-24T11:41:12Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3327997132

Hello, @LindezaGrey
Thank you for providing more information.

I tested your test program with v0.2.14 and found no particular problems.

v0.2.11 has a fatal control issue that has already been found to place excessive strain on the EPD. The effects of this load can remain for several tens of minutes even after the power is turned off. In your case, the effects of v0.2.11 likely appeared on the screen while you were testing v0.2.14.
To remedy this, I think you should stop using v0.2.11 and use v0.2.14 to slowly fill the entire screen with alternating black and white, then leave it for a while. This should eliminate the effects.

The following two problems exist with v0.2.11.
1. Excessive control was applied to the EPD, which can cause the gradation to shift in the opposite direction after release, resulting in noticeable staining.
2. All pixels within the update range are refreshed, including pixels whose gradation has not changed.

I think the reason why your test program looks fine in v0.2.11 is due to the effects of 2 above, but since it places unnecessary load on the EPD, I recommend migrating to v0.2.14.

### Comment 9: LindezaGrey at 2025-09-24T14:50:31Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3329046490

I see your point - probably my code was only working because of the strain put on the EPD. I changed my program logic with calls to `lcd.clear` which gives me consistent "clean" screens.
Thanks for pushing the updates and your responses.

### Comment 10: LindezaGrey at 2025-09-24T16:54:03Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3329858638

Ws running it for some time now, and one line died :`)

![Image](https://github.com/user-attachments/assets/284de6fc-1248-4e62-be53-34a751abf3d3)

Dont know if it is related so i am not reopening....

### Comment 11: lovyan03 at 2025-09-24T23:52:55Z

Permalink: https://github.com/m5stack/M5GFX/issues/152#issuecomment-3331096133

I'm sorry to hear about the unfortunate state your Paper S3 is in.

It's very difficult to determine this condition.
It's difficult to accurately determine whether the damage occurred by chance or was caused by software, but I'll keep this as a case in mind.

I also have two Paper S3s that I purchased early on, and one of them already had one broken line when I first started it up, and the broken areas have increased over time as I've used it. The other one has not experienced any broken lines. I've used both devices quite frequently, but I feel like there are some individual differences.
On the damaged unit I have, even the undamaged pixels have unstable gradations and tend to transition to gray with continued use.
