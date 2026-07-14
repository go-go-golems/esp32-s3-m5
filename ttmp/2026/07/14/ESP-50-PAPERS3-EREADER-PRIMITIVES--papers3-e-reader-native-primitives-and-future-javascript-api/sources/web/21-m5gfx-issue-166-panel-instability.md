# m5stack/M5GFX issue #166: Wierd ghosting effect during fast refresh

- URL: https://github.com/m5stack/M5GFX/issues/166
- State: `closed`
- Created: 2025-11-18T03:10:56Z
- Updated: 2025-11-19T06:59:53Z
- Author: `shinemoon`

## Issue body

![P20251118-105753(1).jpg](https://github.com/user-attachments/assets/0a26e361-8b67-494c-8893-77773747d758)

Though it's expected more ghosting shadow in epd_fast (and fastest). I noticed there will be always one obvious (even more obvious than those painted areas) horizonal line in middle of screen (exactly middle ) after around 6 times pushSprite (and there are no real content in that area in those refreshing ), I tried even epd_text same behavior... so far I am taking one mode switch or full refresh to clean but I feel this line seems not really due to fast rrfresh itself?






## Comments

### Comment 1: lovyan03 at 2025-11-18T03:51:52Z

Permalink: https://github.com/m5stack/M5GFX/issues/166#issuecomment-3544907084

Hello, @shinemoon
If the line appears in the same place every time, it's probably due to damage to the hardware panel. If that's the case, it's something that can't be fixed with a software fix.
This is also happening on my Paper S3, and it probably can't be fixed.

### Comment 2: shinemoon at 2025-11-18T07:27:12Z

Permalink: https://github.com/m5stack/M5GFX/issues/166#issuecomment-3545881169

> Hello, [@shinemoon](https://github.com/shinemoon) If the line appears in the same place every time, it's probably due to damage to the hardware panel. If that's the case, it's something that can't be fixed with a software fix. This is also happening on my Paper S3, and it probably can't be fixed.

Thanks, @lovyan03 , but just to double check , you suppose that's HW related issue even this 'ghosting line' can be cleaned after one full refresh (or just one epd_mode switch)?  sounds a bit strange.



### Comment 3: lovyan03 at 2025-11-18T10:28:55Z

Permalink: https://github.com/m5stack/M5GFX/issues/166#issuecomment-3546770431

Hello, @shinemoon
The epd_fast and epd_fastest modes prioritize speed and do not aim for a complete clear image. Areas with no gradation changes are left unprocessed.
However, lines with damage to the panel will have unstable gradations. As a result, the damaged areas will gradually become visible.
Immediately after changing the mode, a full refresh will be performed, so it will probably be cleared.
To check if this is a GFX issue, try a partial refresh instead of a full-screen refresh and see if the position of the strange lines changes.

### Comment 4: shinemoon at 2025-11-19T06:34:05Z

Permalink: https://github.com/m5stack/M5GFX/issues/166#issuecomment-3551022885

> Hello, [@shinemoon](https://github.com/shinemoon) The epd_fast and epd_fastest modes prioritize speed and do not aim for a complete clear image. Areas with no gradation changes are left unprocessed. However, lines with damage to the panel will have unstable gradations. As a result, the damaged areas will gradually become visible. Immediately after changing the mode, a full refresh will be performed, so it will probably be cleared. To check if this is a GFX issue, try a partial refresh instead of a full-screen refresh and see if the position of the strange lines changes.

Got it, actually I can confirm the location will not be changed during 'partial refresh' without touching the shadow area => So... it can be confirmed as some HW related issue, right?

If so, fine then it's not one GFX issue. I will close this , thanks

### Comment 5: lovyan03 at 2025-11-19T06:50:46Z

Permalink: https://github.com/m5stack/M5GFX/issues/166#issuecomment-3551067167

↓This issue also contains related content, so if you're interested, please take a look.
https://github.com/m5stack/M5GFX/issues/152#issuecomment-3331096133

I'm not sure why the PaperS3's EPD panel is damaged.

Possibilities include...
- There may be a flaw in the M5GFX's control, causing overload on the panel.
- There may be a flaw in the PaperS3's EPD control circuit, making it more susceptible to damage.
- The EPD panel itself may be designed to be more susceptible to damage.

At this point, all of these are possibilities, but I don't know for sure.

### Comment 6: shinemoon at 2025-11-19T06:59:53Z

Permalink: https://github.com/m5stack/M5GFX/issues/166#issuecomment-3551097276

Thanks, then I will close this ticket as such
