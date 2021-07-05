# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

from pprint import pprint
pprint(moo.io.default_load_path)
# Load configuration types
import moo.otypes
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('appfwk/app.jsonnet')

moo.otypes.load_types('trigger/triggerprimitivemaker.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd, 
import dunedaq.rcif.cmd as rccmd # AddressedCmd, 
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.appfwk.app as app # AddressedCmd,
import dunedaq.trigger.triggerprimitivemaker as tpm

from appfwk.utils import mcmd, mrccmd, mspec

import json
import math
from pprint import pprint


#===============================================================================
def acmd(mods: list) -> cmd.CmdObj:
    """ 
    Helper function to create appfwk's Commands addressed to modules.
        
    :param      cmdid:  The coommand id
    :type       cmdid:  str
    :param      mods:   List of module name/data structures 
    :type       mods:   list
    
    :returns:   A constructed Command object
    :rtype:     dunedaq.appfwk.cmd.Command
    """
    return cmd.CmdObj(
        modules=cmd.AddressedCmds(
            cmd.AddressedCmd(match=m, data=o)
            for m,o in mods
        )
    )

#===============================================================================
def generate(
        OUTPUT_PATH: str,
        INPUT_FILE: str,
        SLOWDOWN_FACTOR: float
):
    cmd_data = {}

    # Derived parameters
    CLOCK_FREQUENCY_HZ = 50000000 / SLOWDOWN_FACTOR

    # Define modules and queues
    queue_specs = [
        app.QueueSpec(inst="tpset_q", kind='FollySPSCQueue', capacity=1000),
    ]

    mod_specs = [
        mspec("tpm", "TriggerPrimitiveMaker", [
            app.QueueInfo(name="tpset_sink", inst="tpset_q", dir="output"),
        ]),
        
        mspec("tps_sink", "TPSetSink", [
            app.QueueInfo(name="tpset_source", inst="tpset_q", dir="input"),
        ]),
    ]

    cmd_data['init'] = app.Init(queues=queue_specs, modules=mod_specs)

    cmd_data['conf'] = acmd([
        ("tpm", tpm.ConfParams(
            filename=INPUT_FILE,
            number_of_loops=-1, # Infinite
            tpset_time_offset=0,
            tpset_time_width=10000,
            clock_frequency_hz=CLOCK_FREQUENCY_HZ,
            maximum_wait_time_us=1000
        )),
    ])

    startpars = rccmd.StartParams(run=1, disable_data_storage=False)
    cmd_data['start'] = acmd([
        ("tpm", startpars),
        ("tps_sink", startpars),
    ])

    cmd_data['pause'] = acmd([])

    cmd_data['resume'] = acmd([])
    
    cmd_data['stop'] = acmd([
        ("tpm", None),
        ("tps_sink", None),
    ])

    cmd_data['scrap'] = acmd([
        ("tpm", None),
    ])

    return cmd_data
