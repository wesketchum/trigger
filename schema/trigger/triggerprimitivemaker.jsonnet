local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.triggerprimitivemaker";
local s = moo.oschema.schema(ns);

local types = {
  name: s.string("Name", ".*",
    doc="Name of a plugin etc"),

  any: s.any("Data", doc="Any"),

  conf: s.record("Conf", [
    s.field("primitive_maker", self.name,
      doc="Name of the primitive maker implementation to be used via plugin"),
    s.field("primitive_maker_config", self.any,
      doc="Configuration for the primitive maker implementation"),
    ], doc="TriggerPrimitiveMaker configuration"),

};

moo.oschema.sort_select(types, ns)
