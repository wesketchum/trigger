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
moo.otypes.load_types('trigger/triggeractivitymaker.jsonnet')
moo.otypes.load_types('trigger/triggerzipper.jsonnet')
moo.otypes.load_types('trigger/faketpcreatorheartbeatmaker.jsonnet')
moo.otypes.load_types('trigger/tasetsink.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd,
import dunedaq.rcif.cmd as rccmd # AddressedCmd,
import dunedaq.appfwk.cmd as cmd # AddressedCmd,
import dunedaq.appfwk.app as app # AddressedCmd,
import dunedaq.trigger.triggerprimitivemaker as tpm
import dunedaq.trigger.triggeractivitymaker as tam
import dunedaq.trigger.triggerzipper as tzip
import dunedaq.trigger.faketpcreatorheartbeatmaker as ftpchm
import dunedaq.trigger.tasetsink as tasetsink

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

#FIXME maybe one day, triggeralgs will define schemas... for now allow a dictionary of 4byte int, 4byte floats, and strings
moo.otypes.make_type(schema='number', dtype='i4', name='temp_integer', path='temptypes')
moo.otypes.make_type(schema='number', dtype='f4', name='temp_float', path='temptypes')
moo.otypes.make_type(schema='string', name='temp_string', path='temptypes')
def make_moo_record(conf_dict,name,path='temptypes'):
    fields = []
    for pname,pvalue in conf_dict.items():
        typename = None
        if type(pvalue) == int:
            typename = 'temptypes.temp_integer'
        elif type(pvalue) == float:
            typename = 'temptypes.temp_float'
        elif type(pvalue) == str:
            typename = 'temptypes.temp_string'
        else:
            raise Exception(f'Invalid config argument type: {type(value)}')
        fields.append(dict(name=pname,item=typename))
    moo.otypes.make_type(schema='record', fields=fields, name=name, path=path)

#===============================================================================
def generate(
        INPUT_FILES: str,
        OUTPUT_FILE: str,
        SLOWDOWN_FACTOR: float,
        NUMBER_OF_LOOPS: int,
        ACTIVITY_CONFIG: dict = dict(min_pts=3),
):
    cmd_data = {}

    # Derived parameters
    CLOCK_FREQUENCY_HZ = 50000000 / SLOWDOWN_FACTOR

    # Define modules and queues
    queue_specs = [
        app.QueueSpec(inst=f"tpset_q", kind='FollyMPMCQueue', capacity=100000),
        app.QueueSpec(inst="zipped_tpset_q", kind='FollyMPMCQueue', capacity=100000),
        app.QueueSpec(inst='taset_q', kind='FollySPSCQueue', capacity=100000),
    ]

    mod_specs = [
        mspec(f"tpm{i}", "TriggerPrimitiveMaker", [
            app.QueueInfo(name="tpset_sink", inst=f"tpset_q", dir="output"),
        ]) for i in range(len(INPUT_FILES))
    ] + [
        mspec("zip", "TPZipper", [
            app.QueueInfo(name="input", inst="tpset_q", dir="input"),
            app.QueueInfo(name="output", inst="zipped_tpset_q", dir="output"),
        ]),
        
        mspec('tam', 'TriggerActivityMaker', [ # TPSet -> TASet
            app.QueueInfo(name='input', inst='zipped_tpset_q', dir='input'),
            app.QueueInfo(name='output', inst='taset_q', dir='output'),
        ]),

        mspec("ta_sink", "TASetSink", [
            app.QueueInfo(name="taset_source", inst="taset_q", dir="input"),
        ]),
    ]

    
    cmd_data['init'] = app.Init(queues=queue_specs, modules=mod_specs)

    make_moo_record(ACTIVITY_CONFIG,'ActivityConf','temptypes')
    import temptypes
    
    cmd_data['conf'] = acmd([
        (f"tpm{i}", tpm.ConfParams(
            filename=input_file,
            number_of_loops=NUMBER_OF_LOOPS,
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
            max_latency_ms=100,
            region_id=0,
            element_id=0
        )),
        ('tam', tam.Conf(
            activity_maker="TriggerActivityMakerDBSCANPlugin",
            geoid_region=0, # Fake placeholder
            geoid_element=0, # Fake placeholder
            window_time=10000, # should match whatever makes TPSets, in principle
            buffer_time=625000, # 10ms in 62.5 MHz ticks
            activity_maker_config=temptypes.ActivityConf(**ACTIVITY_CONFIG)
        )),
        ('ta_sink', tasetsink.Conf(
            output_filename=OUTPUT_FILE
        ))
    ])

    start_order=[
        "ta_sink",
        "tam",
        "zip",
    ] + [
        f'ftpchm{i}' for i in range(len(INPUT_FILES))
    ] + [
        f'tpm{i}' for i in range(len(INPUT_FILES)) 
    ]
    
    startpars = rccmd.StartParams(run=1, disable_data_storage=False)
    cmd_data['start'] = acmd([(mod, startpars) for mod in start_order])

    cmd_data['pause'] = acmd([])

    cmd_data['resume'] = acmd([])

    cmd_data['stop'] =  acmd([(mod, None) for mod in start_order[::-1] ])

    cmd_data['scrap'] =  acmd([(mod, None) for mod in start_order[::-1] ])

    return cmd_data
