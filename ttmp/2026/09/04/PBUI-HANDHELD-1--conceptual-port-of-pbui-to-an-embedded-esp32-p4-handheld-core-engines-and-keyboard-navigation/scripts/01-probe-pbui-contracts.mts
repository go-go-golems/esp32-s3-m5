// Investigation only: exercise actual PBUI source, not a firmware implementation.
// Usage: cd PBUI && pnpm exec tsx /absolute/path/to/this.mts "$PWD" /absolute/ticket
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { pathToFileURL } from 'node:url';
const [root, ticket] = process.argv.slice(2);
if (!root || !ticket) throw new Error('usage: probe PBUI_ROOT TICKET_ROOT');
const load = (p: string) => import(pathToFileURL(path.join(root, 'src/presentation', p)).href);
const { createPresentationTypeGraph } = await load('actions/typeGraph.ts');
const { createActionRegistry } = await load('actions/registry.ts');
const { createRelationSystem } = await load('relations/system.ts');
const { resolveAcceptance } = await load('acceptance/resolve.ts');
const { acceptStep } = await load('interaction/accept.ts');
const snapshot = { revision: 1, scopes: ['local', 'global'], modes: new Set(), capabilities: new Set(), product: {} };
const graph = createPresentationTypeGraph([
  { id: 'object', abstract: true }, { id: 'file', parents: ['object'] },
  { id: 'image', parents: ['file'] }, { id: 'mem' }, { id: 'segment' },
]);
const rule = (id: string, subject: string, status: string) => ({
  kind: 'rule', id, action: 'object.open', subject, match: 'subtypes', scopes: ['global'],
  metadata: { label: id }, test: () => ({ kind: status }), bind: () => ({ by: id }),
});
const query = { subject: { type: 'image', value: 'i' }, invocation: 'menu' };
const resolve = (g: any, rs: any[]) => createActionRegistry({ graph: g, scopes: ['global', 'local'], contributions: rs }).resolve(query, snapshot);
const suppressed = resolve(graph, [rule('generic', 'object', 'available'), rule('specific', 'image', 'hidden')]);
const prematureDrop = resolve(graph, [rule('generic', 'object', 'available')]);
assert.equal(suppressed.actions.length, 0);
assert.equal(prematureDrop.actions[0].candidateId, 'generic');
// Adding a redundant parent preserves reachability but changes shortest-distance rank.
const redundant = createPresentationTypeGraph([
  { id: 'object', abstract: true }, { id: 'file', parents: ['object'] },
  { id: 'image', parents: ['file', 'object'] },
]);
const rs = [rule('at-file', 'file', 'available'), rule('at-object', 'object', 'available')];
assert.equal(resolve(graph, rs).actions[0].candidateId, 'at-file');
assert.equal(resolve(redundant, rs).ambiguities.length, 1);
let applied = 0;
const relation = (id: string) => ({ id, from: 'mem', to: 'segment', match: 'exact', exposure: { acceptance: true },
  apply: () => { applied++; return { type: 'segment', value: 's1' }; } });
const relations = createRelationSystem({ graph, scopes: ['global'], relations: [relation('r1'), relation('r2')] });
const indirect = resolveAcceptance({ relations }, { types: ['segment'], prompt: '?' }, { type: 'mem', value: 'm1' }, snapshot);
assert.equal(indirect.kind, 'ambiguous');
assert.equal(indirect.options.length, 2); // Identical outputs do NOT eliminate relation ambiguity.
const before = applied;
const directFiltered = resolveAcceptance({ relations }, { types: ['mem', 'segment'], prompt: '?', filter: () => false }, { type: 'mem', value: 'm1' }, snapshot);
assert.equal(directFiltered.kind, 'none');
assert.equal(applied, before); // No relation fallback after direct filter rejection.
const request = { types: ['segment'], prompt: '?' };
let state = acceptStep({ kind: 'idle' }, { type: 'request', requestId: 10, request }).state;
state = acceptStep(state, { type: 'offer', reference: { type: 'mem', value: 'm1' }, resolution: indirect }).state;
const foreign = acceptStep(state, { type: 'choose', option: { relation: 'foreign', result: { type: 'file', value: 'wrong' } } });
assert.equal(foreign.effects[0].reference.type, 'file'); // Caller trust, not validation in machine.
const next = acceptStep({ kind: 'idle' }, { type: 'request', requestId: 11, request }).state;
const staleOffer = acceptStep(next, { type: 'offer', reference: { type: 'mem', value: 'm1' }, resolution: { kind: 'accepted', option: indirect.options[0] } });
assert.equal(staleOffer.effects[0].requestId, 11); // Offers have no request ID: async adapter must guard.
const jsx = fs.readFileSync(path.join(ticket, 'sources/pbui-handheld.jsx'), 'utf8');
const prefix = jsx.slice(jsx.indexOf('\n'), jsx.indexOf('export default function PBUIHandheld'));
const sandbox: any = {};
vm.createContext(sandbox);
vm.runInContext(prefix + '\nthis.probe = {availCmds, fold, MAXGI, TL, appLines, APPS};', sandbox);
const p = sandbox.probe;
assert.ok(p.availCmds([], 0).includes('newtile'));
const world = p.fold(p.MAXGI, {skip:[],taskSet:{},memForget:[],memPin:[],memUnpin:[],ctxEvict:[],ctxPin:[]});
const report = {
  kind: 'actual-source-contract-probes', assertions: 'passed',
  hidden_before_selection: suppressed.actions.length,
  deleting_hidden_before_selection_leaks: prematureDrop.actions[0].candidateId,
  redundant_edge: { before: resolve(graph, rs).actions[0].candidateId, after: resolve(redundant, rs).ambiguities },
  equal_result_relations: indirect,
  direct_filter_rejection: { result: directFiltered.kind, relation_calls: applied-before },
  foreign_choose_is_trusted: foreign.effects,
  stale_offer_without_request_id: staleOffer.effects,
  prototype: { empty_screen_offers: Array.from(p.availCmds([], 0)), timeline_entries: p.TL.length, max_cursor: p.MAXGI, final_edits: world.edits.length },
};
console.log(JSON.stringify(report, null, 2));
