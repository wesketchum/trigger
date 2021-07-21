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
moo.otypes.load_types('trigger/triggerzipper.jsonnet')
moo.otypes.load_types('trigger/faketpcreatorheartbeatmaker.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd,
import dunedaq.rcif.cmd as rccmd # AddressedCmd,
import dunedaq.appfwk.cmd as cmd # AddressedCmd,
import dunedaq.appfwk.app as app # AddressedCmd,
import dunedaq.trigger.triggerprimitivemaker as tpm
import dunedaq.trigger.triggerzipper as tzip
import dunedaq.trigger.faketpcreatorheartbeatmaker as ftpchm

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
        INPUT_FILES: str,
        SLOWDOWN_FACTOR: float
):
    cmd_data = {}

    # Derived parameters
    CLOCK_FREQUENCY_HZ = 50000000 / SLOWDOWN_FACTOR

    # Define modules and queues
    queue_specs = [
        app.QueueSpec(inst=f"tpset_q{i}", kind='FollySPSCQueue', capacity=1000)
        for i in range(len(INPUT_FILES))
    ] +    [
        app.QueueSpec(inst="tpset_plus_hb_q", kind='FollyMPMCQueue', capacity=1000),
        app.QueueSpec(inst="zipped_tpset_q", kind='FollyMPMCQueue', capacity=1000),
    ]

    mod_specs = [
        mspec(f"tpm{i}", "TriggerPrimitiveMaker", [
            app.QueueInfo(name="tpset_sink", inst=f"tpset_q{i}", dir="output"),
        ]) for i in range(len(INPUT_FILES))
    ] + [

        mspec(f"ftpchm{i}", "FakeTPCreatorHeartbeatMaker", [
            app.QueueInfo(name="tpset_source", inst=f"tpset_q{i}", dir="input"),
            app.QueueInfo(name="tpset_sink", inst="tpset_plus_hb_q", dir="output"),
        ]) for i in range(len(INPUT_FILES))

    ] + [
        mspec("zip", "TPZipper", [
            app.QueueInfo(name="input", inst="tpset_plus_hb_q", dir="input"),
            app.QueueInfo(name="output", inst="zipped_tpset_q", dir="output"),
        ]),
        mspec("tps_sink", "TPSetSink", [
            app.QueueInfo(name="tpset_source", inst="zipped_tpset_q", dir="input"),
        ]),
    ]

    cmd_data['init'] = app.Init(queues=queue_specs, modules=mod_specs)

    cmd_data['conf'] = acmd([
        (f"tpm{i}", tpm.ConfParams(
            filename=input_file,
            number_of_loops=-1, # Infinite
            tpset_time_offset=0,
            tpset_time_width=10000,
            clock_frequency_hz=CLOCK_FREQUENCY_HZ,
            maximum_wait_time_us=1000,
            region_id=0,
            element_id=i,
        )) for i,input_file in enumerate(INPUT_FILES)
    ] + [
        (f"ftpchm{i}", ftpchm.Conf(
          heartbeat_interval = 50000
        )) for i in range(len(INPUT_FILES))
    ] + [
        ("zip", tzip.ConfParams(
            cardinality=len(INPUT_FILES),
            max_latency_ms=1000,
            region_id=0,
            element_id=0
        ))
    ])

    startpars = rccmd.StartParams(run=1, disable_data_storage=False)
    cmd_data['start'] = acmd(
        [
            ("zip", startpars),
            ("tps_sink", startpars),
        ] +
        [ (f'ftpchm{i}', startpars) for i in range(len(INPUT_FILES)) ] +
        [ (f'tpm{i}', startpars) for i in range(len(INPUT_FILES)) ]
    )

    cmd_data['pause'] = acmd([])

    cmd_data['resume'] = acmd([])

    cmd_data['stop'] = acmd(
        [ (f'tpm{i}', None) for i in range(len(INPUT_FILES)) ] +
        [ (f'ftpchm{i}', None) for i in range(len(INPUT_FILES)) ] +
        [
        ("zip", None),
        ("tps_sink", None),
    ])

    cmd_data['scrap'] = acmd(
        [ (f'ftpchm{i}', None) for i in range(len(INPUT_FILES)) ] +
        [ (f'tpm{i}', None) for i in range(len(INPUT_FILES)) ] +
        [
            ("zip", None)
        ]
    )

    return cmd_data
