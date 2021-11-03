local moo = import "moo.jsonnet";
local ns = "dunedaq.trigger.tpsetreceiver";
local s = moo.oschema.schema(ns);

local types = {
    region_number : s.number("region_number", "u2",
                     doc="Region container type for GeoID"),	
    element_number : s.number("element_number", "u4",
                     doc="Element container type for GeoID"),

    queueid : s.string("queue_id", doc="Parameter that configure TPSetReceiver's queues"),
    system_type : s.string("system_type", doc="Parameter that configure TPSetReceiver"),

    geoidqueue : s.record("geoidinst", [s.field("region", self.region_number, doc="" ) , 
                                        s.field("element", self.element_number, doc="" ), 
                                        s.field("system", self.system_type, doc="" ),
                                        s.field("queueinstance", self.queueid, doc="" ) ], 
                           doc="TPSetReceiver configuration"),

    mapgeoidqueue : s.sequence("mapgeoidqueue",  self.geoidqueue, doc="Map of geoids queues" ),

    timeout: s.number( "Timeout", "u8", 
                       doc="Queue timeout in milliseconds" ),    
    topic: s.string("Name", doc="Name for the topic that TPSetReceiver listens on"),
                        
    conf: s.record("ConfParams", [ s.field("map", self.mapgeoidqueue, doc="" ), 
                                   s.field("general_queue_timeout", self.timeout, 100, 
                                           doc="General indication for timeout"),
                                   s.field("topic", self.topic, "", doc="Topic for listening" )
                                  ] , 
                   doc="TPSetReceiver configuration")

};

moo.oschema.sort_select(types, ns)
