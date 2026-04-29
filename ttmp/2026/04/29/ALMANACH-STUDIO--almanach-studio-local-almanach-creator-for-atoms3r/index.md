---
Title: Almanach Studio — Local Almanach Creator for AtomS3R
Ticket: ALMANACH-STUDIO
Status: active
Topics:
    - esp32-s3
    - http-server
    - react
    - static-embed
    - thermal-printer
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../Downloads/almanach-studio(1).jsx
      Note: Source JSX — the Almanach Studio React component (imported into sources/)
    - Path: ../../../../../../../../../code/wesen/2026-03-29--serve-claude-experiments/pkg/server/server.go
      Note: Reference — Go server that serves JSX with Babel-in-browser fallback
    - Path: ../../../../../../../../../code/wesen/2026-03-29--serve-claude-experiments/pkg/server/templates/jsx-host.html
      Note: Reference — HTML template for hosting JSX artifacts with React/Babel
    - Path: ../../../../../../../../../code/wesen/obsidian-vault/Projects/2026/04/29/ARTICLE - Almanach Studio - A Self-Hosted Thermal Almanac Designer for ESP32.md
      Note: Deep technical report in Obsidian vault — textbook-style article covering full system architecture
    - Path: 0017-atoms3r-web-ui/main/CMakeLists.txt
      Note: Reference — CMakeLists with EMBED_TXTFILES for embedding web assets into firmware
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: Reference firmware — AtomS3R HTTP server with embedded assets and wildcard URIs
    - Path: 0017-atoms3r-web-ui/web/vite.config.ts
      Note: Reference — Vite config for building SPA into firmware-embeddable assets
    - Path: components/httpd_assets_embed/httpd_assets_embed.c
      Note: Reusable component for sending embedded assets via esp_http_server
    - Path: components/httpd_assets_embed/include/httpd_assets_embed.h
      Note: Header for the httpd_assets_embed helper
    - Path: stoms3r/main/CMakeLists.txt
      Note: Updated EMBED_TXTFILES with almanach assets
    - Path: stoms3r/main/web_server.c
      Note: SToMS3R HTTP server — now serves Almanach Studio at /almanach and /almanach/bundle.js
    - Path: stoms3r/web/almanach
      Note: esbuild build pipeline for Almanach Studio SPA
    - Path: ttmp/2026/04/29/ALMANACH-STUDIO--almanach-studio-local-almanach-creator-for-atoms3r/reference/02-almanach-studio-project-report.md
      Note: Copy of the project report in the ticket workspace
ExternalSources:
    - local:almanach-studio(1).jsx
Summary: ""
LastUpdated: 2026-04-29T14:56:44.077434872-04:00
WhatFor: ""
WhenToUse: ""
---





# Almanach Studio — Local Almanach Creator for AtomS3R

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32-s3
- http-server
- react
- static-embed
- thermal-printer

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
