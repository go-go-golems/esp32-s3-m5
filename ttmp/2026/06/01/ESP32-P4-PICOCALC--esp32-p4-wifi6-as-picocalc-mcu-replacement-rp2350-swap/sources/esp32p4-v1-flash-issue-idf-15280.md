### Answers checklist.

- [x] I have read the documentation [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/) and the issue is not addressed there.
- [x] I have updated my IDF branch (master or release) to the latest version and checked that the issue is present there.
- [x] I have searched the issue tracker for a similar issue and not found a similar issue.

### General issue report

I am using esp-idf 5.4.

I have new ESP32-P4 v1.0 Silicon.
Before I had v0.1 Silicon. working on a prototype that mirrors most of the hardware on ESP32-P4-eval board.

But since changing to ESP32-P4 v1.0 Silicon i have issues programming this new chip.

I get this error "A fatal error occurred: bootloader/bootloader.bin requires chip revision in range [v0.1 - v0.99] (this chip is revision v1.0). Use --force to flash anyway."

Please look at my attached log file, old vs new silicon log, and sdkconfig below.

[idf_py_flash_issue_012425.txt](https://github.com/user-attachments/files/18543328/idf_py_flash_issue_012425.txt)

[sdkconfig.txt](https://github.com/user-attachments/files/18542917/sdkconfig.txt)

![Image](https://github.com/user-attachments/assets/806dd6b2-b684-41f0-bdeb-227e639107aa)



