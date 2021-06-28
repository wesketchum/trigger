local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.buffercreator";
local s = moo.oschema.schema(ns);

local types = {
    size: s.number("Size", dtype="i8"),

    conf: s.record("Conf", [

      s.field("sleep_time", self.size, 1000000000,
        doc="Sleep time between thread read"),

      s.field("buffer_size", self.size, 100,
        doc="Maximum number of TPSet that buffer will store. If maximum reached, oldest is deleted to give room for new entry"),


    ], doc="BufferManager configuration parameters"),

};

moo.oschema.sort_select(types, ns)