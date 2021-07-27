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

moo.otypes.load_types('nwqueueadapters/networktoqueue.jsonnet')
moo.otypes.load_types('nwqueueadapters/networkobjectreceiver.jsonnet')

moo.otypes.load_types('trigger/triggerprimitivemaker.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd, 
import dunedaq.rcif.cmd as rccmd # AddressedCmd, 
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.appfwk.app as app # AddressedCmd,
import dunedaq.trigger.triggerprimitivemaker as tpm
import dunedaq.nwqueueadapters.networktoqueue as ntoq
import dunedaq.nwqueueadapters.networkobjectreceiver as nor

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
):
    cmd_data = {}

    # Define modules and queues
    queue_specs = [
        app.QueueSpec(inst="tpset_q", kind='FollySPSCQueue', capacity=1000),
    ]

    mod_specs = [

        mspec(f"tpset_subscriber", "NetworkToQueue", [
            app.QueueInfo(name="output", inst=f"tpset_q", dir="output")
        ]),

        mspec("tps_sink", "TPSetSink", [
            app.QueueInfo(name="tpset_source", inst="tpset_q", dir="input"),
        ]),
    ]

    cmd_data['init'] = app.Init(queues=queue_specs, modules=mod_specs)

    cmd_data['conf'] = acmd([
        ("tpset_subscriber", ntoq.Conf(msg_type="dunedaq::trigger::TPSet",
                                             msg_module_name="TPSetNQ",
                                             receiver_config=nor.Conf(ipm_plugin_type="ZmqSubscriber",
                                                                      address='tcp://127.0.0.1:5000',
                                                                      subscriptions=["foo"]))
         ),
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
