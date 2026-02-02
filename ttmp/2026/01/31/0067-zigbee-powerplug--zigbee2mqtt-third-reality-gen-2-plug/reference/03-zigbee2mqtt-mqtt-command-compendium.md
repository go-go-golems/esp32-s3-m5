---
Title: Zigbee2MQTT MQTT command compendium
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Comprehensive, structured reference for Zigbee2MQTT MQTT topics, request commands, and advanced operations with official source pointers."
LastUpdated: 2026-01-31T11:48:57-05:00
WhatFor: "Serve as a detailed, copy/paste-ready command map for Zigbee2MQTT MQTT control and maintenance."
WhenToUse: "Use when you need to issue or understand Zigbee2MQTT MQTT commands or locate the authoritative reference for a specific command set."
---

# Zigbee2MQTT MQTT Command Compendium

## Goal

This document provides a detailed, textbook-style guide to the Zigbee2MQTT MQTT command interface. It is organized by concept and intent, with clear pointers to the official reference documentation for each command family. By the end of this guide, you should understand how Zigbee2MQTT structures its MQTT API and be able to issue any supported command with confidence.

## Context

Zigbee2MQTT is a bridge that connects Zigbee devices to an MQTT broker. Rather than requiring specialized APIs or proprietary protocols, it exposes a structured MQTT interface that follows consistent conventions. Most operations are performed by publishing JSON payloads to request topics under a configurable base topic (the default is `zigbee2mqtt`). The bridge publishes responses on corresponding response topics, enabling programmatic interaction with your Zigbee network from any MQTT client.

This design has several advantages. First, MQTT is a widely supported protocol with clients available in virtually every programming language and platform. Second, the request/response pattern with JSON payloads is both human-readable and machine-parseable. Third, the topic hierarchy provides natural organization—you can subscribe to specific slices of the network (a single device, all bridge events, or everything) depending on your needs.

Think of this guide as a map. You can navigate by *topic family* (bridge versus device), by *command type* (commissioning, maintenance, binding, OTA), or by *data flow* (request, response, state). The sections that follow build from foundational concepts toward advanced operations.

---

## Part I: The Language of the API

Before issuing commands, it helps to understand the vocabulary Zigbee2MQTT uses. This section introduces the core concepts that recur throughout the API.

### 1. Base Topic and Namespace

Every Zigbee2MQTT installation operates under a **base topic**. By default, this is `zigbee2mqtt`, but you can configure it to any value in your `configuration.yaml`. All MQTT communication occurs within this namespace.

Within the base topic, there are two primary branches:

- **Bridge topics** live at `zigbee2mqtt/bridge/...` and handle system-level operations: network status, configuration, device inventory, and administrative commands.
- **Device topics** live at `zigbee2mqtt/<friendly_name>/...` and handle device-specific operations: reading state, sending commands, and monitoring availability.

This separation means you can grant MQTT ACLs at the topic level—giving an automation system access to device control while restricting bridge administration to a management interface.

*Reference: MQTT topics and messages (base topic, bridge topics, device topics).*

### 2. Friendly Name versus IEEE Address

Zigbee devices have two identifiers. The **IEEE address** is a globally unique 64-bit hardware address (often displayed in hexadecimal, such as `0x00158d0001a2b3c4`). The **friendly name** is a human-readable label you assign when the device joins your network (such as `office_plug` or `living_room_lamp`).

Most device-level commands use the friendly name because it's easier to remember and type. Many bridge-level commands accept either identifier through fields named `id` (for IEEE address) or `name` (for friendly name), depending on the specific command. When in doubt, consult the command's documentation for the expected field names.

*Reference: MQTT topics and messages (device request payloads).*

### 3. The Standard Device Triplet

For any paired device, Zigbee2MQTT creates three topics that form a consistent interface:

- **State topic**: `zigbee2mqtt/<friendly_name>` — The bridge publishes the device's current state here whenever it changes. Subscribe to this topic to monitor the device.
- **Set topic**: `zigbee2mqtt/<friendly_name>/set` — Publish commands here to control the device. The payload is a JSON object with the properties you want to change.
- **Get topic**: `zigbee2mqtt/<friendly_name>/get` — Publish here to request the current value of a property. The device will respond by publishing its state.

This triplet provides a uniform interface across all device types. Whether you're controlling a smart plug, a temperature sensor, or a color-changing bulb, the pattern remains the same—only the available properties change.

*Reference: MQTT topics and messages (device set/get topics).*

### 4. Requests, Responses, and Transactions

Bridge-level operations use a request/response pattern. To issue a command, you publish a JSON payload to a request topic. The bridge processes the command and publishes the result to the corresponding response topic.

The topic structure follows a predictable convention:

- **Request**: `zigbee2mqtt/bridge/request/<action>`
- **Response**: `zigbee2mqtt/bridge/response/<action>`

Many requests accept an optional `transaction` field. If you include it, Zigbee2MQTT echoes the value back in the response. This enables correlation when you're issuing multiple concurrent commands—your client can match responses to their originating requests by comparing transaction identifiers.

*Reference: MQTT topics and messages (request/response patterns).*

---

## Part II: Core Topic Taxonomy

With the foundational concepts established, we can survey the complete topic hierarchy. This section catalogs the topics you'll encounter, organized by purpose.

### A. Bridge State and Metadata

These topics provide visibility into Zigbee2MQTT's operational state and configuration. They are read-only; you observe them by subscribing.

| Topic | Purpose |
|-------|---------|
| `zigbee2mqtt/bridge/state` | Running status of the Zigbee2MQTT process |
| `zigbee2mqtt/bridge/info` | Version, configuration, and coordinator information |
| `zigbee2mqtt/bridge/config` | Runtime configuration settings |
| `zigbee2mqtt/bridge/logging` | Log output (subscribe to receive log messages) |
| `zigbee2mqtt/bridge/definitions` | Supported device definitions and converters |
| `zigbee2mqtt/bridge/devices` | List of all paired devices with their properties |
| `zigbee2mqtt/bridge/groups` | List of configured groups |
| `zigbee2mqtt/bridge/health` | Health status information |
| `zigbee2mqtt/bridge/event` | Events such as device join, leave, and interview completion |
| `zigbee2mqtt/bridge/extensions` | Loaded extensions |
| `zigbee2mqtt/bridge/converters` | Loaded custom converters |

The `bridge/devices` topic is particularly useful for automation. It provides a complete inventory of your network, including each device's IEEE address, friendly name, model, manufacturer, and supported features.

*Reference: MQTT topics and messages (bridge topics).*

### B. Device State and Availability

Each device has its own topic namespace. The primary topics are:

| Topic | Purpose |
|-------|---------|
| `zigbee2mqtt/<friendly_name>` | Device state, including all exposed properties |
| `zigbee2mqtt/<friendly_name>/availability` | Online/offline status |

The state topic publishes a JSON object whenever the device reports a change. The exact properties depend on the device type—a smart plug might report `state`, `power`, `voltage`, and `current`, while a temperature sensor reports `temperature` and `humidity`.

The availability topic publishes `online` or `offline` based on whether the device is responding. You can use this to build automations that avoid sending commands to unreachable devices or to alert when a device goes missing.

*Reference: MQTT topics and messages (device topics).*

---

## Part III: Command Catalog (Bridge Requests)

This section enumerates the bridge-level commands you can issue. Each command follows the pattern established earlier: publish to `zigbee2mqtt/bridge/request/<command>` and receive the result on `zigbee2mqtt/bridge/response/<command>`. All payloads are JSON objects.

### 1. Commissioning and Joining

When you want to add new devices to your network, you must first open the network for joining.

**Permit Join**

- **Topic**: `zigbee2mqtt/bridge/request/permit_join`
- **Payload**: `{ "time": <seconds>, "device": "<friendly_name>" | null, "coordinator": "<friendly_name>" | null }`
- **Notes**: The `time` field specifies how long to keep the network open. Setting `time` to `0` immediately closes the network to new devices. The optional `device` field restricts joining to a specific router device (useful for mesh placement). The `coordinator` field can direct joining through a specific coordinator in multi-coordinator setups.

For security, keep join windows short. An open network allows any Zigbee device within range to attempt pairing.

*Reference: MQTT topics and messages (permit_join).*

### 2. Health, Coordinator, and Lifecycle

These commands help you monitor and manage the Zigbee2MQTT process itself.

**Health Check**

- **Topic**: `zigbee2mqtt/bridge/request/health_check`
- **Payload**: `{}`
- **Notes**: Returns the health status of the Zigbee2MQTT process. Useful for monitoring systems.

**Coordinator Check**

- **Topic**: `zigbee2mqtt/bridge/request/coordinator_check`
- **Payload**: `{}`
- **Notes**: Checks the status of the Zigbee coordinator hardware. This is supported only by specific coordinator types; consult your adapter's documentation.

**Restart Zigbee2MQTT**

- **Topic**: `zigbee2mqtt/bridge/request/restart`
- **Payload**: `{}`
- **Notes**: Gracefully restarts the Zigbee2MQTT process. This is useful after configuration changes that require a restart.

*Reference: MQTT topics and messages (health_check, coordinator_check, restart).*

### 3. Network Map and Diagnostics

Understanding your mesh network's topology can help diagnose connectivity issues.

**Network Map**

- **Topic**: `zigbee2mqtt/bridge/request/networkmap`
- **Payload**: `{ "type": "graphviz" | "raw" | "plantuml", "routes": true | false }`
- **Notes**: Generates a map of your Zigbee network. The `type` field selects the output format—`graphviz` produces DOT format for visualization tools, `raw` provides machine-readable JSON, and `plantuml` generates PlantUML diagrams. When `routes` is `true`, the map includes routing table information.

Be aware that generating a network map queries every device in your network. On large networks, this can take significant time and may make Zigbee2MQTT temporarily less responsive.

*Reference: MQTT topics and messages (networkmap).*

### 4. Inventory and Metadata Queries

These commands retrieve information about your network's current state.

**Get Devices List**

- **Topic**: `zigbee2mqtt/bridge/request/devices`
- **Payload**: `{}`
- **Notes**: Returns the complete list of paired devices with their properties.

**Get Groups List**

- **Topic**: `zigbee2mqtt/bridge/request/groups`
- **Payload**: `{}`
- **Notes**: Returns all configured groups and their members.

**Get Definitions**

- **Topic**: `zigbee2mqtt/bridge/request/definitions`
- **Payload**: `{ "properties": ["exposes", "supports_ota", ...] }`
- **Notes**: Returns device definitions. The optional `properties` array filters which definition fields to include.

**Get Extensions**

- **Topic**: `zigbee2mqtt/bridge/request/extensions`
- **Payload**: `{}`
- **Notes**: Returns the list of loaded extensions.

*Reference: MQTT topics and messages (devices, groups, definitions, extensions).*

### 5. Bridge Configuration and Backups

These commands modify Zigbee2MQTT's runtime configuration and create backups.

**Set Bridge Options**

- **Topic**: `zigbee2mqtt/bridge/request/options`
- **Payload**: `{ "options": { "log_level": "debug" } }`
- **Notes**: Updates runtime configuration options. The response may include a `restart_required` field indicating whether the change needs a restart to take effect.

**Backup**

- **Topic**: `zigbee2mqtt/bridge/request/backup`
- **Payload**: `{}`
- **Notes**: Creates a backup of the Zigbee2MQTT state. The response includes a base64-encoded ZIP file containing the backup data.

*Reference: MQTT topics and messages (options, backup).*

### 6. Device Management

These commands manage individual devices in your network.

**Rename Device**

- **Topic**: `zigbee2mqtt/bridge/request/device/rename`
- **Payload**: `{ "from": "old_name", "to": "new_name" }`
- **Notes**: Changes a device's friendly name. This updates the device list and adjusts topic routing accordingly.

**Remove Device**

- **Topic**: `zigbee2mqtt/bridge/request/device/remove`
- **Payload**: `{ "id": "<IEEE address>", "force": true | false, "block": true | false }`
- **Notes**: Removes a device from the network. If `force` is `true`, the device is removed from the database even if it cannot be reached. If `block` is `true`, the device is prevented from rejoining.

**Interview Device**

- **Topic**: `zigbee2mqtt/bridge/request/device/interview`
- **Payload**: `{ "id": "<IEEE address>" }`
- **Notes**: Re-interviews a device to refresh its capabilities. Useful when a device's behavior doesn't match its reported features.

**Configure Device**

- **Topic**: `zigbee2mqtt/bridge/request/device/configure`
- **Payload**: `{ "id": "<IEEE address>" }`
- **Notes**: Re-runs the device configuration process. This can resolve issues where a device joined but wasn't configured correctly.

**Set Device Options**

- **Topic**: `zigbee2mqtt/bridge/request/device/options`
- **Payload**: `{ "id": "<IEEE address>", "options": { ... } }`
- **Notes**: Updates device-specific options such as reporting intervals or transition times.

*Reference: MQTT topics and messages (device management).*

### 7. Binding and Reporting

Zigbee binding creates direct communication paths between devices, bypassing the coordinator. Reporting configuration controls how often devices send updates.

**Bind**

- **Topic**: `zigbee2mqtt/bridge/request/device/bind`
- **Payload**: `{ "from": "<device_or_group>", "to": "<device_or_group>" }`
- **Notes**: Creates a binding from one device (or group) to another. For example, binding a switch to a light allows the switch to control the light directly.

**Unbind**

- **Topic**: `zigbee2mqtt/bridge/request/device/unbind`
- **Payload**: `{ "from": "<device_or_group>", "to": "<device_or_group>" }`
- **Notes**: Removes an existing binding.

**Clear All Binds**

- **Topic**: `zigbee2mqtt/bridge/request/device/binds/clear`
- **Payload**: `{ "device": "<friendly_name>" }`
- **Notes**: Removes all bindings from a device. Useful when repurposing a device.

**Configure Reporting**

- **Topic**: `zigbee2mqtt/bridge/request/device/reporting/configure`
- **Payload**: `{ "id": "<device>", "cluster": "genOnOff", "attribute": "onOff", "minimum_report_interval": 1, "maximum_report_interval": 300, "reportable_change": 1 }`
- **Notes**: Configures how often a device reports an attribute. The intervals are in seconds. The `reportable_change` threshold determines how much the value must change to trigger an immediate report.

**Read Reporting Configuration**

- **Topic**: `zigbee2mqtt/bridge/request/device/reporting/read`
- **Payload**: `{ "id": "<device>", "cluster": "genOnOff", "attribute": "onOff" }`
- **Notes**: Retrieves the current reporting configuration for an attribute.

*Reference: Binding documentation and MQTT topics and messages (reporting requests).*

### 8. Groups

Zigbee groups allow you to control multiple devices with a single command. When you send a command to a group, all member devices receive it simultaneously—this is faster than sending individual commands.

**Add Group**

- **Topic**: `zigbee2mqtt/bridge/request/group/add`
- **Payload**: `{ "friendly_name": "living_room", "id": 1 }`
- **Notes**: Creates a new group. The `id` is the Zigbee group ID (a number); the `friendly_name` is the human-readable label.

**Remove Group**

- **Topic**: `zigbee2mqtt/bridge/request/group/remove`
- **Payload**: `{ "id": 1, "force": true | false }`
- **Notes**: Deletes a group. If `force` is `true`, the group is removed from the database even if member devices cannot be notified.

**Rename Group**

- **Topic**: `zigbee2mqtt/bridge/request/group/rename`
- **Payload**: `{ "from": "old_name", "to": "new_name" }`
- **Notes**: Changes a group's friendly name.

**Add Device to Group**

- **Topic**: `zigbee2mqtt/bridge/request/group/members/add`
- **Payload**: `{ "group": "living_room", "device": "lamp", "endpoint": 1 }`
- **Notes**: Adds a device to a group. The `endpoint` field specifies which device endpoint to add (relevant for multi-endpoint devices).

**Remove Device from Group**

- **Topic**: `zigbee2mqtt/bridge/request/group/members/remove`
- **Payload**: `{ "group": "living_room", "device": "lamp", "endpoint": 1 }`
- **Notes**: Removes a device from a group.

**Remove All Group Members**

- **Topic**: `zigbee2mqtt/bridge/request/group/members/remove_all`
- **Payload**: `{ "group": "living_room" }`
- **Notes**: Removes all devices from a group. Omit the `group` field to remove a specific device from all groups it belongs to.

**Set Group Options**

- **Topic**: `zigbee2mqtt/bridge/request/group/options`
- **Payload**: `{ "id": 1, "options": { ... } }`
- **Notes**: Updates group-specific options.

*Reference: Groups documentation (group add/remove/rename/members/options).*

### 9. Touchlink

Touchlink is a Zigbee commissioning method that uses physical proximity. It's commonly used for factory-resetting devices that are not responding normally.

**Identify**

- **Topic**: `zigbee2mqtt/bridge/request/touchlink/identify`
- **Payload**: `{ "ieee_address": "<device>", "channel": 11 }`
- **Notes**: Causes a device to identify itself (typically by blinking). Useful for verifying which physical device corresponds to an address.

**Factory Reset**

- **Topic**: `zigbee2mqtt/bridge/request/touchlink/factory_reset`
- **Payload**: `{ "ieee_address": "<device>", "channel": 11 }`
- **Notes**: Factory-resets a device via Touchlink. The device must be in close proximity to your coordinator.

**Scan**

- **Topic**: `zigbee2mqtt/bridge/request/touchlink/scan`
- **Payload**: `{}`
- **Notes**: Scans for Touchlink-capable devices nearby.

*Reference: Touchlink documentation (supported adapters and request topics).*

### 10. OTA Updates

Many Zigbee devices support over-the-air firmware updates. Zigbee2MQTT can manage this process.

**Check for Update**

- **Topic**: `zigbee2mqtt/bridge/request/device/ota_update/check`
- **Payload**: `{ "id": "<device>" }`
- **Notes**: Checks whether a firmware update is available for the device.

**Start Update**

- **Topic**: `zigbee2mqtt/bridge/request/device/ota_update/update`
- **Payload**: `{ "id": "<device>" }`
- **Notes**: Begins the firmware update process. Progress is published on the device's state topic.

**Stop Update**

- **Topic**: `zigbee2mqtt/bridge/request/device/ota_update/stop`
- **Payload**: `{ "id": "<device>" }`
- **Notes**: Cancels an in-progress update.

**Schedule an Update**

- **Topic**: `zigbee2mqtt/bridge/request/device/ota_update/schedule`
- **Payload**: `{ "id": "<device>" }`
- **Notes**: Schedules an update to begin when the device next checks in.

**Unschedule Update**

- **Topic**: `zigbee2mqtt/bridge/request/device/ota_update/unschedule`
- **Payload**: `{ "id": "<device>" }`
- **Notes**: Cancels a scheduled update.

**Downgrade Commands**

- **Topics**: `.../ota_update/downgrade` and `.../ota_update/schedule/downgrade`
- **Notes**: These commands install older firmware versions. Use with caution—downgrading can cause compatibility issues.

*Reference: OTA updates documentation (check/update/stop/schedule/downgrade).*

### 11. Install Codes and Extensions

These commands handle advanced features such as Zigbee 3.0 install codes and custom extensions.

**Install Code Add**

- **Topic**: `zigbee2mqtt/bridge/request/install_code/add`
- **Payload**: `{ "value": "<install code>" }`
- **Notes**: Adds an install code for secure device joining. The device must support Zigbee 3.0 install code commissioning.

**Extension Save/Remove**

- **Topics**: `zigbee2mqtt/bridge/request/extension/save` and `zigbee2mqtt/bridge/request/extension/remove`
- **Notes**: Manages custom JavaScript extensions that modify Zigbee2MQTT's behavior.

**Converter Save/Remove**

- **Topics**: `zigbee2mqtt/bridge/request/converter/save` and `zigbee2mqtt/bridge/request/converter/remove`
- **Notes**: Manages custom device converters for unsupported devices.

*Reference: MQTT topics and messages (install_code, extension, converter requests).*

---

## Part IV: Device Commands via /set and /get

While bridge requests manage the network, device commands control individual devices. This section covers the patterns for direct device interaction.

### 1. The Generic /set Pattern

To control a device, publish a JSON object to its `/set` topic. The object's keys correspond to the device's exposed properties.

- **Topic**: `zigbee2mqtt/<friendly_name>/set`
- **Payload**: A JSON object with property names and values

The available properties depend on the device type. A smart plug might support `state` (on/off), while a dimmable bulb supports `state`, `brightness`, `color`, and `color_temp`. Consult the device's page on the Zigbee2MQTT website for its complete list of exposed properties.

**Example: Turn on a smart plug**

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/set' -m '{"state":"ON"}'
```

You can set multiple properties in a single command:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/living_room_lamp/set' -m '{"state":"ON","brightness":200,"color_temp":350}'
```

*Reference: MQTT topics and messages (device /set usage).*

### 2. The /get Pattern

To request the current value of a property, publish to the device's `/get` topic. The payload indicates which property you want to read.

- **Topic**: `zigbee2mqtt/<friendly_name>/get`
- **Payload**: `{ "<property>": "" }` or `{ "<property>": null }`

The device will respond by publishing its current state to the state topic.

**Example: Request a device's power-on behavior**

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/get' -m '{"power_on_behavior": ""}'
```

Not all devices support `/get` for all properties. Some values are only updated when the device reports them (for example, on change or at configured intervals).

*Reference: MQTT topics and messages (device /get usage).*

### 3. Direct ZCL Read/Write

For advanced use cases, you can issue raw Zigbee Cluster Library (ZCL) commands. This is useful when a device has capabilities that Zigbee2MQTT's standard interface doesn't expose.

- **Topic**: `zigbee2mqtt/<friendly_name>/set`
- **Payload**: `{ "read": { "cluster": "<cluster>", "attributes": ["<attribute>", ...] } }` or `{ "write": { "cluster": "<cluster>", "attributes": { "<attribute>": <value> } } }`

Use this feature only when you understand the device's ZCL clusters and attributes. Incorrect values can misconfigure or confuse the device.

**Example: Read the onOff attribute from the genOnOff cluster**

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/set' -m '{"read": {"cluster": "genOnOff", "attributes": ["onOff"]}}'
```

*Reference: MQTT topics and messages (read/write support).*

---

## Part V: Operational Patterns and Constraints

This section addresses practical considerations for working with the Zigbee2MQTT MQTT interface.

### 1. Availability Tracking

Zigbee2MQTT publishes availability status for each device on the topic `zigbee2mqtt/<friendly_name>/availability`. The payload is either `online` or `offline`.

This information is valuable for automation. Before sending a command, you can check whether the device is reachable. You can also trigger alerts when critical devices go offline—a missing temperature sensor might indicate a dead battery.

Subscribe to availability with a wildcard to monitor your entire network:

```bash
mosquitto_sub -h localhost -t 'zigbee2mqtt/+/availability' -v
```

*Reference: MQTT topics and messages (availability).*

### 2. Legacy API Deprecation

Older versions of Zigbee2MQTT used a different API structure. The modern request/response API described in this document is now the default. The legacy API is deprecated and disabled in recent versions unless you explicitly enable it in the configuration.

If you're migrating from an older installation or following outdated tutorials, update your topic patterns and payloads to match the current API.

*Reference: MQTT topics and messages (API notes).*

### 3. Security and Permissions

The Zigbee2MQTT MQTT interface provides powerful administrative capabilities. Consider these security practices:

- **MQTT broker ACLs**: Configure access control lists to restrict who can publish to sensitive topics. Bridge request topics (`zigbee2mqtt/bridge/request/#`) can reconfigure your network, remove devices, or trigger restarts.
- **Short join windows**: Keep `permit_join` enabled only when actively pairing devices. An open network allows any Zigbee device in range to attempt joining.
- **Network map moderation**: The `networkmap` command queries every device on your network. On busy networks, use it sparingly to avoid degrading responsiveness.

*Reference: MQTT topics and messages (permit_join, networkmap notes).*

---

## Part VI: Reference Map (Authoritative Sources)

The Zigbee2MQTT documentation is the ground truth for command parameters, payload formats, and device-specific behavior. Use these pages as your primary reference.

| Resource | URL |
|----------|-----|
| MQTT topics and messages | https://www.zigbee2mqtt.io/guide/usage/mqtt_topics_and_messages.html |
| Adapter settings | https://www.zigbee2mqtt.io/guide/configuration/adapter-settings.html |
| Groups | https://www.zigbee2mqtt.io/guide/usage/groups.html |
| Binding | https://www.zigbee2mqtt.io/guide/usage/binding.html |
| Touchlink | https://www.zigbee2mqtt.io/guide/usage/touchlink.html |
| OTA updates | https://www.zigbee2mqtt.io/guide/usage/ota_updates.html |
| Health check | https://www.zigbee2mqtt.io/guide/usage/health_check.html |

---

## Usage Examples

The following examples demonstrate common operations. All commands assume you have `mosquitto_pub` and `mosquitto_sub` installed and that your MQTT broker is running on `localhost:1883`.

### Example 1: Enable Pairing for 60 Seconds

Open the network for new devices to join:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/permit_join' -m '{"time": 60}'
```

After 60 seconds, the network automatically closes. To close it immediately:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/permit_join' -m '{"time": 0}'
```

### Example 2: Rename a Device

Change a device's friendly name from `old_name` to `new_name`:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/device/rename' -m '{"from": "old_name", "to": "new_name"}'
```

After renaming, the device's topics change accordingly. Update any automations that reference the old name.

### Example 3: Create a Group and Add a Device

First, create the group:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/group/add' -m '{"friendly_name": "living_room", "id": 1}'
```

Then add a device to it:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/group/members/add' -m '{"group": "living_room", "device": "lamp"}'
```

Now you can control all devices in the group with a single command:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/living_room/set' -m '{"state":"ON"}'
```

### Example 4: Generate a Network Map

Request a Graphviz-format network map:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/networkmap' -m '{"type": "graphviz"}'
```

Subscribe to the response topic to receive the result:

```bash
mosquitto_sub -h localhost -t 'zigbee2mqtt/bridge/response/networkmap' -C 1
```

### Example 5: Start an OTA Firmware Update

Check whether an update is available:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/device/ota_update/check' -m '{"id": "office_plug"}'
```

If an update is available, start it:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/device/ota_update/update' -m '{"id": "office_plug"}'
```

Monitor progress by subscribing to the device's state topic:

```bash
mosquitto_sub -h localhost -t 'zigbee2mqtt/office_plug' -v
```

---

## Related

- `reference/01-zigbee2mqtt-third-reality-gen-2-plug-quickstart.md`
- `reference/02-diary.md`
