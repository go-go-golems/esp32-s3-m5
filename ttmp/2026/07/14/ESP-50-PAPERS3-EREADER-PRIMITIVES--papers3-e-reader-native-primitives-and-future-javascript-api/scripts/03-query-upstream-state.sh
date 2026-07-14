#!/usr/bin/env bash
set -euo pipefail

printf '%s\n' '== Query date =='
date --iso-8601=seconds

printf '%s\n' '== M5GFX issues relevant to PaperS3 =='
for issue in 181 152 119; do
  printf '\n-- issue %s --\n' "$issue"
  gh api "repos/m5stack/M5GFX/issues/$issue" \
    --jq '{number,title,state,created_at,updated_at,closed_at,html_url,body,labels:[.labels[].name],comments}'
  gh api "repos/m5stack/M5GFX/issues/$issue/comments?per_page=100" \
    --jq '.[] | {created_at,user:.user.login,body,html_url}'
done

printf '%s\n' '== M5GFX latest release and recent releases =='
gh api repos/m5stack/M5GFX/releases/latest \
  --jq '{tag_name,published_at,html_url,name,body}'
gh api 'repos/m5stack/M5GFX/releases?per_page=10' \
  --jq '.[] | {tag_name,published_at,html_url,body}'

printf '%s\n' '== Panel_EPD commit history =='
gh api 'repos/m5stack/M5GFX/commits?path=src/lgfx/v1/platforms/esp32/Panel_EPD.cpp&per_page=30' \
  --jq '.[] | [.sha[0:12],.commit.author.date,.commit.message] | @tsv'

printf '%s\n' '== Issue 181 fix commit =='
gh api repos/m5stack/M5GFX/commits/33f8ce25e96903bc8d11122de81147d8a5cca39b \
  --jq '{sha,html_url,commit:.commit.message,files:[.files[]|{filename,status,additions,deletions,patch}]}'

printf '%s\n' '== Verify issue 181 fix is an ancestor of 0.2.25 =='
gh api repos/m5stack/M5GFX/compare/33f8ce25e96903bc8d11122de81147d8a5cca39b...ad9b814264d4e2000e9f30070002310bbccaffc9 \
  --jq '{status,ahead_by,behind_by,total_commits}'

printf '%s\n' '== M5GFX 0.2.17 ESP-IDF 5.4 release note =='
gh api repos/m5stack/M5GFX/releases/tags/0.2.17 \
  --jq '{tag_name,published_at,html_url,body}'

printf '%s\n' '== M5Unified latest release =='
gh api repos/m5stack/M5Unified/releases/latest \
  --jq '{tag_name,published_at,html_url,body}'

printf '%s\n' '== MQuickJS current head =='
gh api repos/bellard/mquickjs/commits/main \
  --jq '{sha,date:.commit.author.date,message:.commit.message,html_url}'
