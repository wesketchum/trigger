local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.moduleleveltrigger";
local s = moo.oschema.schema(ns);

local types = {
  region_id : s.number("region_id", "u2"),
  element_id : s.number("element_id", "u4"),
  system_type : s.string("system_type"),

  geoid : s.record("GeoID", [s.field("region", self.region_id, doc="" ),
      s.field("element", self.element_id, doc="" ),
      s.field("system", self.system_type, doc="" )],
      doc="GeoID"),

  linkvec : s.sequence("link_vec", self.geoid),
  token_count: s.number("token_count", dtype="i4"),
  
  conf : s.record("ConfParams", [
    s.field("links", self.linkvec,
      doc="List of link identifiers that may be included into trigger decision"),

    s.field("initial_token_count", self.token_count, 0,
      doc="Number of trigger tokens to start the run with"),


  ], doc="ModuleLevelTrigger configuration parameters"),

  
};

moo.oschema.sort_select(types, ns)
