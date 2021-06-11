local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.triggerprimitivemaker";
local s = moo.oschema.schema(ns);

local types = {
    pathname : s.string("Path", "path", doc="File path, file name"),
    loops: s.number("loops", dtype="u8", doc="Number of loops"),
    rows: s.number("rows", dtype="u8", doc="Number of rows"),

    conf: s.record("ConfParams", [
        s.field("filename", self.pathname, "/tmp/example.csv",
                doc="File name of input csv file for trigger primitives"),
        s.field("number_of_loops", self.loops, 1,
                doc="Number of times the data in the csv file are sent"),
        s.field("chunk_offset", self.rows, 1,
                doc="Offset for the TPSet boundaries [ n*width+offset, (n+1)*width+offset ]"),
        s.field("chunk_width", self.rows, 1,
                doc="Width for the TPSet boundaries [ n*width+offset, (n+1)*width+offset ]"),
    ], doc="TriggerPrimitiveFromFile configuration"),

};

moo.oschema.sort_select(types, ns)
