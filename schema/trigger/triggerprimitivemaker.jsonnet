local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.triggerprimitivemaker";
local s = moo.oschema.schema(ns);

local types = {
    pathname : s.string("Path", moo.re.hierpath, doc="File path, file name"),
    loops: s.number("loops", dtype="u8", doc="Number of loops"),
    rows: s.number("rows", dtype="u8", doc="Number of rows"),
    freq: s.number("freq", dtype="u8", doc="A frequency"),
    microseconds: s.number("microseconds", dtype="u8", doc="Microseconds"),
    region : s.number("region", "u2", doc="Region ID for GeoID"),
    element : s.number("element", "u4", doc="Element ID for GeoID"),

    conf: s.record("ConfParams", [
        s.field("filename", self.pathname, "/tmp/example.csv",
                doc="File name of input csv file for trigger primitives"),
        s.field("number_of_loops", self.loops, 1,
                doc="Number of times the data in the csv file are sent"),
        s.field("tpset_time_offset", self.rows, 1,
                doc="Offset for the TPSet boundaries: [ n*width+offset, (n+1)*width+offset ]"),
        s.field("tpset_time_width", self.rows, 1,
                doc="Width int time of the generated TPSets"),
        s.field("clock_frequency_hz", self.freq, 50000000,
                doc="Simulated clock frequency in Hz"),
        s.field("maximum_wait_time_us", self.microseconds, 1000,
                doc="Maximum wait time until the running flag is checked in microseconds"),
        s.field("region_id", self.region, 0,
                doc="Detector region ID to be reported as the source of the TPs"),
        s.field("element_id", self.element, 0,
                doc="Detector element ID to be reported as the source of the TPs"),
    ], doc="TriggerPrimitiveMaker configuration"),

};

moo.oschema.sort_select(types, ns)
