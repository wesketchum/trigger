local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.fakedataflow";
local s = moo.oschema.schema(ns);

local types = {
  
  conf : s.record("ConfParams", [
  
  ], doc="FakeDataFlow configuration parameters"),

  
};

moo.oschema.sort_select(types, ns)
