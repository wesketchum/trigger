local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.intervaltccreator";
local s = moo.oschema.schema(ns);

local types = {
  ticks: s.number("ticks", dtype="i8"),
  freq: s.number("frequency", dtype="u8"),
  repeat_count: s.number("repeat_count", dtype="i4"),
  timestamp_estimation: s.enum("timestamp_estimation", ["kTimeSync", "kSystemClock"]),
  
  conf : s.record("ConfParams", [
    s.field("trigger_interval_ticks", self.ticks, 64000000,
      doc="Interval between triggers in 16 ns time ticks (default 1.024 s) "),

    s.field("clock_frequency_hz", self.ticks, 50000000,
      doc="Assumed clock frequency in Hz (for current-timestamp estimation)"),

    s.field("repeat_trigger_count", self.repeat_count, 1,
      doc="Number of times to send each trigger decision (for overlapping trigger tests)"),
      
    s.field("stop_burst_count", self.repeat_count, 0,
      doc="Number of triggers to send ~simultaneously at stop (for queue draining tests)"),

    s.field("timestamp_method", self.timestamp_estimation, "kTimeSync",
      doc="Use TimeSync queues to estimate timestamp (instead of system clock)"),
    
  ], doc="IntervalTCCreator configuration parameters"),

};

moo.oschema.sort_select(types, ns)
