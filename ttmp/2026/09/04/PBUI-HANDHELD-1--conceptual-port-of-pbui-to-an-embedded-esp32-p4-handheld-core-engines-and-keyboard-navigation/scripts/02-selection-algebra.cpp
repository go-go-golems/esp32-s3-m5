// Design experiment, not production firmware. g++ -std=c++20 -Wall -Wextra
// -Werror -fsanitize=address,undefined -g 02-selection-algebra.cpp -o /tmp/pbui-algebra
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <random>
#include <set>
#include <tuple>
#include <vector>

enum class Status { available, unavailable, inapplicable, hidden };
Status first_failure(Status a, Status b) { return a == Status::available ? b : a; }
struct Rank { int distance, scope, priority; };
bool better(Rank a, Rank b) {
  // Do not negate priority: INT_MIN must remain well-defined.
  if (a.distance != b.distance) return a.distance < b.distance;
  if (a.scope != b.scope) return a.scope < b.scope;
  return a.priority > b.priority;
}
bool equal(Rank a, Rank b) {
  return a.distance == b.distance && a.scope == b.scope && a.priority == b.priority;
}
struct Candidate { int id; Rank rank; Status status; };
struct Summary { bool empty = true; Rank rank{}; std::set<int> ids; };
Summary merge(Summary a, const Summary& b) {
  if (a.empty) return b;
  if (b.empty) return a;
  if (better(a.rank, b.rank)) return a;
  if (better(b.rank, a.rank)) return b;
  a.ids.insert(b.ids.begin(), b.ids.end());
  return a;
}
Summary singleton(const Candidate& c) {
  if (c.status == Status::inapplicable) return {};
  return {false, c.rank, {c.id}};
}
Summary folded(const std::vector<Candidate>& cs) {
  Summary out;
  for (const auto& c : cs) out = merge(out, singleton(c));
  return out;
}
Summary sorted_oracle(std::vector<Candidate> cs) {
  cs.erase(std::remove_if(cs.begin(), cs.end(), [](auto c) { return c.status == Status::inapplicable; }), cs.end());
  std::sort(cs.begin(), cs.end(), [](auto a, auto b) { return better(a.rank, b.rank); });
  if (cs.empty()) return {};
  Summary result{false, cs.front().rank, {}};
  for (auto c : cs) if (equal(c.rank, result.rank)) result.ids.insert(c.id);
  return result;
}
bool same(const Summary& a, const Summary& b) {
  return a.empty == b.empty && (a.empty || (equal(a.rank,b.rank) && a.ids==b.ids));
}
int main() {
  constexpr std::array states{Status::available,Status::unavailable,Status::inapplicable,Status::hidden};
  for (auto a : states) for (auto b : states) for (auto c : states)
    assert(first_failure(first_failure(a,b),c)==first_failure(a,first_failure(b,c)));
  assert(first_failure(Status::hidden,Status::unavailable)!=first_failure(Status::unavailable,Status::hidden));
  assert(better({0,0,INT32_MAX},{0,0,INT32_MIN}));
  std::mt19937 rng(17029);
  for (int trial=0;trial<5000;++trial) {
    std::vector<Candidate> cs;
    const auto n=rng()%40;
    for (unsigned i=0;i<n;++i) cs.push_back({int(i),{int(rng()%4),int(rng()%4),int(rng()%9)-4},states[rng()%4]});
    auto oracle=sorted_oracle(cs);
    for(int p=0;p<8;++p) { std::shuffle(cs.begin(),cs.end(),rng); assert(same(folded(cs),oracle)); }
    Summary a,b,c;
    for(unsigned i=0;i<n;++i) {
      auto& dst=i%3==0?a:i%3==1?b:c;
      dst=merge(dst,singleton(cs[i]));
    }
    assert(same(merge(merge(a,b),c),merge(a,merge(b,c))));
    assert(same(merge(a,b),merge(b,a)));
    assert(same(merge(a,a),a));
  }
  // Truncating a candidate list can delete a hidden winner: overflow must error.
  std::vector<Candidate> dangerous{{0,{2,0,0},Status::available},{1,{0,0,0},Status::hidden}};
  assert(folded(dangerous).ids==std::set<int>{1});
  dangerous.resize(1);
  assert(folded(dangerous).ids==std::set<int>{0});
  // Visible shortcuts derive from the clipped occurrence list, never whole doc.
  std::vector<int> document(100); for(int i=0;i<100;++i)document[i]=i;
  std::vector<int> visible(document.begin()+40,document.begin()+56);
  std::vector<int> picks(visible.begin(),visible.begin()+9);
  assert(picks.front()==40 && picks.back()==48);
  std::puts("PASS: 64 condition associativity cases; 5000 selection worlds x 8 permutations; summary associativity/commutativity/idempotence; overflow counterexample; viewport picks; priority extremes");
}
