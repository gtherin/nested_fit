#!/usr/bin/env python

# The new version uses rich to print out
from rich.live import Live as RLive
from rich.layout import Layout as RLayout
from rich.panel import Panel as RPanel
from rich.table import Table as RTable
# from rich.progress import Progress as RProgress
# from rich.progress import BarColumn as RBarColumn
# from rich.progress import TextColumn as RTextColumn
# from rich.columns import Columns as RColumns

# Custom rich widgets import
from .widgets import bar as cbar
from .widgets import timer as ctimer

# Metadata
from .metadata import __features__

# Rich debugging
# from rich import print as rprint

# Other imports
import pandas as pd
from importlib.metadata import version as imp_version
import subprocess
import pathlib
import logging
import yaml
import json
import time
import psutil


class NFDashboardHeader():
    def __init__(self):
        # Timer starts on header __init__
        # This is not of much importance. We only want a course way
        # to time the dashboard running time
        self._timer = ctimer.NFDashboardTimer()

        self._layout = RLayout()
        self._layout.split_row(RLayout(name='left'), RLayout(name='right'))

        # Make the Top left
        self._cpu_load_disp = cbar.HRollingBarDisplay(15, callback=psutil.cpu_percent)
        self._mem_load_disp = cbar.HRollingBarDisplay(15, callback=lambda: psutil.virtual_memory()[2])
        top_left = RLayout()
        top_left.split_row(RLayout(name='left'), RLayout(name='right'))

        version_time_grid = RTable.grid(expand=False)
        version_time_grid.add_column(justify='right')
        version_time_grid.add_column(justify='left')
        version_time_grid.add_row('[b]Version[/b]', ' ' + imp_version('nested_fit'))
        version_time_grid.add_row('[b]Elapsed[/b]', self._timer)
        top_left['left'].update(version_time_grid)

        cpu_mem_grid = RTable.grid(expand=False)
        cpu_mem_grid.add_column(justify='right')
        cpu_mem_grid.add_column(justify='right')
        cpu_mem_grid.add_row('[b]CPU[/b] ', self._cpu_load_disp)
        cpu_mem_grid.add_row('[b]MEM[/b] ', self._mem_load_disp)
        top_left['right'].update(cpu_mem_grid)

        self._layout['left'].update(top_left)

        # Make the top right
        top_right = RLayout()
        top_right.split_row(RLayout(name='left'), RLayout(name='center'), RLayout(name='right'))
        switches = RTable.grid(expand=False)
        switches.add_column(justify='right')
        switches.add_column(justify='left')
        switches.add_row(
            '[b]OpenMP[/b]',
            ' [green]YES[/green]' if __features__['OpenMP'] == 'ON' else ' [red]NO[/red]'
        )
        switches.add_row(
            '[b]OpenMPI[/b]',
            ' [green]YES[/green]' if __features__['OpenMPI'] == 'ON' else ' [red]NO[/red]'
        )

        top_right['left'].update(switches)

        debug_grid = RTable.grid(expand=False)
        debug_grid.add_column(justify='right')
        debug_grid.add_column(justify='left')

        if __features__['LTRACE'] == 'ON':
            debug_grid.add_row('', ' [b][yellow]:warning: Tracing on![/yellow][/b]')
        if __features__['BUILDTYPE'] == 'Debug':
            debug_grid.add_row('', ' [b][yellow]:warning: Debug build![/yellow][/b]')

        top_right['center'].update(debug_grid)

        debug_grid2 = RTable.grid(expand=False)
        debug_grid2.add_column(justify='right')
        debug_grid2.add_column(justify='left')

        if __features__['PPROF'] == 'ON':
            debug_grid2.add_row('[b][yellow]:warning: Profiling on![/yellow][/b]', '')
        else:
            debug_grid2.add_row('', '')
        debug_grid2.add_row('[green]✓ ALL OK[/green]', '')

        top_right['right'].update(debug_grid2)

        self._layout['right'].update(top_right)

    def __rich__(self):
        return self._layout

    def update(self):
        self._timer.update()
        self._cpu_load_disp.update()
        self._mem_load_disp.update()


class NFDashboardInput():
    def __init__(self):
        self._layout = RLayout()

        self._layout.update('Test Layout directly')

    def __rich__(self):
        return self._layout

    def update(self):
        pass


class Configurator():
    '''Writes the nf_input.yaml file for automatic runs and creates a python settings interface.
    '''

    def __init__(self,
                 datafiles=[],
                 specstr='x,c',
                 likelihood='GAUSSIAN',
                 expressions=[],
                 params={},

                 livepoints=200,
                 search_method='SLICE_SAMPLING',
                 search_params=(0.5, 3),
                 search_maxtries=1000,
                 search_multries=100,
                 search_maxsteps=100000,

                 conv_method='LIKE_ACC',
                 conv_accuracy=1.E-05,
                 conv_parameter=0.01,

                 cluster_enable=False,
                 cluster_method='f',
                 cluster_distance=0.5,
                 cluster_bandwidth=0.2,

                 keep_yaml=True
                 ):

        self.logger = logging.getLogger("rich")

        if not datafiles:
            self.logger.error('Configurator needs at least one datafile.')
            return None

        # Set some defaults
        self._config = {}

        # major.minor only
        self._config['version'] = float('.'.join(imp_version('nested_fit').split('.')[:2]))

        # datafiles
        self._config['datafiles'] = datafiles

        self._config['search'] = {}
        self._config['search']['livepoints'] = livepoints
        self._config['search']['method'] = search_method
        self._config['search']['param1'] = search_params[0]
        self._config['search']['param2'] = search_params[1]
        self._config['search']['max_tries'] = search_maxtries
        self._config['search']['tries_mult'] = search_multries
        self._config['search']['num_tries'] = 1
        self._config['search']['max_steps'] = search_maxsteps

        self._config['convergence'] = {}
        self._config['convergence']['method'] = conv_method
        self._config['convergence']['accuracy'] = conv_accuracy
        self._config['convergence']['parameter'] = conv_parameter

        self._config['clustering'] = {}
        self._config['clustering']['enabled'] = cluster_enable
        self._config['clustering']['method'] = cluster_method
        self._config['clustering']['distance'] = cluster_distance
        self._config['clustering']['bandwidth'] = cluster_bandwidth

        # input file layout
        self._config['specstr'] = specstr

        self._config['likelihood'] = 'GAUSSIAN'

        self._config['function'] = {}

        self.multiexp = len(expressions) > 1
        if expressions:
            if self.multiexp:
                for i, expr in enumerate(expressions):
                    self._config['function'][f'expression_{i + 1}'] = expr
            else:
                self._config['function']['expression'] = expressions[0]

        self._config['function']['params'] = params

        # Get all data by default
        self.manual_extents = False
        if self.multiexp:
            for i, exp in enumerate(expressions):
                self._config[f'data_{i + 1}'] = {'xmin': 0, 'xmax': 0, 'ymin': 0, 'ymax': 0}
        else:
            self._config['data'] = {'xmin': 0, 'xmax': 0, 'ymin': 0, 'ymax': 0}
            ext = pathlib.Path(self._config['datafiles'][0]).suffixes[-1]
            if ext == '.csv':
                delimiter = ','
            elif ext == '.tsv':
                delimiter = '\t'
            else:
                self.logger.error('Input file invalid format/extension.')
                self.logger.error('Valid formats: `.csv` and `.tsv`.')
                return None
            self._df = pd.read_csv(self._config['datafiles'][0], delimiter=delimiter, header=None)

        self._reconfigure_data_extents()

        self._keep_yaml = keep_yaml

    def set_expression(self, expr, slot=0):
        if slot == 0:
            self._config['function']['expression'] = expr
        else:
            self._config['function'][f'expression_{slot}'] = expr

    def set_extents(self, xmin, xmax, slot=0):
        if xmin == 0 and xmax == 0:
            self.manual_extents = False
            self._reconfigure_data_extents()
        else:
            self.manual_extents = True
            if slot == 0:
                self._config['data']['xmin'] = xmin
                self._config['data']['xmax'] = xmax
            else:
                self._config[f'data_{slot}']['xmin'] = xmin
                self._config[f'data_{slot}']['xmax'] = xmax

    def set_params(self, **params):
        self._config['function']['params'] = params

    def sample(self, path='.', silent_output=False):
        version = imp_version('nested_fit')

        self._write_yaml_file(path)

        self._nf_process = subprocess.Popen(
            [f'nested_fit{version}', '-lo', '-v', 'error'],
            stdout=subprocess.PIPE,
            cwd=pathlib.Path(path).resolve()
        )

        if not silent_output:
            self._live_dash = self._generate_live_dashboard()

        # pbar = tqdm(total=self._config['search']['max_steps'], desc='Running nested_fit', disable=disablebar)

        # TODO: (César): Make this a thread and send data via socket
        while self._nf_process.poll() is None:
            live_data = self._parse_nf_stdout()

            if not live_data:
                continue
            if not silent_output:
                self._draw_live_table(live_data)

        if not self._keep_yaml:
            pathlib.Path.unlink(f'{path}/nf_input.yaml', missing_ok=True)

        try:
            with open(f'{path}/nf_output_res.json', 'r') as f:
                return json.load(f)
        except IOError as e:
            self.logger.error('Could not load nested_fit\'s output result.')
            self.logger.error(f'I/O exception {e}')
            return None

    def dashboard(self):
        pass

    def _parse_stdout_error(self, line):
        # Handle errors
        if '<ERROR>' in line[0]:
            print(''.join(line), end='')
            return True
        if 'LO' not in line[0]:
            return True
        return False

    def _parse_nf_stdout(self):
        line = self._nf_process.stdout.readline().decode("utf-8").split('|')

        if self._parse_stdout_error(line):
            return None

    def _generate_live_dashboard(self):
        # Display some input options so we know
        self._live_dash = RLayout()

        self._live_dash.split_column(
            RLayout(name='header', size=4),
            RLayout(name='body')
        )

        self._live_dash['body'].split_row(
            RLayout(name='input_info'),
            RLayout(name='output_info')
        )

        header = NFDashboardHeader()
        header_panel = RPanel(header, title='Nested Fit Dashboard')
        input_info = NFDashboardInput()
        input_panel = RPanel(input_info, title='Input Info')
        self._live_dash['header'].update(header_panel)
        self._live_dash['body']['input_info'].update(input_panel)

        with RLive(self._live_dash, refresh_per_second=1 / 1.5):
            for _ in range(100):
                header.update()
                input_info.update()
                time.sleep(1.5)

    def _draw_live_table(self, data):
        pass

    def _write_yaml_file(self, path):
        # We want the datafiles as a string
        datafiles = self._config['datafiles']
        self._config['datafiles'] = ', '.join(datafiles)

        with open(f'{path}/nf_input.yaml', 'w') as f:
            data = yaml.dump(self._config, width=10000)
            f.write(data)

        self._config['datafiles'] = datafiles

    def _calculate_data_extents(self):
        # Get where the x's are column-wise
        x_col = self._config['specstr'].split(',').index('x')
        return (self._df[x_col].min().item(), self._df[x_col].max().item())

    def _get_data(self):
        if self.multiexp:
            self.logger.error('_get_data does not support multiple datafiles.')
            return None

        x_col = self._config['specstr'].split(',').index('x')
        y_col = self._config['specstr'].split(',').index('c')
        # e_col = self._config['specstr'].split(',').index('ce')

        return (self._df[x_col].tolist(), self._df[y_col].tolist())

    def _reconfigure_data_extents(self):
        if self.manual_extents or not self._config['datafiles']:
            return

        # Read the datafiles and set the extents
        if not self.multiexp:
            for file in self._config['datafiles']:
                xmin, xmax = self._calculate_data_extents()
                self._config['data']['xmin'] = xmin
                self._config['data']['xmax'] = xmax
        else:
            for i, file in enumerate(self._config['datafiles']):
                xmin, xmax = self._calculate_data_extents(file)
                self._config[f'data_{i + 1}']['xmin'] = xmin
                self._config[f'data_{i + 1}']['xmax'] = xmax
