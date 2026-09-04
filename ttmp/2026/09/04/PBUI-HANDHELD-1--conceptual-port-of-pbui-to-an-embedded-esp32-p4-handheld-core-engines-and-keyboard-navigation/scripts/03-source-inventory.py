#!/usr/bin/env python3
"""Emit a reproducible, line-anchored inventory. Usage: script PBUI_ROOT FW_ROOT."""
import hashlib, json, pathlib, subprocess, sys
pbui, fw = map(pathlib.Path, sys.argv[1:3])
patterns = {
 'pbui': {
  'src/presentation/actions/typeGraph.ts': ['export function', 'Cycle detection', 'BFS ancestor'],
  'src/presentation/context/selector.ts': ['export function'],
  'src/presentation/actions/conditions.ts': ['export type Condition', 'export function evaluateCondition'],
  'src/presentation/actions/availability.ts': ['export type Availability'],
  'src/presentation/actions/registry.ts': ['export function', 'Guaranteed collisions'],
  'src/presentation/actions/resolve.ts': ['export function resolveActions', 'partition by action', 'bind winners'],
  'src/presentation/actions/perform.ts': ['export function evaluateFresh'],
  'src/presentation/acceptance/resolve.ts': ['function finish', 'export function resolveAcceptance', 'Subtyping', 'Only acceptance'],
  'src/presentation/interaction/accept.ts': ['export type AcceptState', 'export type AcceptEvent', 'export function acceptStep', 'case "choose"'],
  'src/presentation/interaction/activation.ts': ['export function activationOutcome'],
  'src/presentation/model/compile.ts': ['function mergeFragments', 'export function compilePresentation', 'function snapshot', 'function validateActiveScopes'],
  'src/presentation/model/types.ts': ['export interface'],
  'src/presentation/relations/system.ts': ['export function', 'function discoverable', 'function execute', 'matches(reference'],
  'packages/pbui-ecommerce/src/presentation/actions.ts': ['export const SHOP_TYPES', 'export function'],
  'packages/pbui-ecommerce/src/presentation/runtime.tsx': ['export function'],
  'packages/workbench-core/src/session.ts': ['export interface WorkbenchSession', 'export function repairSession'],
  'packages/workbench-core/src/queries.ts': ['export function canClose'],
 },
 'firmware': {
  'components/picocalc_lcd/picocalc_lcd.c': ['#define LCD_DEFAULT_SPI_HZ', 'static esp_err_t lcd_tx', 'static esp_err_t lcd_ensure_dma_buffer', 'esp_err_t picocalc_lcd_blit_rect', 'esp_err_t picocalc_lcd_blit_row'],
  'components/picocalc_keyboard/picocalc_keyboard.c': ['esp_err_t picocalc_keyboard_recover', 'esp_err_t picocalc_keyboard_read_register', 'esp_err_t picocalc_keyboard_poll_event', 'const char *picocalc_keyboard_key_name'],
  'components/picocalc_keyboard/include/picocalc_keyboard.h': ['#define PICOCALC_KBD_'],
  'components/visual_repl/visual_repl.cpp': ['static uint16_t s_row_pixels', 'glyph5x7', 'std::toupper', 'void draw_cell', 'esp_err_t render_text_row', 'esp_err_t visual_repl_dump_text'],
  '0102-esp32-p4-visual-quickjs-repl/README.md': ['source '],
  '0102-esp32-p4-visual-quickjs-repl/sdkconfig.defaults': ['CONFIG_'],
  '0102-esp32-p4-visual-quickjs-repl/CMakeLists.txt': ['set(EXTRA_COMPONENT_DIRS'],
  '0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp': ['bool key_to_picojs_token', 'void keyboard_task', 'PICOCALC_KBD_STATE_PRESSED', 'xTaskCreate'],
 }
}
report = {'repositories':{},'files':[]}
for name, root in [('pbui',pbui),('firmware',fw)]:
 report['repositories'][name]={'root':str(root),'commit':subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip()}
 for rel, needles in patterns[name].items():
  p=root/rel
  raw=p.read_bytes()
  anchors=[{'line':i,'text':s.strip()} for i,s in enumerate(raw.decode().splitlines(),1) if any(n in s for n in needles)]
  report['files'].append({'repo':name,'path':rel,'sha256':hashlib.sha256(raw).hexdigest(),'lines':len(raw.splitlines()),'anchors':anchors})
print(json.dumps(report,indent=2))
