## Component Management and Usage

[\[中文\]](https://docs.espressif.com/projects/esp-techpedia/zh_CN/latest/esp-friends/advanced-development/component-management.html)

Note

This document is automatically translated using AI. Please excuse any detailed errors. The official English version is still in progress.

Imagine you are developing a complex application that involves multiple functional modules, such as network communication, sensor reading, and screen driving. If you write all functions in one large file, the code will become lengthy and difficult to maintain. At this point, components play a role in modularization.

Each component focuses on a specific functional area, so each component can be developed, tested, and maintained separately. By breaking down functions into components, you can reuse these functions in multiple projects without having to rewrite code. This not only saves development time but also improves development efficiency. The structure of the project will also become clearer.

## IDF Components

ESP-IDF components are reusable code packages that are compiled into static libraries during build and can be linked by other components or applications. This allows them to be used in multiple projects.

A complete component generally includes:

- Source code
- Header files
- CMakeLists.txt
	> - Defines source code and header files
	> - Defines dependencies
	> - Registers components
	> - Configures optional features
	> - CMake build description file, used to describe the build configuration and dependencies of the component, instructing the compiler how to compile, link, and build the component.
- idf\_component.yml
	> - Component manager description file, which lists the other components to be referenced and their version information. In this way, when building a project, the ESP-IDF component manager will automatically download and integrate the required components according to these descriptions to ensure that the project’s dependencies are met.

## Component Dependencies

When compiling each component, the ESP-IDF system will recursively evaluate its dependencies. This means that each component needs to declare the components it depends on, i.e., “requires”.

A dependency refers to other software libraries, modules, or components that a software project depends on. These dependencies are necessary for building and running the project. Software projects often use existing code or libraries to implement specific functions or provide certain services, rather than writing all the code from scratch. These external codes or libraries are the dependencies of the project.

The role of dependencies is to promote code reuse, reduce development costs, speed up development, and improve software quality. Using existing dependencies can avoid reinventing the wheel.

Common dependencies include:

- Third-party libraries
- Frameworks and components
- Runtime environment

## Finding Components

1. [IDF](https://github.com/espressif/esp-idf/tree/master/components) includes some basic functional components, which can be found in the components directory
2. [idf-extra-components](https://github.com/espressif/idf-extra-components) includes some IDF supplementary components
3. [esp-iot-solution](https://github.com/espressif/esp-iot-solution/tree/master/components) provides some peripheral driver components
4. Components maintained by Espressif and those uploaded by third parties can be found in the [esp-registry](https://components.espressif.com/).
5. Searching for third-party component libraries

## Component Manager

The [Component Manager](https://github.com/espressif/idf-component-manager) refers to the tool or system used for managing components. In IDF, the [Component Manager provides a set of commands](https://github.com/espressif/idf-component-manager) for adding, removing, downloading, and managing components, so as to reuse existing functional modules in the project.

## Adding Components with Component Manager

1. Find the components needed for the project in the [esp-registry](https://components.espressif.com/).
2. Use the terminal to enter the root folder of the project directory and execute `idf.py add-dependency "[namespace]/<component name>[version number]"`.
	> - This command will automatically add an entry to `main/idf_component.yml`, and if the file does not exist, it will be created automatically.
	> - Namespace: Optional, default from espressif (case insensitive)
	> - Component name: Required, the name of the component, for example usb\_stream (case insensitive)
	> - Version number: Optional, default is \* representing any version. When adding a component for the first time, the latest version will be downloaded automatically.
3. Execute `idf.py build`

Note

- No need to make additional modifications to the project CMakeLists.txt, you can directly include the public header files of the component in the project source code, compile and link the static library of the component.
- Users cannot directly modify the components in managed\_components, because the Component Manager will automatically detect the component Hash value and restore changes.
- In addition to using the command idf.py add-dependency, users can also directly modify the idf\_component.yml file and manually modify the component entries.

## Using Local Components

In the Component Manager description file `idf_component.yml`, add the local component address as shown in the example below.

```c
dependencies:
    idf: ">=4.4.1"
    cmake_utilities: "0.*"
    iot_usbh_modem:
        version: "0.1.*"
        override_path: "../../../../../components/usb/iot_usbh_modem"
```

## Pulling Components from Git Repository

In the Component Manager description file `idf_component.yml`, add the component’s address on Github as shown in the example below.

```c
dependencies:
    esp-gsl:
        git: https://github.com/leeebo/esp-gsl.git
        version: "*"
    button:
        git: https://github.com/espressif/esp-iot-solution.git
        path: components/button
        version: "*"
```

## Updating Components

1. Modify the component version in the component manager description file `idf_component.yml` to the target version you want to update to
2. Delete `managed_components` and `dependencies.lock` in the project file
3. Execute `idf.py build` or manually execute `idf.py reconfigure`

## Deleting Components

1. Delete the component entry in `idf_component.yml`
2. Delete `build`, `managed_components` and `dependencies.lock`
3. Execute `idf.py build`

## Developing Components (Advanced)

1. Create a component directory, consistent with ESP-IDF component requirements, and write the corresponding `CMakeLists.txt` file
2. Add `idf_component.yml` file, add component information, IDF version requirements, specify dependent other components
3. Add `license.txt` file, add license information of the component. The license information is used to explicitly inform other developers or users of the terms and conditions that need to be complied with when using this component. This can ensure that the use and distribution of the component is legal, and clarify the copyright and authorization status of the component author for his work. The license information is generally contained in a file named license.txt or LICENSE, which includes the following content:
	> - License type: Explicitly specify the license type of the component, such as MIT license, Apache license, GPL license, etc. Different types of licenses have different terms and conditions, and developers and users need to comply with the corresponding license regulations.
	> - Copyright information: Indicate the copyright ownership of the component, that is, the author or copyright holder of the component. Copyright information usually includes the author’s name, organization or company name, and copyright year.
	> - Disclaimer: May contain a disclaimer, stating that the author or copyright holder of the component does not assume any liability for any loss or liability that may be caused by the use of the component.
	> - Permissions and restrictions: Explicitly specify under what conditions the component can be used and distributed, as well as prohibited behaviors or restrictions.
	> - Open source license: If the component is open source software, the license information should contain the corresponding open source license terms, such as the rules for sharing, modifying, and distributing open source code.
4. Add `README.md` file, add a brief introduction of the component, simple usage instructions
5. Add `CHANGELOG.md` file, add the version update record of the component
	> - A document used to record the version update record of the component. It usually contains the changes, improvements, fixes, and new features of each version of the component. Developers and users can understand the version update history of the component by referring to the CHANGELOG.md file, so as to better understand the development and use of the component.
6. Add `test_apps` directory, add test cases of the component
	- The component’s self-check tool, which can ensure the quality and stability of the component, is very important for the development and maintenance of the component
		- It contains applications that test different aspects or functions of the component
7. Add `examples` directory
	> - Provide example code for the component to demonstrate the basic usage and functions of the component
	> - Or specify the examples path in idf\_component.yml, add example code of the component

## Releasing Component

### Release on ESP-Registry

ESP-Registry is a central repository provided by Espressif, offering developers a convenient way to discover and download components for their engineering projects.

1. Register an account on ESP-Registry - You can complete the registration on ESP-Registry by authorizing with your Github account
2. After registering, click on the username in the top right corner and select `Tokens`
3. On this page, click `Create` to create a `Token`
4. Register the newly created `Token` to the environment variable `IDF_COMPONENT_API_TOKEN`
5. Use the `idf.py upload-component` command to upload components to the ESP component registry - `idf.py upload-component --namespace [YOUR_NAMESPACE] --name test_cmp` - `YOUR_NAMESPACE`: ESP-Registry username, which can be seen in the profile - `test_cmp`: The name of the component to be uploaded
6. After the upload is complete, the component can be viewed on the [ESP-Registry](https://components.espressif.com/).

### Published on Github Repository

If you only want to publish to the [Github Repository](https://github.com/), you can follow the component development specifications and directly publish the component to your own [Github Repository](https://github.com/). Other developers can pull this component using the previous method.