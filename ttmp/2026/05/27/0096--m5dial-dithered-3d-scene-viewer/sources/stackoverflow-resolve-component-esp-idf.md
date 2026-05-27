I'm building ESP-IDF project with multiple components (whether custom created or from third-party components in GitHub)

when I tried to combine all together into single project I was not able to compile my projects. I have read the documentation of ESP but even though I could not resolve my issues.

Here is my project structure:

```
- components
  - cpp_utils
     - CMakeLists.txt
  - LoRa_E22
    - CMakeLists.txt
  - CMakeLists.txt (Yes in components level dir)

- main
  - CMakeLists.txt
```

Content:

- cpp utils is the BT C++ library [GitHub link](https://github.com/nkolban/esp32-snippets/tree/fe3d318acddf87c6918944f24e8b899d63c816dd/cpp_utils), it requires both CMakeLists file, in components root and in cpp utils. otherwise, it does not compile at all.
	- cpp\_utils/CMakeLists.txt content
		```
		# Edit following two lines to set component requirements (see docs)
		set(COMPONENT_REQUIRES
		     "console"
		     "fatfs"
		     "json"
		     "mdns"
		     "nvs_flash"
		   )
		set(COMPONENT_PRIV_REQUIRES )
		file(GLOB COMPONENT_SRCS
		     LIST_DIRECTORIES false
		     "*.h"
		     "*.cpp"
		     "*.c"
		     "*.S"
		    )
		set(COMPONENT_ADD_INCLUDEDIRS ".")
		register_component()
		```
- LoRa E22 is a Lora Ebyte library for receiver and transmitting over Radio based on [Lora GitHub](https://github.com/xreef/EByte_LoRa_E22_Series_Library/tree/master)
	- LoRa\_E22/CMakeLists.txt content:
		```
		set(component_srcs "LoRa_E22.cpp")
		idf_component_register(SRCS "${component_srcs}"
		                       PRIV_REQUIRES driver
		                       INCLUDE_DIRS "."
		                       REQUIRES arduino)
		```
- Components CMakeList.txt - is empty file
- main CMakeLists.txt content:
```
set(component_srcs "main.cpp" 
                    "src/ble_jobs/ble_app.cpp" 
                    "src/data/models.cpp" 
                    "src/utils.cpp")

idf_component_register(SRCS  "${component_srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "LoRa_E22"
)
```

all of src/\* are my own code, had no issues with it. Currently the issue is: `Failed to resolve component 'LoRa_E22'.`

if I take out the components CMakeList.txt, then cpp\_utils fails with undefined issues in esp\_bt file and similar.

How can this be resolved? what am I missing?