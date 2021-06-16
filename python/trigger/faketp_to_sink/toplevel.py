import json
import os
import rich.traceback
from rich.console import Console
from os.path import exists, join


# Add -h as default help option
CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

console = Console()

def generate_boot( faketp_spec: dict) -> dict:
    """
    Generates boot informations

    :param      faketp_spec:  The faketp specifications
    :type       faketp_spec:  dict

    :returns:   { description_of_the_return_value }
    :rtype:     dict
    """

    # TODO: think how to best handle this bit.
    # Who is responsible for this bit? Not minidaqapp?
    # Is this an appfwk configuration fragment?
    daq_app_specs = {
        "daq_application_ups" : {
            "comment": "Application profile based on a full dbt runtime environment",
            "env": {
               "DBT_AREA_ROOT": "getenv" 
            },
            "cmd": [
                "CMD_FAC=rest://localhost:${APP_PORT}",
                "INFO_SVC=file://info_${APP_ID}_${APP_PORT}.json",
                "cd ${DBT_AREA_ROOT}",
                "source dbt-setup-env.sh",
                "dbt-setup-runtime-environment",
                "cd ${APP_WD}",
                "daq_application --name ${APP_ID} -c ${CMD_FAC} -i ${INFO_SVC}"
            ]
        },
        "daq_application" : {
            "comment": "Application profile using  PATH variables (lower start time)",
            "env":{
                "CET_PLUGIN_PATH": "getenv",
                "DUNEDAQ_SHARE_PATH": "getenv",
                "LD_LIBRARY_PATH": "getenv",
                "PATH": "getenv",
                "DUNEDAQ_ERS_DEBUG_LEVEL": "1"
            },
            "cmd": [
                "CMD_FAC=rest://localhost:${APP_PORT}",
                "INFO_SVC=file://info_${APP_NAME}_${APP_PORT}.json",
                "cd ${APP_WD}",
                "daq_application --name ${APP_NAME} -c ${CMD_FAC} -i ${INFO_SVC}"
            ]
        }
    }

    boot = {
        "env": {
            "DUNEDAQ_ERS_VERBOSITY_LEVEL": 1
        },
        "apps": {
            faketp_spec["name"]: {
                "exec": "daq_application",
                "host": "host_faketp",
                "port": faketp_spec["port"]
            }
        },
        "hosts": {
            "host_faketp": faketp_spec["host"]
        },
        "response_listener": {
            "port": 56789
        },
        "exec": daq_app_specs
    }

    console.log("Boot data")
    console.log(boot)
    return boot


import click

@click.command(context_settings=CONTEXT_SETTINGS)
@click.option('-s', '--slowdown-factor', default=1.0)
@click.option('-f', '--input-file', type=click.Path())
@click.argument('json_dir', type=click.Path())
def cli(slowdown_factor, input_file, json_dir):
    """
      JSON_DIR: Json file output folder
    """
    console.log("Loading faketp config generator")
    from . import faketp_to_sink
    console.log(f"Generating configs")

    cmd_data_faketp = faketp_to_sink.generate(
        INPUT_FILE = input_file,
        SLOWDOWN_FACTOR = slowdown_factor,
        OUTPUT_PATH = json_dir,
    )

    console.log("faketp cmd data:", cmd_data_faketp)

    if exists(json_dir):
        raise RuntimeError(f"Directory {json_dir} already exists")

    data_dir = join(json_dir, 'data')
    os.makedirs(data_dir)

    app_faketp="faketp"

    jf_faketp = join(data_dir, app_faketp)

    cmd_set = ["init", "conf", "start", "stop", "pause", "resume", "scrap"]
    for app,data in ((app_faketp, cmd_data_faketp),):
        console.log(f"Generating {app} command data json files")
        for c in cmd_set:
            with open(f'{join(data_dir, app)}_{c}.json', 'w') as f:
                json.dump(data[c].pod(), f, indent=4, sort_keys=True)


    console.log(f"Generating top-level command json files")
    start_order = [app_faketp]
    for c in cmd_set:
        with open(join(json_dir,f'{c}.json'), 'w') as f:
            cfg = {
                "apps": { app: f'data/{app}_{c}' for app in (app_faketp, ) }
            }
            if c == 'start':
                cfg['order'] = start_order
            elif c == 'stop':
                cfg['order'] = start_order[::-1]

            json.dump(cfg, f, indent=4, sort_keys=True)


    console.log(f"Generating boot json file")
    with open(join(json_dir,'boot.json'), 'w') as f:
        cfg = generate_boot(
            faketp_spec = {
                "name": app_faketp,
                "host": "localhost",
                "port": 3333
            },
        )
        json.dump(cfg, f, indent=4, sort_keys=True)
    console.log(f"MDAapp config generated in {json_dir}")


if __name__ == '__main__':

    try:
            cli(show_default=True, standalone_mode=True)
    except Exception as e:
            console.print_exception()
