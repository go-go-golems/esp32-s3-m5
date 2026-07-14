# m5stack/M5GFX issue #157: Abnormal Behavior of canvas->pushSprite in latest M5GFX@0.2.15

- URL: https://github.com/m5stack/M5GFX/issues/157
- State: `closed`
- Created: 2025-10-11T13:28:59Z
- Updated: 2025-10-31T17:11:46Z
- Author: `shinemoon`

## Issue body

On **PaperS3** , Everything works fine till I sorted my git trees (and then I try to start from clean work space).. but then display suddenly go to total messed and random display issues (e.g. black/white randomly inverted/ blank screen / can't clean /etc. ) though no any compile issue or error raised.

And there is just typical `Canvas-> pushSprite` worked fine cross old versions.

After cross check and ABA test ,  all issues disappeared after switching back to M5GFX@0.2.14 (seems my `libdeps `stayed in M5Unified@0.28 while I had not proactively triggered the update)

I tried combination below:

- M5Unified@0.2.8 + M5GFX@0.2.14 => **OK**
- M5Unified@0.2.9 + M5GFX@0.2.14 => **OK**
-----
- M5Unified@0.2.8 + M5GFX@0.2.15 => **NOK**
- M5Unified@0.2.9 + M5GFX@0.2.15 => **NOK**
- M5Unified@0.2.10 + M5GFX@0.2.15 => **NOK**

So, though no idea about what happen.. there is kind of critical bug will ruined display in `canvas-> pushSprite` usage for PAPERS3.



## Comments

### Comment 1: lovyan03 at 2025-10-14T02:53:59Z

Permalink: https://github.com/m5stack/M5GFX/issues/157#issuecomment-3399921760

Hello, @shinemoon
I apologize for the inconvenience.
Please try the develop branch. If there are no problems, we will release an update soon.

### Comment 2: shinemoon at 2025-10-16T10:47:00Z

Permalink: https://github.com/m5stack/M5GFX/issues/157#issuecomment-3410272376

Thanks

but as newbie... not sure how to set in platformio.ini to pick the dev branch?

I will have a try then

> Hello, @shinemoon
> I apologize for the inconvenience.
> Please try the develop branch. If there are no problems, we will release an update soon.



### Comment 3: lovyan03 at 2025-10-17T01:25:58Z

Permalink: https://github.com/m5stack/M5GFX/issues/157#issuecomment-3413444006

Add the following to the `env` section in platformio.ini :
```
lib_deps = https://github.com/M5Stack/M5GFX#develop
                  M5Stack/M5Unified
```
need once `Full Clean` before build.

<img width="588" height="239" alt="Image" src="https://github.com/user-attachments/assets/1531eabf-44da-4873-9fcd-d82ca274f5b3" />

### Comment 4: shinemoon at 2025-10-17T03:26:49Z

Permalink: https://github.com/m5stack/M5GFX/issues/157#issuecomment-3413678330

Thanks , so far it looks **OK** under below combination :

Dependency Graph
|-- M5GFX @ 0.2.15+sha.fd824ee
|-- M5Unified @ 0.2.10
|-- SPI @ 2.0.0
|-- SPIFFS @ 2.0.0
|-- SD @ 2.0.0
|-- FS @ 2.0.0
|-- SD_MMC @ 2.0.0
|-- WebServer @ 2.0.0
|-- WiFi @ 2.0.0

### Comment 5: lovyan03 at 2025-10-29T12:10:10Z

Permalink: https://github.com/m5stack/M5GFX/issues/157#issuecomment-3461188159

Thank you for the information.
Version 0.2.16 has been released, and I believe this issue has been resolved.

### Comment 6: shinemoon at 2025-10-31T17:11:46Z

Permalink: https://github.com/m5stack/M5GFX/issues/157#issuecomment-3474059748

#

> Thank you for the information. Version 0.2.16 has been released, and I believe this issue has been resolved.

True, now 0.2.16 under use.. so far so good. Thanks!
