from rich.console import Console

# Add -h as default help option
CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

console = Console()

import click

@click.command(context_settings=CONTEXT_SETTINGS)
@click.option('-s', '--slowdown-factor', default=1.0)
@click.option('-f', '--input-file', type=click.Path(), multiple=True)
@click.argument('json_dir', type=click.Path())
def cli(slowdown_factor, input_file, json_dir):
    """
      JSON_DIR: Json file output folder
    """

    from .. import util
    
    console.log("Loading faketp config generator")
    from . import faketp_to_sink
    console.log(f"Generating configs")

    modules_faketp = faketp_to_sink.generate(
        INPUT_FILES = input_file,
        SLOWDOWN_FACTOR = slowdown_factor,
    )

    apps={ "faketp" : util.app(modules=modules_faketp,
                               host="localhost")
          }

    util.make_apps_json(apps, {}, json_dir)

if __name__ == '__main__':

    try:
            cli(show_default=True, standalone_mode=True)
    except Exception as e:
            console.print_exception()
