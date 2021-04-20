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

moo.otypes.load_types('trigger/intervaltccreator.jsonnet')
moo.otypes.load_types('trigger/moduleleveltrigger.jsonnet')
moo.otypes.load_types('trigger/fakedataflow.jsonnet')
moo.otypes.load_types('trigger/faketimestampeddatagenerator.jsonnet')
moo.otypes.load_types('trigger/timingtriggercandidatemaker.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd, 
import dunedaq.rcif.cmd as rccmd # AddressedCmd, 
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.appfwk.app as app # AddressedCmd,
import dunedaq.trigger.intervaltccreator as itcc
import dunedaq.trigger.moduleleveltrigger as mlt
import dunedaq.trigger.fakedataflow as fdf
import dunedaq.trigger.faketimestampeddatagenerator as ftsdg
import dunedaq.trigger.timingtriggercandidatemaker as ttcm

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
        TRIGGER_RATE_HZ: float = 1.0,
        OUTPUT_PATH: str = ".",
        TOKEN_COUNT: int = 10,
        CLOCK_SPEED_HZ: int = 50000000,
):
    """
    { item_description }
    """
    cmd_data = {}

    # Derived parameters
    TRIGGER_INTERVAL_NS = math.floor((1e9/TRIGGER_RATE_HZ))

    # Define modules and queues
    queue_bare_specs = [
        app.QueueSpec(inst="timestamped_data_q", kind='FollyMPMCQueue', capacity=1000),
        app.QueueSpec(inst="time_sync_q", kind='FollySPSCQueue', capacity=1000),
        app.QueueSpec(inst="token_q", kind='FollySPSCQueue', capacity=2000),
        app.QueueSpec(inst="trigger_decision_q", kind='FollySPSCQueue', capacity=2000),
        app.QueueSpec(inst="trigger_candidate_q", kind='FollySPSCQueue', capacity=2000),
    ]

    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = app.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))


    mod_specs = [
        mspec("fdf", "FakeDataFlow", [
            app.QueueInfo(name="trigger_decision_source", inst="trigger_decision_q", dir="input"),
            app.QueueInfo(name="trigger_complete_sink", inst="token_q", dir="output"),
        ]),
        
        mspec("mlt", "ModuleLevelTrigger", [
            app.QueueInfo(name="token_source", inst="token_q", dir="input"),
            app.QueueInfo(name="trigger_decision_sink", inst="trigger_decision_q", dir="output"),
            app.QueueInfo(name="trigger_candidate_source", inst="trigger_candidate_q", dir="output"),
        ]),

        mspec("ftsdg", "FakeTimeStampedDataGenerator", [
            app.QueueInfo(name="time_stamped_data_sink", inst="timestamped_data_q", dir="output"),
        ]),

        mspec("ttcm", "TimingTriggerCandidateMaker", [
            app.QueueInfo(name="input", inst="timestamped_data_q", dir="input"),
            app.QueueInfo(name="output", inst="trigger_candidate_q", dir="output"),
        ]),
    ]

    cmd_data['init'] = app.Init(queues=queue_specs, modules=mod_specs)

    cmd_data['conf'] = acmd([
        ("fdf", fdf.ConfParams(
        )),
        ("mlt", mlt.ConfParams(
            links=[idx for idx in range(3)],
            initial_token_count=TOKEN_COUNT                    
        )),
        ("ftsdg", ftsdg.Conf(
            sleep_time=TRIGGER_INTERVAL_NS,
            frequency=CLOCK_SPEED_HZ
        )),
        ("ttcm", ttcm.Conf(
        )),
    ])

    startpars = rccmd.StartParams(run=1, disable_data_storage=False)
    cmd_data['start'] = acmd([
        ("fdf", startpars),
        ("mlt", startpars),
        ("ftsdg", startpars),
        ("ttcm", startpars),
    ])

    cmd_data['stop'] = acmd([
        ("fdf", None),
        ("mlt", None),
        ("ftsdg", None),
        ("ttcm", None),
    ])

    cmd_data['pause'] = acmd([
        ("", None)
    ])

    resumepars = rccmd.ResumeParams(trigger_interval_ticks=50000000)
    cmd_data['resume'] = acmd([
        ("mlt", resumepars)
    ])

    cmd_data['scrap'] = acmd([
        ("", None)
    ])

    return cmd_data
