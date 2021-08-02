# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

from pprint import pprint
# Load configuration types
import moo.otypes

moo.otypes.load_types('trigger/triggerprimitivemaker.jsonnet')
moo.otypes.load_types('trigger/triggerzipper.jsonnet')
moo.otypes.load_types('trigger/faketpcreatorheartbeatmaker.jsonnet')

# Import new types
import dunedaq.trigger.triggerprimitivemaker as tpm
import dunedaq.trigger.triggerzipper as tzip
import dunedaq.trigger.faketpcreatorheartbeatmaker as ftpchm

from ..util import module

#===============================================================================
def generate(
        INPUT_FILES: str,
        SLOWDOWN_FACTOR: float
):
    # Derived parameters
    CLOCK_FREQUENCY_HZ = 50000000 / SLOWDOWN_FACTOR

    modules = {}

    for i,input_file in enumerate(INPUT_FILES):
        modules[f"tpm{i}"] = module(plugin="TriggerPrimitiveMaker",
                                 connections={"tpset_sink": f"ftpchm{i}.tpset_source"},
                                 conf=tpm.ConfParams(filename=input_file,
                                                     number_of_loops=-1, # Infinite
                                                     tpset_time_offset=0,
                                                     tpset_time_width=10000,
                                                     clock_frequency_hz=CLOCK_FREQUENCY_HZ,
                                                     maximum_wait_time_us=1000,
                                                     region_id=0,
                                                     element_id=i))
        modules[f"ftpchm{i}"] = module(plugin="FakeTPCreatorHeartbeatMaker",
                                       connections={"tpset_sink" : "zip.input"},
                                       conf=ftpchm.Conf(heartbeat_interval = 50000))

    modules.update({
        "zip": module(plugin="TPZipper",
                      connections={"output" : "tpsink.tpset_source"},
                      conf=tzip.ConfParams(cardinality=len(INPUT_FILES),
                                           max_latency_ms=1000,
                                           region_id=0,
                                           element_id=0)
                      ),
        
        "tpsink": module(plugin="TPSetSink",
                         connections={},
                         conf=None)
    })

    return modules
