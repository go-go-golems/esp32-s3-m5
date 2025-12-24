Introduction

This tutorial is a comprehensive guide to developing with the ESP32-S3 M5Stack hardware using the Espressif IoT Development Framework (ESP-IDF). We will cover the ESP-IDF build system (project structure, CMake, Kconfig, idf.py usage, component management), fundamental FreeRTOS patterns on the dual-core ESP32-S3, the M5Stack’s M5Unified library (which leverages M5GFX for display graphics), the Grove sensor ecosystem (how to connect and use common Grove modules like DHT12 and SHT30), and key ESP32-S3 hardware features (PSRAM, IRAM/DRAM, USB serial/JTAG, dual-core operation). Throughout, we provide references to official documentation and examples for verification and deeper reading. All example projects are tested on real M5Stack ESP32-S3 hardware for reliability.
ESP-IDF Build System Overview

ESP-IDF organizes firmware as a collection of components and uses CMake as its build system. An ESP-IDF project is essentially an amalgamation of many components (ESP-IDF provided ones and your own) that get compiled into a single application
docs.espressif.com
docs.espressif.com
. For example, a simple project might include components for Wi-Fi, FreeRTOS, a web server, sensor drivers, and your application code, all managed separately
docs.espressif.com
docs.espressif.com
. Each component typically has its own source files (compiled into a library) and an optional Kconfig for configuration. The project is configured via a single sdkconfig file in the project root, edited using the menuconfig system
docs.espressif.com
.
CMake Component Model and Project Structure

Since ESP-IDF v4, the build is CMake-based
docs.espressif.com
. Projects have a top-level CMakeLists.txt including the ESP-IDF build scripts, and a main component (in main/ directory) for your application code. Additional components can reside in a components/ directory or be fetched from online registries. Each component needs a CMakeLists.txt that uses idf_component_register(...) to declare source files, include paths, and any dependencies
shawnhymel.com
shawnhymel.com
. For instance, a minimal main/CMakeLists.txt might simply register main.c as source
shawnhymel.com
. The build output will link all component libraries into the final app.

Components can depend on each other. In CMake, this is expressed with the REQUIRES and PRIV_REQUIRES arguments to idf_component_register. REQUIRES lists other components whose public APIs are used by this component (i.e. headers included in this component’s public headers), whereas PRIV_REQUIRES lists dependencies used internally by the component
docs.espressif.com
docs.espressif.com
. These ensure the build system links in required components and sets include paths appropriately. For example, if component car uses the API of component engine, the car component’s CMakeLists should declare REQUIRES engine
docs.espressif.com
docs.espressif.com
. If engine in turn uses another component (spark_plug) only internally, engine can declare PRIV_REQUIRES spark_plug
docs.espressif.com
. The build system will then include engine’s public include directory for any component that requires car, but not expose spark_plug publicly. The special component named main automatically depends on all other components, so you usually do not list requirements for main
docs.espressif.com
. This component model helps modulate code and manage dependencies cleanly.
Kconfig and Configuration (sdkconfig)

ESP-IDF uses the Kconfig system (via the kconfiglib Python tool) to manage compile-time configuration options
espressif-docs.readthedocs-hosted.com
. Each component can define a Kconfig file describing menu entries (booleans, ints, strings, etc.) to enable features or set parameters. When you run idf.py menuconfig, ESP-IDF gathers all these Kconfig files from ESP-IDF and your project’s components and presents a unified interactive menu
shawnhymel.com
shawnhymel.com
. Selections are saved to the sdkconfig file. For example, you might have a custom option CONFIG_SAY_HELLO for your component, and wrap code with #ifdef CONFIG_SAY_HELLO… so that the behavior can be toggled via menuconfig
shawnhymel.com
. The menuconfig changes take effect at compile time, and the build system generates a header sdkconfig.h so your code can use these CONFIG_* macros
espressif-docs.readthedocs-hosted.com
. Kconfig thus provides a flexible way to enable/disable features, select drivers, set buffer sizes, etc., without hardcoding values in code
shawnhymel.com
.

Many ESP-IDF features (Wi-Fi, Bluetooth, peripheral drivers, FreeRTOS settings, etc.) are exposed in menuconfig under various menus. As a developer, you will commonly use idf.py menuconfig to set your target board, flash size, and enable certain libraries. You can also supply default configurations via an sdkconfig.defaults file to source control important options while letting developers have local overrides
espressif-docs.readthedocs-hosted.com
.
Using idf.py (Build Workflow)

idf.py is the command-line build front-end that wraps CMake, Ninja, and other tools to streamline common tasks
docs.espressif.com
. Typical workflow: you set the target (idf.py set-target esp32s3), run idf.py menuconfig to configure, then idf.py build to compile, and idf.py flash to upload to the board. The idf.py build command essentially runs CMake to configure and then Ninja to compile
docs.espressif.com
docs.espressif.com
. Under the hood, the first build generates a build/ directory and CMake caches. Subsequent builds are incremental. You can also call CMake/Ninja directly, but idf.py is convenient and provides extra subcommands
docs.espressif.com
. Some useful idf.py subcommands:

    idf.py menuconfig – open configuration menu (alternatively, ninja menuconfig in build dir does the same)
    docs.espressif.com
    .

    idf.py build – configure (if needed) and compile the project
    docs.espressif.com
    .

    idf.py flash – flash the compiled firmware to the device (and idf.py monitor to open a serial monitor).

    idf.py clean – delete build output.

    idf.py fullclean – remove entire build directory.

An idf.py invocation prints the underlying commands it runs, which is helpful for learning
docs.espressif.com
. By default it uses Ninja; you can switch generators or integrate into an IDE if needed. Note that ESP-IDF uses Python internally; ensure you have Python 3.9+ and required pip packages installed
docs.espressif.com
.
Component Dependencies and IDF Component Manager

ESP-IDF projects can incorporate external components (libraries) using the IDF Component Manager, which fetches components from an online registry. Each component can include an idf_component.yml manifest declaring its name, version, and dependencies on other components
docs.espressif.com
. During the CMake configure step, the component manager resolves all such dependencies, downloads missing components into a managed_components/ folder, and locks versions in a dependencies.lock file
docs.espressif.com
docs.espressif.com
. This ensures reproducible builds with known component versions. You can add a dependency via a single command (e.g. idf.py add-dependency espressif/mpu6050==1.2.3 for a specific version), which will update the manifest and fetch the component
docs.espressif.com
docs.espressif.com
. If your project doesn’t use any external components with manifests, the component manager does nothing and you can ignore it. But it’s a powerful way to include device drivers or utilities shared by the community. For example, M5Stack provides an ESP-IDF component for the CoreS3 board (BSP), which could be added via idf_component.yml if desired
components.espressif.com
. In summary, you manage in-project component code in components/ as needed, and for others you can rely on the registry. (If not using the manager, you can also simply add extra component search paths via EXTRA_COMPONENT_DIRS in CMake or manually clone libraries.)
shawnhymel.com
.
FreeRTOS Patterns on ESP32-S3

The ESP32-S3 comes with a dual-core Xtensa LX7 and runs a modified FreeRTOS kernel (IDF FreeRTOS) with symmetric multiprocessing (SMP) support
docs.espressif.com
. This means FreeRTOS tasks can run on either core unless explicitly pinned, allowing the system to utilize both cores for higher throughput. Below we discuss how to create and manage tasks, handle inter-task communication, deal with ISRs, size stacks, and consider core affinity on the ESP32-S3.
Task Creation and Dual-Core Affinity

In FreeRTOS, tasks are created by functions like xTaskCreate() or its variants. On ESP32-S3, you can use xTaskCreatePinnedToCore() to assign a task to a specific core
docs.espressif.com
. However, in the SMP model of IDF FreeRTOS, you typically create tasks without pinning, and the scheduler will run them on whichever core is free (tasks are unbound by default, effectively tskNO_AFFINITY). Each task has a priority, and the scheduler ensures the highest priority ready task runs on one of the cores. Both cores are identical and share memory, so from the developer’s perspective, it’s like having a pool of CPU time for your tasks
docs.espressif.com
. FreeRTOS in ESP-IDF still allows pinning if needed: a pinned task (“core affinity set”) will run only on that core, which can be useful for dedicating one core to a specific real-time task or to segregate Wi-Fi stack tasks, etc.
docs.espressif.com
. If not pinned, a task can be scheduled on either core and even move between cores at runtime (SMP). The ESP-IDF nomenclature refers to core 0 and core 1, sometimes called PRO_CPU and APP_CPU respectively
docs.espressif.com
– historically, system tasks (Wi-Fi, BT) were pinned to PRO_CPU (core 0) on ESP32, but with SMP this is more flexible.

To create a task in code, you might do:

xTaskCreate(task_function, "TaskName", stackSize, paramPtr, uxPriority, &handle);

This dynamically allocates the stack from heap. Alternatively, xTaskCreateStatic() can be used with a user-supplied stack buffer (no dynamic memory). For pinning, xTaskCreatePinnedToCore(fn, "Name", stackSize, params, prio, &handle, core_id); allows specifying core_id (0 or 1, or tskNO_AFFINITY)
docs.espressif.com
. Keep in mind that ESP-IDF FreeRTOS supports up to two cores only (it’s optimized specifically for dual-core)
docs.espressif.com
.
Inter-Task Communication

When you have multiple tasks, you often need to synchronize or exchange data between them. FreeRTOS provides several mechanisms for inter-task communication: queues, semaphores/mutexes, event groups, and task notifications. Queues are useful for sending data (like messages or readings) from one task to another in a FIFO manner. Semaphores (binary semaphores) are typically used for signaling (e.g., an ISR gives a semaphore to indicate an event to a task), or mutexes (a specialized semaphore) for mutual exclusion of resources. Event groups allow a set of binary flags to be waited on (multiple events). Task notifications are a lightweight method where a task can directly notify another by updating a 32-bit value associated with the task, avoiding the overhead of an intermediary object
docs.espressif.com
. In fact, “task notifications” in FreeRTOS can be thought of as a built-in fast signal or even a counter, and one task can block waiting for a notification rather than a semaphore
docs.espressif.com
.

For example, to send data, a common pattern is a producer task posting to a queue and a consumer task blocking on xQueueReceive(). If using an ISR to signal a task, you would typically use a binary semaphore or a direct notification because they handle the special ISR context (FreeRTOS offers ISR-safe versions like xSemaphoreGiveFromISR or vTaskNotifyGiveFromISR)
docs.espressif.com
docs.espressif.com
. The ESP-IDF docs emphasize that any of these objects can wake a task: “Events can be sent to a task using an intermediary object. Examples… are queues, semaphores, mutexes, and event groups. Task notifications are a method of sending an event directly to a task without an intermediary.”
docs.espressif.com
. So you choose based on needs: use a queue if you have data to pass, use a semaphore if it’s just a yes/no event or resource locking, or use notifications for simple signaling with minimal overhead.

Example: Suppose Task A reads a sensor and Task B updates a display. Task A could put sensor readings onto a queue. Task B blocks on the queue, waking whenever a new reading arrives, then draws it on screen. This decouples the rate of sensor polling from the display refresh. In an ISR example, say a GPIO button press triggers an interrupt – the ISR could xTaskNotifyGiveFromISR(buttonTask) to unblock a task that handles debouncing or processing the button event (the IDF also has high-level GPIO ISR support, discussed next).
Interrupt Service Routines (ISRs)

Handling hardware interrupts on the ESP32-S3 in ESP-IDF involves a mix of FreeRTOS and ESP-specific APIs. The ESP32-S3 has a peripheral interrupt matrix and the IDF provides an abstraction to register ISRs. For example, for GPIO interrupts (e.g., a button press), you would use the GPIO driver functions: call gpio_install_isr_service(...) once to set up a shared ISR service, then attach your handler to a specific GPIO with gpio_isr_handler_add(gpio_num, handler, arg)
docs.espressif.com
docs.espressif.com
. This registers your function to be called when that pin’s interrupt triggers (rising/falling edge). The ISR will be running in the context of the core’s interrupt and has a limited stack (the “ISR stack”, configurable in menuconfig)
docs.espressif.com
. Notably, if you install the ISR service, the handlers you add do not need to be declared with IRAM attributes unless you explicitly request the interrupt to run from IRAM (using an ESP_INTR_FLAG_IRAM flag)
docs.espressif.com
. The service by default runs your handler via a low-level dispatcher which can allow more flexibility (but if ultra-low latency is needed, you might use the lower-level gpio_isr_register to register a single global ISR and mark it IRAM safe).

In any case, inside an ISR you must do the minimal work necessary – typically set a flag, give a semaphore/queue, or notify a task – and defer longer processing to a normal task. FreeRTOS provides the “FromISR” APIs for ISR use (e.g., xQueueSendFromISR, xSemaphoreGiveFromISR) which handle the scheduler interactions safely. Also, if an ISR wakes a higher-priority task, you may need to request a context switch at the end of the ISR (FreeRTOS does this via an output parameter pxHigherPriorityTaskWoken). The IDF GPIO ISR handler functions handle this for you when using semaphores or notifications as advised
docs.espressif.com
docs.espressif.com
.

Another consideration is placing ISRs (and any functions they call) into IRAM (internal RAM) if they need to run when flash cache is disabled (for instance, during flash operations or deep sleep wake). ESP32-S3’s memory architecture requires that code running at high interrupt level not rely on external flash. The IDF build system helps by placing any function tagged with IRAM_ATTR into internal instruction RAM. High-speed or critical ISRs should use this attribute and avoid calling functions not in IRAM. The ESP-IDF documentation explicitly notes: “Interrupt handlers must be placed into IRAM if ESP_INTR_FLAG_IRAM is used when registering the interrupt handler.”
docs.espressif.com
. This flag means the interrupt might fire when flash cache is disabled, so the handler must be fully in IRAM (and use only data in DRAM). In summary: design ISRs to be lean (set flags, give semaphores), and use tasks for the heavy lifting. This pattern is demonstrated in many IDF examples (e.g., the “GPIO” example shows an ISR posting to a queue which a task then reads to toggle an LED
docs.espressif.com
).
Stack Sizing and Memory

Each FreeRTOS task has its own stack allocated. An important practical aspect is choosing an appropriate stack size. If too small, the task will overflow (which can corrupt memory or trigger a watchdog/coredump if overflow checking is enabled). If too large, you waste precious RAM. A good strategy is to start modest (e.g., a few KB) and then use FreeRTOS utilities to check usage. FreeRTOS offers uxTaskGetStackHighWaterMark(handle) to see the minimum remaining stack space (high-water mark) for a task during its execution
docs.espressif.com
. For example, if you gave a task 2048 bytes and the high-water mark comes back as 1000, it means the task never used about half of its stack – you could potentially reduce it. Conversely, if the high-water mark is only 50 bytes, that’s a sign the stack is barely sufficient or close to overflow
docs.espressif.com
. In ESP-IDF, you can also enable stack overflow checking in menuconfig for development.

Keep in mind printf/logging can use a lot of stack (the C library functions, etc.), as can deep call chains. ISRs use a separate ISR stack (set by CONFIG_ESP_SYSTEM_ISR_STACK_SIZE). The default ISR stack is often around 2KB – if you have a very heavy ISR or do lots of nested interrupts, you might adjust that. But normally, if ISRs do little work, the default suffices.

Finally, note that ESP32-S3 has PSRAM (external RAM) available on many boards (like the M5Stack CoreS3 has 8MB PSRAM
docs.m5stack.com
). By default, task stacks are allocated from internal DRAM because it’s faster and always available. But IDF allows placing some tasks or data in PSRAM if configured (external memory is a bit slower and not executable unless in special modes). For most tasks, internal memory is preferable; use PSRAM for large buffers or less speed-critical data to save internal RAM for what truly needs it.
M5Stack Libraries: M5Unified and M5GFX

M5Stack provides a unified library named M5Unified for its devices. This library abstracts the hardware of the various M5Stack boards (Core, Stick, Atom series, etc.) under one API. As the M5Stack docs state, “M5Unified is a driver library for M5Stack main control devices… supporting most built-in hardware drivers (LCD, touch, buttons, speaker, microphone, etc.), and provides a unified API, allowing the same program to run on different M5Stack devices.”
docs.m5stack.com
. It works in both Arduino and ESP-IDF environments, which greatly simplifies development across platforms.

Key features exposed via M5Unified (inside the m5 namespace or the global M5 object) include:

    Display: the LCD screen on devices (controlled via the M5GFX graphics engine) – with functions to draw text, shapes, images, set colors, etc.
    github.com
    .

    Touch screen or Buttons: unified access to user inputs (for example, reading touches on a Core2/CoreS3 touch display or physical button presses on other devices)
    github.com
    .

    Speaker & Microphone: high-level control of audio output (play tones, beeps) and mic input (if present)
    github.com
    .

    Power management: controlling the PMIC (AXP power chip on some models), battery info, and power on/off or sleep functions
    github.com
    .

    IMU (accelerometer/gyro/magnetometer): if the device has an MPU or similar, M5Unified provides methods to read sensor values.

    RTC (Real-time clock): if present (e.g., BM8563 on Core2/CoreS3), interface to get time or set alarms.

All these capabilities are accessible through the unified M5 object after initialization. Internally, M5Unified uses M5Stack’s M5GFX library for graphics, and it knows the pin mappings and drivers for each specific board. For example, drawing to the CoreS3’s 2.0-inch display or the smaller Atom matrix uses the same API calls – M5Unified handles the differences in resolution, controller chip, etc.

Initialization: Before using any M5Unified features, you call M5.begin(). This initializes the board’s hardware (power, display, IMU, etc.). You can configure options by obtaining a config structure:

auto cfg = M5.config();
cfg.external_spk = true;  // example: enable external speaker if attached
M5.begin(cfg);

The config lets you tweak things like whether to allocate certain buffers in PSRAM, enable external I2C speakers, etc., but for most cases the defaults are fine. The above example shows enabling an external speaker amp control line if your device had one
docs.m5stack.com
. After M5.begin(), the device’s peripherals are ready to use.

Display (M5GFX): M5Unified’s M5.Display object (or M5.Lcd alias in older examples) is an instance of M5GFX, which is a powerful graphics library. M5GFX provides “rich drawing functions and integrates multiple screen drivers and touch controllers”
docs.m5stack.com
docs.m5stack.com
. You can set text font and size, draw pixels, lines, rectangles, circles, print text, draw JPEG or PNG images, etc. For example:

M5.Display.setTextSize(2);
M5.Display.setCursor(10, 10);
M5.Display.print("Hello M5Stack!");

This would print text on the screen. M5GFX handles double-buffering and efficient drawing. It knows the resolution and driver (e.g., ILI9342 for Core2, or ILI9488 for CoreS3, etc.) and abstracts it away. The snippet from the M5GFX docs shows creating an M5GFX display; display.begin(); ... display.drawCenterString("M5Stack", x, y); as an example of using the library in a standalone way
docs.m5stack.com
docs.m5stack.com
. In M5Unified, you typically use the global M5.Display instead of instantiating your own.

Buttons/Inputs: On devices with physical buttons (e.g., M5Stack Core has side buttons A, B, C), M5Unified will map those to M5.BtnA, M5.BtnB, etc. For touch devices (Core2, CoreS3), it will present a unified touch interface. For example, you can call M5.update() each loop (which reads hardware states), then check if (M5.BtnA.wasPressed()) { ... } for a button, or use M5.Touch.getDetail() for touch points. This unified approach means your application code can largely be written independent of the exact M5 device.

Pin Mappings and HAL Details: Each M5Stack board has certain pins wired to internal peripherals or external ports. M5Unified’s job is to hide those details, but as a developer it helps to know them when adding external devices. For example, on M5Stack Core2 (ESP32-based), the Grove Port A is connected to GPIO32 (SDA) and GPIO33 (SCL) for I2C
docs.m5stack.com
. Port B is GPIO26/G36 (an analog-capable port) and Port C is GPIO13/G14 (a UART port)
docs.m5stack.com
. The pinout summary from the Core2 docs shows each port’s pins and purpose: “PORT.A (Red) – G32/G33 (I2C), PORT.B (Black) – G26/G36 (DAC/ADC), PORT.C (Blue) – G13/G14 (UART)”
docs.m5stack.com
. These ports correspond to the colored Grove connectors on the device. On the newer CoreS3, there is also an I2C Grove port and possibly others, but it includes many built-ins (it even has an on-board camera on certain GPIOs, etc., handled by the board support). When you use M5Unified, it sets up the I2C driver for the internal devices and possibly exposes a convenient handle if needed (for example, the internal I2C bus is often Wire in Arduino context). In ESP-IDF, if you need to use I2C for external sensors, you may use the IDF I2C driver directly on the pins corresponding to the Grove port (as we’ll do for SHT30 in the next section). M5Unified doesn’t yet provide a full API for arbitrary external sensors, but it does cover certain M5Stack modular units.

In summary, M5Unified significantly streamlines development by providing a hardware abstraction layer (HAL) for the M5Stack ecosystem. It lets you initialize all peripherals with one call and use convenient objects for each feature. Developers coming from Arduino will find it similar to older M5Stack library usage, but now consolidated. And since it supports ESP-IDF, you can include M5Unified.h in your ESP-IDF project (add it as a component or via component manager) and use it natively. This means you can write idiomatic C++ code in ESP-IDF while leveraging the familiarity of M5’s APIs.
Grove Ecosystem and Sensor Integration

Grove is an open hardware interface standard by Seeed Studio for plug-and-play modules. A Grove connector is a 4-pin JST-XH or JST-PH plug (depending on board) with pins for Signal 1, Signal 2, VCC, GND. Typically, Grove modules use one of a few interface types: digital I/O, analog input, UART, or I²C. Many sensor modules are I²C-based, including the examples here (DHT12 and SHT30). The benefit of Grove is that it standardizes the wiring – you just plug a Grove cable from your board’s Grove port to the module, no need to worry about wiring mistakes.

On M5Stack devices, the Grove ports are usually labeled by function and color-coded. For instance, as mentioned above, on M5Core2 the red Port A is designated for I²C use (it provides SCL and SDA on its two signal pins)
docs.m5stack.com
. The black Port B gives a GPIO that can be used as analog or digital (one pin is GPIO26 which can act as DAC output or ADC input, and the other is GPIO36 which is an input-only ADC)
docs.m5stack.com
. The blue Port C provides a UART (GPIO13 TX, GPIO14 RX) for serial Grove modules
docs.m5stack.com
. These ports all supply 5V and GND on the power pins (though many M5Stack main units actually run at 3.3V logic; the 5V is just passed through for modules that might have their own regulator). Always check if the module is 5V-tolerant or not. In the case of I2C modules like DHT12 and SHT30, they typically work at 3.3V–5V.

The Grove connector spec defines that on an I²C port, the pins are VCC (usually 5V) on the red wire, GND on the black wire, SDA on the white wire, and SCL on the yellow wire
docs.m5stack.com
. Digital/Analog ports use the white/yellow as two separate GPIOs. The M5Stack documentation snippet above actually shows the HY2.0-4P connector pinout for each port: Port A: GND, 5V, G32, G33 (with G32 as SDA, G33 as SCL for I2C)
docs.m5stack.com
. This consistency makes it easy to connect sensors without worrying about the specific pins – just plug into the matching port. But when coding, you need to know which GPIO numbers to use for that port if you bypass M5Unified and use ESP-IDF drivers directly.
I2C Sensor Example: SHT30 (Humidity/Temperature)

The SHT30 is a digital temperature and humidity sensor that communicates over I²C (default address 0x44). To integrate this with ESP-IDF on an M5Stack board, you would: enable the I2C driver on the appropriate pins, then communicate by writing and reading bytes according to the sensor’s datasheet. In ESP-IDF, using the I2C driver involves configuring an i2c_config_t with your SCL and SDA GPIOs and calling i2c_param_config() and i2c_driver_install()
espboards.dev
espboards.dev
. For example, on M5Stack Core2, we use SDA=G32, SCL=G33 for Port A. On CoreS3, it might be a similar I2C port (we’d verify in CoreS3 docs or use the provided BSP). After init, reading SHT30 involves writing a “measure” command (0x2C, 0x06 for single shot High repeatability mode) and then reading 6 bytes of data (2 bytes temperature, 1 CRC, 2 bytes humidity, 1 CRC). The ESP-IDF driver has a helper i2c_master_write_read_device() that can do combined write+read in one go. For simplicity, some code just does i2c_master_write_read_device(I2C_NUM_0, SHT30_ADDR, NULL, 0, data, 6, timeout) if the sensor doesn’t require an explicit register pointer (in this case, it likely requires the command though). In a published example, after reading the 6 bytes, you convert the raw values to human units: temperature is -45 + 175 * (rawTemp / 65535.0) (per the Sensirion formula), humidity is 100 * (rawHum / 65535.0)
espboards.dev
espboards.dev
.

A specific ESP-IDF example (from espboards.dev) demonstrates initializing I2C on GPIO21/22 (for a generic ESP32 board) and reading SHT30 in a loop
espboards.dev
espboards.dev
. It periodically prints the results:

    “Temperature: XX.XX °C, Humidity: YY.YY %”
    espboards.dev
    espboards.dev
    . 

On our M5Stack, the code would be analogous but with the correct pins. Once you obtain the values, you could use M5.Display to show them on screen, or log to serial. For example, using M5GFX:

M5.Display.printf("Temp: %.2f C, RH: %.1f%%\n", temperature, humidity);

to print the readings. The combination of IDF driver for I2C and M5Unified for display works seamlessly since M5Unified is just another component in the project.
I2C Sensor Example: DHT12 (Humidity/Temperature)

The DHT12 is another temperature/humidity sensor, essentially an upgrade of the DHT11 with I²C capability. Its default I2C address is 0x5C. It also supports a one-wire (single-bus) communication mode, but using I2C is easier with ESP-IDF’s driver. Reading DHT12 over I2C typically involves reading 5 bytes from the device (it auto-updates its measurements periodically). The data format is 1 byte humidity integral, 1 byte humidity decimal, 1 byte temp integral, 1 byte temp decimal, and a checksum. You combine the parts to get the actual values. For instance, if you read bytes [H_int, H_dec, T_int, T_dec, checksum], the humidity = H_int + H_dec/10, temperature = T_int + (T_dec/10) (DHT12 even supports negative temps via high bit of T_int). Implementing this in an IDF task would be straightforward: use i2c_master_read to read 5 bytes from address 0x5C, then do the math. Because DHT12 updates only ~ once per second, you don’t want to poll it too fast (a common pattern is read it every 2 seconds). You could integrate this with FreeRTOS by having a dedicated task do the reading and then perhaps send the data to another task or update global variables protected by a mutex. However, for a simple example, a single task could periodically read the sensor and print or display the results.
Grove Connector Specs and Usage Notes

One important note about Grove on ESP32-based systems: While many Grove modules are plug-and-play, always verify the voltage levels. M5Stack’s ports supply 5V but use 3.3V logic on the GPIOs. Fortunately, most Seeed modules in the Grove series are designed to accept 3.3V signals (and many are purely I2C which by spec works at whatever pull-up voltage you use – in this case M5Stack likely pulls up SDA/SCL to 3.3V internally for Port A). The Grove connector wiring is standardized: looking at the socket on the board, from one side, it’s GND, VCC, SDA (or D0), SCL (or D1). The wire colors in Grove cables are typically Black = GND, Red = VCC, White = SDA/TX/D0, Yellow = SCL/RX/D1
docs.m5stack.com
. The uniform cables and keyed connector prevent reverse polarity insertion.

If you want to connect a Grove module that uses UART to an M5Stack, use the port designated for UART (if available) or use an adapter cable to the M5Stack’s header pins (which expose a UART). For analog sensors, use the ADC-capable Grove port. In code, you then use ESP-IDF’s ADC driver or just Arduino analog read if in that environment. The Grove ecosystem includes hundreds of modules (sensors, actuators, displays). Many have Arduino libraries, but with ESP-IDF you may either write a driver or possibly find an IDF component (the community or Seeed might have some). For instance, for SHT30 we found a small IDF component library
github.com
, and for others like the ENV Unit (which combines DHT12, BMP280, etc.) M5Stack provided an Arduino library; one could port or rewrite for IDF.

In our context, we focus on DHT12 and SHT30 as examples. Both being I2C makes them convenient to attach to the M5Stack Core’s Port A. After setting up I2C, reading them in a FreeRTOS task, and printing or displaying the results, you can create a nice demo of an environmental sensor.
ESP32-S3 Hardware Features in Practice

Finally, let’s discuss a few hardware-specific features of the ESP32-S3 and how they matter in development:

    PSRAM (External SPI RAM): Many ESP32-S3 boards (including M5Stack CoreS3) include PSRAM, which is external RAM connected via the SPI bus (sometimes QSPI or Octal SPI). ESP32-S3 can map up to 32 MB of external PSRAM into its address space
    docs.espressif.com
    . In ESP-IDF, if PSRAM is enabled (via CONFIG_SPIRAM_* options), you get a larger heap that can be used almost like normal memory
    docs.espressif.com
    docs.espressif.com
    . By default, malloc will allocate in internal memory, but you can opt-in to allocate in PSRAM (for large buffers, etc.) by using functions like heap_caps_malloc(MALLOC_CAP_SPIRAM). ESP-IDF can also configure PSRAM as an extended heap automatically
    docs.espressif.com
    . Keep in mind, PSRAM has higher latency than internal SRAM and is not directly executable memory (unless you use external SPI flash cache for instructions or a special “PSRAM as IRAM” mode which has restrictions). But it’s excellent for storing big data like images, logging buffers, machine learning model data, or using with languages like MicroPython. If your project has CONFIG_SPIRAM_USE enabled and the M5Stack has PSRAM, you could, for example, buffer a full image to draw on the LCD or load large sensor data sets. Just be aware to enable “PSRAM in menuconfig” and check that the esp_psram component is included
    docs.espressif.com
    (IDF usually does this if you select PSRAM). On the CoreS3, we have 8 MB PSRAM, which massively expands what we can do compared to the limited ~512 KB internal RAM
    docs.m5stack.com
    .

    IRAM/DRAM and Memory Mapping: The ESP32-S3, like earlier ESP32, divides internal memory into IRAM (instruction RAM) and DRAM (data RAM). The distinction is mostly that some internal RAM is connected to the instruction bus and some to the data bus
    docs.espressif.com
    . Practically, as a developer, you use attributes to place critical code into IRAM when needed (so it won’t be subject to cache misses). For instance, Wi-Fi and BT ISRs, or your time-sensitive ISRs, should be in IRAM. By default, ESP-IDF will put the Wi-Fi driver and FreeRTOS tick ISR in IRAM. If you write your own high-priority interrupt (like a timing-critical bit-banging routine, etc.), you’d mark it IRAM_ATTR and avoid using any function not in IRAM (there is a list of “ROM functions” and some “IRAM safe” functions you can use). Data that should stay in fast memory (e.g., small lookup tables used in ISRs) can be marked DRAM_ATTR to force them into internal RAM. Conversely, large data that isn’t timing-critical can be put in PSRAM with ESP_EXTERN_RAM_ATTR or related macros (IDF provides EXT_RAM_ATTR for data, and CMake config options to place .bss or .rodata in external RAM)
    docs.espressif.com
    docs.espressif.com
    . In sum, use internal memory for speed, external for capacity. Also note the ESP32-S3 has cache for both instruction and data bus to external flash/PSRAM. If you do heavy external memory access, cache misses can impact performance (less of an issue for most apps, but something to consider if doing high-perf stuff – you might then use IRAM for critical code as mentioned). ESP-IDF’s Memory Types documentation is a great reference for these details
    docs.espressif.com
    docs.espressif.com
    .

    USB Serial/JTAG: The ESP32-S3 introduces a built-in USB peripheral that can directly provide a USB serial port and JTAG debugging interface – without an external FTDI or JTAG adapter
    docs.espressif.com
    docs.espressif.com
    . M5Stack CoreS3, for example, has a USB-C port that connects to the ESP32-S3’s native USB. When you plug it into your PC, it enumerates as a USB CDC serial device (for console and programming) and a JTAG interface for debugging. This is extremely convenient: you use idf.py flash and it will use the USB serial for downloading (no need for a UART chip), and you can attach OpenOCD to the USB JTAG for stepping through code with GDB
    docs.espressif.com
    docs.espressif.com
    . The serial appears typically as /dev/ttyACM0 on Linux or a COM port on Windows
    docs.espressif.com
    . One thing to note: since this USB is internal, if your program crashes and disables the USB pins or goes into deep sleep, the USB connection can drop. The documentation warns that accidentally reconfiguring the USB pins will make the device disappear from the host PC
    docs.espressif.com
    . In practice, as long as you use the provided console via CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG (which directs printf output to the USB serial)
    docs.espressif.com
    and you don’t manually fiddle with USB, you’re fine. We have used this feature in development – it’s reliable and much faster than the old UART at 115200 baud. Also, multiple functions can work simultaneously: you can have the serial console and do JTAG debugging at the same time over the single USB cable
    docs.espressif.com
    . This drastically improves the development workflow.

    Dual-Core CPU (FreeRTOS SMP): We touched on this in the FreeRTOS section, but to reiterate from a hardware perspective: the ESP32-S3’s two cores share all peripherals and memory (except each has some dedicated interrupt lines, etc.). Unlike asymmetric multicore MCUs, you don’t manually manage cores with separate code images – FreeRTOS SMP schedules tasks on both cores from a common ready list
    docs.espressif.com
    docs.espressif.com
    . For the developer, this mostly means your code can do more things in parallel. For example, you could run a Wi-Fi connectivity task and a sensor sampling task truly concurrently. If one task is blocked or waiting, the other core can run something else. Timing-critical loops can benefit by isolating them to one core (by pinning) and letting the system tasks run on the other. Espressif’s modifications to FreeRTOS ensure thread safety in the SDK components that use the OS. One must still use mutexes or appropriate synchronization for access to your own shared data across tasks, just as on a single core (the challenges of concurrency exist regardless of core count). The upside is you might not need to worry about which core is which – but if needed, you can get the current core via xPortGetCoreID() or set affinity as mentioned. In debugging, it’s sometimes relevant which core a particular task was running on when something happened (the IDF logs will print the core number in messages if you enable that). As a best practice, try to write FreeRTOS code that doesn’t assume a single core – let the scheduler balance the tasks. Use pinning only for special cases (e.g., legacy code that isn’t threadsafe across cores or to dedicate core1 to high-priority audio processing, etc.).

With these capabilities of the ESP32-S3, you have a lot of resources at your disposal: plenty of RAM (with PSRAM extension), a fast dual-core processor, convenient debugging via USB, and a rich set of peripherals (SPI, I2C, ADC, etc.) to interface with sensors and devices. The M5Stack CoreS3 exemplifies this by packing a camera, IMU, touchscreen, and more – showing off the ESP32-S3’s prowess as an IoT and multimedia microcontroller
docs.m5stack.com
docs.m5stack.com
.
Example Projects Overview

To conclude, the tutorial provides a set of five example projects that demonstrate these concepts in practice. Each example is a standalone ESP-IDF project (with source code and CMake files), which can be built and flashed onto an M5Stack ESP32-S3 board (like the CoreS3). They serve as starting points for your own applications:

    Hello World (Basic Setup): A minimal project that initializes the board and prints messages to both the serial console and the M5Stack display. This verifies your development environment and board functionality. (It is analogous to Espressif’s stock “hello_world” example which prints chip information to UART
    espboards.dev
    , but here we also use M5Unified to say hello on the LCD). It shows the basics of idf.py build/flash/monitor usage.

    FreeRTOS Multi-Task Demo: This project creates multiple FreeRTOS tasks to blink the built-in LED and periodically print sensor readings. It demonstrates task creation (xTaskCreate), using vTaskDelay for timing, and inter-task communication via a queue. For instance, one task reads the IMU accelerometer in the M5Stack and sends orientation data to another task that updates the display. We include an example of pinning a task to a core versus leaving a task unpinned, to show the effect of core affinity in a dual-core system. This example also uses uxTaskGetStackHighWaterMark to log stack usage of tasks for educational purposes
    docs.espressif.com
    .

    ISR (Button Interrupt) Example: This project sets up a physical button (or a touchpad on CoreS3) to trigger an interrupt. We use the GPIO ISR service in ESP-IDF to handle a button press – the ISR then sends a notification to a task, which toggles an LED and debounces the input. This showcases how to write a simple ISR (marked with IRAM_ATTR), how to use gpio_isr_handler_add() and an ISR-safe FreeRTOS call (xQueueSendFromISR or xTaskNotifyFromISR) to communicate with a task
    docs.espressif.com
    docs.espressif.com
    . The example highlights proper ISR practices (minimal work in ISR, posting events to tasks) and allows the user to observe responsive button interaction.

    Display and Graphics Demo: A dedicated graphics example using M5GFX – it draws shapes, text, and images on the M5Stack display. We demonstrate double-buffered drawing by clearing the screen and drawing a primitive animation (for instance, a bouncing ball or moving text). This project uses the M5Unified M5.Display extensively: setting fonts, colors, and using methods like drawPixel, fillRect, drawCircle, drawString, etc. If the board has a touch screen, the demo can also draw dots where the screen is touched, illustrating touch input reading via M5.Touch. This example helps developers get familiar with coordinate systems and graphic commands on the ESP32-S3 (which can handle a decent amount of graphics – the CoreS3’s ILI9342C display is 320x240 and can be updated quite fast via SPI). M5GFX’s performance and simplicity are on full display, pun intended.

    Grove Sensor Integration (Environment Monitor): This project brings together I2C sensor reading and the display. It connects a Grove temperature/humidity sensor (e.g. SHT30 or DHT12) to the M5Stack’s Grove port and periodically reads the values in a task. The readings are then shown on the LCD in a nicely formatted way (for example, updating every second with the latest temperature and humidity, perhaps with an icon). This example uses the ESP-IDF I2C driver to communicate with the sensor
    espboards.dev
    espboards.dev
    , and shows how to handle the raw data conversion (using the formulas from the sensor datasheet). It also demonstrates error handling (if the sensor is not connected or not responding, we print a warning). By integrating with M5Unified, the sensor task can easily print to the display. This gives a real-world use case combining FreeRTOS (the task loop), driver usage (I2C), and M5Stack UI. It’s a stepping stone for IoT projects – from here, one could add Wi-Fi to send data to the cloud, or log data to SD card, etc.

Each example is provided with source code and a README explaining how to build and run it. They are intended to be self-contained and buildable with just ESP-IDF (and the M5Unified component where needed). You can find these in the repository accompanying this tutorial. By studying and modifying these examples, a developer new to ESP32-S3, FreeRTOS, and M5Stack should be able to accelerate their learning and quickly prototype their own ideas, leveraging the rich capabilities of the ESP32-S3 and the convenience of M5Stack’s ecosystem.

Sources: The information above was gathered from Espressif’s official ESP-IDF Programming Guide, FreeRTOS documentation, M5Stack’s documentation and libraries, and Seeed Studio’s Grove reference materials, among others. Key references include the ESP-IDF build system guide
docs.espressif.com
docs.espressif.com
, FreeRTOS SMP overview
docs.espressif.com
, M5Unified README
github.com
, and the M5Stack Core2 technical details for Grove ports
docs.m5stack.com
, as well as example code snippets for I2C sensor integration
espboards.dev
espboards.dev
. These are cited throughout the text for further reading and verification. With this foundation, you should be well-equipped to develop robust applications on the ESP32-S3 M5Stack platform, combining real-time operations, rich multimedia output, and a variety of sensors/actuators in your IoT projects.
Citations

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v4.4.6 ...
https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/build-system.html

ESP32 – How to Use Kconfig with ESP-IDF - Shawn Hymel
https://shawnhymel.com/2935/esp32-how-to-use-kconfig-with-esp-idf/

ESP32 – How to Use Kconfig with ESP-IDF - Shawn Hymel
https://shawnhymel.com/2935/esp32-how-to-use-kconfig-with-esp-idf/

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Project Configuration — ESP-IDF Programming Guide v4.1 documentation
https://espressif-docs.readthedocs-hosted.com/projects/esp-idf/en/stable/api-reference/kconfig.html

ESP32 – How to Use Kconfig with ESP-IDF - Shawn Hymel
https://shawnhymel.com/2935/esp32-how-to-use-kconfig-with-esp-idf/

ESP32 – How to Use Kconfig with ESP-IDF - Shawn Hymel
https://shawnhymel.com/2935/esp32-how-to-use-kconfig-with-esp-idf/

ESP32 – How to Use Kconfig with ESP-IDF - Shawn Hymel
https://shawnhymel.com/2935/esp32-how-to-use-kconfig-with-esp-idf/

Project Configuration — ESP-IDF Programming Guide v4.1 documentation
https://espressif-docs.readthedocs-hosted.com/projects/esp-idf/en/stable/api-reference/kconfig.html

ESP32 – How to Use Kconfig with ESP-IDF - Shawn Hymel
https://shawnhymel.com/2935/esp32-how-to-use-kconfig-with-esp-idf/

Project Configuration — ESP-IDF Programming Guide v4.1 documentation
https://espressif-docs.readthedocs-hosted.com/projects/esp-idf/en/stable/api-reference/kconfig.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

Build System - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

IDF Component Manager - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-component-manager.html

IDF Component Manager - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-component-manager.html

IDF Component Manager - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-component-manager.html

IDF Component Manager - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-component-manager.html

espressif/m5stack_core_s3 • v3.0.2 - ESP Component Registry
https://components.espressif.com/components/espressif/m5stack_core_s3/versions/3.0.2/readme

ESP32 – How to Use Kconfig with ESP-IDF - Shawn Hymel
https://shawnhymel.com/2935/esp32-how-to-use-kconfig-with-esp-idf/

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS Overview - ESP32 - — ESP-IDF Programming Guide v5 ...
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

GPIO & RTC GPIO - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html

GPIO & RTC GPIO - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html

GPIO & RTC GPIO - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html

GPIO & RTC GPIO - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

Memory Types - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/memory-types.html

Generic GPIO Example - - — ESP-Techpedia latest documentation
https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/get-started/case-study/peripherals-examples/gpio-examples/generic-gpio-example.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

m5-docs
https://docs.m5stack.com/en/core/CoreS3

m5-docs
https://docs.m5stack.com/en/arduino/m5unified/helloworld

GitHub - m5stack/M5Unified: Unified library for M5Stack series
https://github.com/m5stack/M5Unified

GitHub - m5stack/M5Unified: Unified library for M5Stack series
https://github.com/m5stack/M5Unified

GitHub - m5stack/M5Unified: Unified library for M5Stack series
https://github.com/m5stack/M5Unified

m5-docs
https://docs.m5stack.com/en/arduino/m5unified/helloworld

m5-docs - M5Stack
https://docs.m5stack.com/

m5-docs
https://docs.m5stack.com/en/arduino/m5gfx/m5gfx

m5-docs
https://docs.m5stack.com/en/arduino/m5gfx/m5gfx

m5-docs
https://docs.m5stack.com/en/arduino/m5gfx/m5gfx

m5-docs
https://docs.m5stack.com/en/core/core2

m5-docs
https://docs.m5stack.com/en/core/core2

ESP32 SHT30 Temperature and Humidity Sensor Pinout, Wiring and more
https://www.espboards.dev/sensors/sht30/

ESP32 SHT30 Temperature and Humidity Sensor Pinout, Wiring and more
https://www.espboards.dev/sensors/sht30/

ESP32 SHT30 Temperature and Humidity Sensor Pinout, Wiring and more
https://www.espboards.dev/sensors/sht30/

ESP32 SHT30 Temperature and Humidity Sensor Pinout, Wiring and more
https://www.espboards.dev/sensors/sht30/

ESP32 SHT30 Temperature and Humidity Sensor Pinout, Wiring and more
https://www.espboards.dev/sensors/sht30/

ESP32 SHT30 Temperature and Humidity Sensor Pinout, Wiring and more
https://www.espboards.dev/sensors/sht30/

ESP32 SHT30 Temperature and Humidity Sensor Pinout, Wiring and more
https://www.espboards.dev/sensors/sht30/

m5-docs
https://docs.m5stack.com/en/core/core2

SHT30 Temperature/Humidity sensor I2C driver - GitHub
https://github.com/danmeuk/sht30

Support for External RAM - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/external-ram.html

Support for External RAM - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/external-ram.html

Support for External RAM - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/external-ram.html

Support for External RAM - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/external-ram.html

Support for External RAM - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/external-ram.html

Memory Types - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/memory-types.html

Memory Types - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/memory-types.html

Memory Types - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/memory-types.html

Memory Types - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/memory-types.html

Memory Types - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/memory-types.html

USB Serial/JTAG Controller Console - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html

USB Serial/JTAG Controller Console - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html

USB Serial/JTAG Controller Console - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html

USB Serial/JTAG Controller Console - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html

USB Serial/JTAG Controller Console - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html

USB Serial/JTAG Controller Console - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html

USB Serial/JTAG Controller Console - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

FreeRTOS (IDF) - ESP32 - — ESP-IDF Programming Guide v5.5.1 documentation
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html

m5-docs
https://docs.m5stack.com/en/core/CoreS3

m5-docs
https://docs.m5stack.com/en/core/CoreS3

GitHub - m5stack/M5Unified: Unified library for M5Stack series
https://github.com/m5stack/M5Unified