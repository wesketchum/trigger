import click
from rich.console import Console

# Add -h as default help option
CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

console = Console()


@click.command(context_settings=CONTEXT_SETTINGS)
@click.option('-s', '--slowdown-factor', default=1.0)
@click.option('-f', '--input-file', type=click.Path(), multiple=True)
@click.option('-c', '--candidate-plugin', default='TriggerCandidateMakerPrescalePlugin')
@click.option('-C', '--candidate-config', default='dict(prescale=1000)')
@click.option('-a', '--activity-plugin', default='TriggerActivityMakerPrescalePlugin')
@click.option('-A', '--activity-config', default='dict(prescale=1000)')
@click.argument('json_dir', type=click.Path())
def cli(slowdown_factor, input_file,
        candidate_plugin, candidate_config,
        activity_plugin, activity_config,
        json_dir):
    '''
      JSON_DIR: Json file output folder
    '''

    from .. import util

    console.log('Loading faketp chain config generator')
    from . import faketp_chain
    console.log(f'Generating configs')

    modules_faketp = faketp_chain.generate(
        INPUT_FILES=input_file,
        SLOWDOWN_FACTOR=slowdown_factor,
        CANDIDATE_PLUGIN=candidate_plugin,
        CANDIDATE_CONFIG=eval(candidate_config),
        ACTIVITY_PLUGIN=activity_plugin,
        ACTIVITY_CONFIG=eval(activity_config)
    )

    apps = {
        "faketp": util.app(modulegraph=modules_faketp,
                           host="localhost")
    }

    app_connections = {}

    util.make_apps_json(apps, app_connections, json_dir)


if __name__ == '__main__':

    try:
        cli(show_default=True, standalone_mode=True)
    except Exception as e:
        console.print_exception()
