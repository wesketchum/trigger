local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.fakedataflow";
local s = moo.oschema.schema(ns);

local types = {
  count: s.number("count", dtype="i4"),
  ms: s.number("milliseconds", dtype="i4"),
  probability: s.number("probability", dtype="f8"),
  
  conf : s.record("ConfParams", [
    s.field("forget_decision_prob", self.probability,
      doc="Probability to completely forget about a received TD"),
    s.field("hold_decision_prob", self.probability,
      doc="Probability to hold a TD and send TDT later"),
    s.field("hold_max_size", self.count,
      doc="Maximum number of held TDs before sending TDTs for held TDs"),
    s.field("hold_min_size", self.count,
      doc="Minimum number of held TDs to keep when sending TDTs for held TDs"),
    s.field("hold_min_ms", self.ms,
      doc="Minimum number ms to hold TDs once hold_max_size is met"),
    s.field("release_randomly_prob", self.probability,
      doc="Probability to release a random held TD out of order")
  ], doc="FakeDataFlow configuration parameters")
  
};

moo.oschema.sort_select(types, ns)
