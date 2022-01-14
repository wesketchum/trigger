local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.tpwriter";
local s = moo.oschema.schema(ns);

local types = {
    pathname : s.string("Path", moo.re.hierpath, doc="File path, file name"),

    conf: s.record("ConfParams", [
        s.field("filename", self.pathname,
                doc="File name of output file for trigger primitives"),
    ], doc="TPWriter configuration"),

};

moo.oschema.sort_select(types, ns)
