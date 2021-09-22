import click
from rich.console import Console

# Add -h as default help option
CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

console = Console()


@click.command(context_settings=CONTEXT_SETTINGS)
@click.option('-s', '--slowdown-factor', default=1.0)
@click.option('-f', '--input-file', type=click.Path())
@click.option('--producer-host', default="localhost")
@click.option('--consumer1-host', default="localhost")
@click.option('--consumer2-host', default="localhost")
@click.argument('json_dir', type=click.Path())
def cli(slowdown_factor, input_file, producer_host, consumer1_host, consumer2_host, json_dir):
    """
      JSON_DIR: Json file output folder
    """

    from .. import util

    ############################################################################
    #
    # Step 1: generate the dicts of modules for each app
    
    console.log("Loading tpset_producer config generator")
    from . import tpset_producer_gen
    console.log("Loading tpset_consumer config generator")
    from . import tpset_consumer_gen
    console.log(f"Generating configs")

    modules_producer = tpset_producer_gen.generate(
        INPUT_FILE=input_file,
        SLOWDOWN_FACTOR=slowdown_factor,
    )

    modules_consumer1 = tpset_consumer_gen.generate()

    modules_consumer2 = tpset_consumer_gen.generate()

    ############################################################################
    #
    # Step 2: Create dict of apps. App consists of
    # list-of-connected-modules and a host to run on

    apps = {"tpset_producer": util.app(modulegraph=modules_producer,
                                       host=producer_host),
            "tpset_consumer1": util.app(modulegraph=modules_consumer1,
                                        host=consumer1_host),
            "tpset_consumer2": util.app(modulegraph=modules_consumer2,
                                        host=consumer2_host),
            }

    ############################################################################
    #
    # Step 3: Connect outputs of one app to inputs of another. Here we
    # connect the output `tpset_sink` in the `tpm` module of the
    # `tpset_producer` app to `tpset_source` in the `tps_sink` module
    # of each of the `tpset_consumer` apps, via a pub/sub connection

    app_connections = {
        "tpset_producer.tpsets_out": util.publisher(msg_type="dunedaq::trigger::TPSet",
                                                msg_module_name="TPSetNQ",
                                                subscribers=["tpset_consumer1.tpsets_in",
                                                             "tpset_consumer2.tpsets_in"])
    }

    ############################################################################
    #
    # Step 4: Create the json files for nanorc from the lists of
    # modules and the connections between modules. The function
    # assigns ports and creates the necessary N2Q/Q2N pairs for the
    # connections between apps
    util.make_apps_json(apps, app_connections, json_dir)

    console.log(f"config generated in {json_dir}")


if __name__ == '__main__':

    try:
        cli(show_default=True, standalone_mode=True)
    except Exception as e:
        console.print_exception()
