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

    apps = {"tpset_producer": util.app(modules=modules_producer,
                                       host=producer_host),
            "tpset_consumer1": util.app(modules=modules_consumer1,
                                        host=consumer1_host),
            "tpset_consumer2": util.app(modules=modules_consumer2,
                                        host=consumer2_host),
            }

    app_connections = {
        "tpset_producer.tpm.tpset_sink": util.publisher(msg_type="dunedaq::trigger::TPSet",
                                                        msg_module_name="TPSetNQ",
                                                        subscribers=["tpset_consumer1.tps_sink.tpset_source",
                                                                     "tpset_consumer2.tps_sink.tpset_source"])
    }

    util.make_apps_json(apps, app_connections, json_dir)

    console.log(f"config generated in {json_dir}")


if __name__ == '__main__':

    try:
        cli(show_default=True, standalone_mode=True)
    except Exception as e:
        console.print_exception()
