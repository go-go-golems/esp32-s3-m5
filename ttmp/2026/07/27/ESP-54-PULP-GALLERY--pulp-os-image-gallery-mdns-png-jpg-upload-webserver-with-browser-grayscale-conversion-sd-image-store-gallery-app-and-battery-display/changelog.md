# Changelog

## 2026-07-27

- Initial workspace created


## 2026-07-27

Ticket created: ESP-54 design for mDNS (pulp.local), browser-side 4-bit grayscale image upload webserver + SD store, image gallery app (extends latent DrawOpKind::Bitmap), and battery/charging display. Design doc, diary, and tasks authored; key files related.

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/components/s3paper_m5/src/m5_backend.cpp — Bitmap op skip arm — central implementation gap identified


## 2026-07-27

Ticket delivered: doctor passed clean (vocabulary added: mdns, image-upload, gallery, battery); 8 key files related to design doc; bundle uploaded to reMarkable at /ai/2026/07/27/ESP-54-PULP-GALLERY (single PDF, toc-depth 2), verified via cloud ls.

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/07/27/ESP-54-PULP-GALLERY--pulp-os-image-gallery-mdns-png-jpg-upload-webserver-with-browser-grayscale-conversion-sd-image-store-gallery-app-and-battery-display/design-doc/01-intern-guide-image-gallery-mdns-upload-webserver-and-battery-display.md — Primary deliverable (intern design + implementation guide)


## 2026-07-27

Implemented all four features: mDNS (pulp.local, espressif/mdns managed component), browser upload webserver (POST /images/upload + .g4 format + crop/quantize page), gallery app + bitmap blit (DrawOpKind::Bitmap now implemented in m5_backend), battery display (battery singleton + home glyph). Host tests PASS 38186 (+12 bitmap), build OK, device probes 19-22 PASS, end-to-end browser upload verified.

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os/main/app_images.cpp — Image catalog + display + .g4 format + upload mailbox


## 2026-07-27

Added browser auto-rotate for landscape images (rotate 90deg + fill/cover, full-width span for sideways viewing) with a versioned default-index migration; diagnosed the diagonal stripes as the test image itself (rasterizer verified correct via horizontal-bands image); added 'images' console command; stored all test scripts in scripts/ with a README. Build OK, host tests PASS 38186.

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os/main/net_serve.cpp — Upload page auto-rotate + versioned default-index migration

