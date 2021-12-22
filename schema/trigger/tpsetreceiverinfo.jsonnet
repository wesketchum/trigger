// This is the application info schema used by the fake data producer.
// It describes the information object structure passed by the application
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.trigger.tpsetreceiverinfo");

local info = {
   uint8  : s.number("uint8", "u8", doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("tpsets_received", self.uint8, 0, doc="Number of received TPSets"),
   ], doc="TPSet Receiver information")
};

moo.oschema.sort_select(info)