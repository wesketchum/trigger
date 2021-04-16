local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.triggeractivitymaker";
local s = moo.oschema.schema(ns);

local types = {
  name: s.string("Name", ".*",
    doc="Name of a plugin etc"),

  any: s.any("Data", doc="Any"),

  conf: s.record("Conf", [
    s.field("activity_maker", self.name,
      doc="Name of the activity maker implementation to be used via plugin"),
    s.field("activity_maker_config", self.any,
      doc="Configuration for the activity maker implementation"),
    ], doc="TriggerActivityMaker configuration"),

};

moo.oschema.sort_select(types, ns)
