local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.tasetsink";
local s = moo.oschema.schema(ns);

local hier = {
    filename: s.string("Filename"),

    conf : s.record("Conf", [
        s.field("output_filename", hier.filename,
                doc="Output filename"),
    ], doc="TASetSink configuration"),

  
};

moo.oschema.sort_select(hier, ns)
