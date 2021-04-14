local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.moduleleveltrigger";
local s = moo.oschema.schema(ns);

local types = {
  linkid: s.number("link_id", dtype="i4"),
  linkvec : s.sequence("link_vec", self.linkid),
  token_count: s.number("token_count", dtype="i4"),
  
  conf : s.record("ConfParams", [
    s.field("links", self.linkvec,
      doc="List of link identifiers that may be included into trigger decision"),

    s.field("initial_token_count", self.token_count, 0,
      doc="Number of trigger tokens to start the run with"),


  ], doc="ModuleLevelTrigger configuration parameters"),

  
};

moo.oschema.sort_select(types, ns)
