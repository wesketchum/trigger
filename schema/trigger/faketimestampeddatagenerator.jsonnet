local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.faketimestampeddatagenerator";
local s = moo.oschema.schema(ns);

local types = {
    size: s.number("Size", dtype="u8"),

    conf: s.record("Conf", [

      s.field("sleep_time", self.size, 1000000000,
        doc="Sleep time between thread sends"),

      s.field("frequency", self.size, 50000000,
        doc="Assumed clock frequency in Hz (for current-timestamp estimation)"),


    ], doc="FakeTimeStampedDataGenerator configuration parameters"),

};

moo.oschema.sort_select(types, ns)