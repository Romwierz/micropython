#!/usr/bin/env python3
#
# Analyze certain acpects of ports to better know how are they implemented/done.
#
# Copyright (C) 2026 Michał Romsicki
#
# This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
# -----------------------------------------------------------------------
#
# The goal is to make implementing a new port easier. The analysis is performed
# on specified ports (their dirs) passed as positional arguments.
# On default the analyzed directories includes the directory corresponding
# to the port itself and $TOP/{py,shared,extmod}. Additional directories
# can be passed using '--add' option.
#
# The underlying tools are gtags for generating the symbol tags and global
# for displaying these tags, therefore they are required to use this script.
#
# Example:
#   ./tools/analyze-ports.py ports/alif ports/bare-arm --pattern 'mp_hal_delay_ms'
#
# Todo:
# - !!! better output formatting and options to rearrange it
# - if gtags/global has the option to add arbitrary tags, add option to use
#   additional macros (e.g. the ones used in build system files)
# - add option to show code block corresponding to definition/reference?
# - check tag file size and print warning about it (they can be very large).
# - if additional directories are specified, would there be a good way to check
#   if port's code actually uses this additional directory? it's possible that it
#   would be faster than generating tags for this directory (which would be an
#   exercise in futility)

from sys import version_info

if not (version_info.major >= 3 and version_info.minor >= 12):
    print('Python >=3.12 is required')
    print("You are using Python {}.{}.{}".format(version_info.major, version_info.minor, version_info.micro))
    exit(1)

from argparse import ArgumentParser
from sys import exit
from os import path, walk
from dataclasses import dataclass, field
from subprocess import run
from typing import override, cast

@dataclass
class Port:
    path: str
    name: str = ''
    targets_file: str = ''

    def __post_init__(self):
        self.name = path.basename(self.path)
        self.targets_file = root_dir + '/target-files-' + self.name

@dataclass
class Reference:
    line: str
    line_nr: int = 0
    file: str = ''
    usage: str = ''

    def __post_init__(self):
        l = self.line.split(maxsplit=3)
        if len(l) != 4:
            raise ValueError(f'Unexpected global output: {self.line!r}')
        self.line_nr = int(l[1])
        self.file = l[2]
        self.usage = l[3]

    @override
    def __str__(self) -> str:
        return f"{self.file}:{self.line_nr} '{self.usage}'"

@dataclass
class Definition:
    line: str
    line_nr: int = 0
    file: str = ''
    signature: str = ''

    def __post_init__(self):
        l = self.line.split(maxsplit=3)
        self.line_nr = int(l[1])
        self.file = l[2]
        self.signature = l[3]

    @override
    def __str__(self) -> str:
        return f"{self.file}:{self.line_nr} '{self.signature}'"

@dataclass
class Symbol:
    # Symbol: function or macro
    name: str
    defined: bool
    definitions: list[Definition] = field(default_factory=list)
    references: list[Reference] = field(default_factory=list)

    # Symbol can have multiple definitions (e.g. static functions)
    def append_definition(self, line: str):
        self.definitions.append(Definition(line))

    @override
    def __str__(self) -> str:
        ret = ''
        for defn in self.definitions:
            ret = ret.join(f"{defn.line_nr}:{defn.file} '{defn.signature}'")
        return ret

@dataclass
class PortAnalyzer:
    port: Port
    pattern: str
    symbols: dict[str, Symbol] = field(default_factory=dict)

def print_green(s: str):
    print("\033[92m {}\033[00m".format(s)) # ]] workaround for indentation issue in (n)vim

def print_yellow(s: str):
    print("\033[93m {}\033[00m".format(s)) # ]]

def dump_c_files(dir_path: str) -> list[str]:
    ret_files: list[str] = []
    for dirpath, _, files in walk(dir_path):
        for file in files:
            if file.endswith(('.c', '.h')):
                ret_files.append(path.join(dirpath, file))
    return ret_files

def generate_file_list(port: Port, additional_dirs: list[str] | None):
    print(f'\nGenerating target files list for {port.name}...', end='')
    dirs = ['py', 'shared', 'extmod', 'ports/' + port.name]
    for dir in additional_dirs or []:
        abs_path = path.abspath(dir)
        prefix = path.commonpath([root_dir, abs_path])
        dirs.append(abs_path.removeprefix(prefix + '/'))
    with open(port.targets_file, 'w') as f:
        for dir in dirs:
            for file in dump_c_files(path.join(root_dir, f'{dir}')):
                f.write(f"{file}\n")
    print(' Done.')

def generate_tags(port: Port):
    print(f'Generating tags for {port.name}...', end='')
    run(['gtags', '-C', root_dir, '-f', port.targets_file])
    print(' Done.')

def rm_tags_files(port: Port):
    print(f'Deleting tags...', end='')
    # 
    print(' Done.')

def analyze(port: Port, pattern: str):
    pa = PortAnalyzer(port=port, pattern=pattern)

    print(f'\nAnalyzing \'{pattern}\' for {port.name}...')

    # Search all definitions matching pattern
    print_green('  Definitions:')
    cp = run(['global', '-xid', pattern], capture_output=True, text=True)
    for line in cp.stdout.splitlines():
        s = Symbol(name=line.split(maxsplit=3)[0], defined=True)
        if s.name not in pa.symbols:
            pa.symbols[s.name] = s
        pa.symbols[s.name].append_definition(line)

    for sym in pa.symbols:
        if pa.symbols[sym].defined:
            print_yellow(pa.symbols[sym].name)
            for defn in pa.symbols[sym].definitions:
                print(defn)

    # For each symbol definition, search references
    print_green('  References:')
    for sym in pa.symbols:
        cp = run(['global', '-xir', pa.symbols[sym].name], capture_output=True, text=True)
        for line in cp.stdout.splitlines():
            r = Reference(line)
            pa.symbols[sym].references.append(r)
        print_yellow(pa.symbols[sym].name)
        for ref in pa.symbols[sym].references:
            print(ref)

    # For each symbol undefined, search references
    print_green('  References with no definition:')
    cp = run(['global', '-xis', pattern], capture_output=True, text=True)
    for line in cp.stdout.splitlines():
        s = Symbol(name=line.split(maxsplit=3)[0], defined=False)
        r = Reference(line)
        s.references.append(r)
        if not s.name in pa.symbols:
            pa.symbols[s.name] = s
        else:
            pa.symbols[s.name].references.append(r)

    for defn in pa.symbols:
        if not pa.symbols[defn].defined:
            print_yellow(pa.symbols[defn].name)
            for ref in pa.symbols[defn].references:
                print(ref)

    print('Done.')

# ================ The actual script ================
desc = 'Analyze certain aspects of specified ports.'
ports: list[Port] = []
root_dir = path.split(path.dirname(__file__))[0].removesuffix('/.')

parser = ArgumentParser(description=desc) 
parser.add_argument('port', nargs='+', help='port\'s directory') 
parser.add_argument('--pattern', required=True, help='pattern to search for in tags')
parser.add_argument('-v', '--verbose', action='store_true',
                      help='print verbose output (not implemented)') 
parser.add_argument('-s', '--case-sensitive', action='store_true',
                      help='search patterns case sensitively (not implemented)') 
parser.add_argument('-a', '--add', action='append', metavar='DIR',
                      help='''additional directory to be included in analysis;
                      Warning: external libraries can be huge, so the whole process can
                      take longer and produce much bigger tags file. Use with caution.''') 

args = parser.parse_args() 
ports_paths = cast(list[str], args.port)
additional_dirs = cast(list[str] | None, args.add)
pattern = cast(str, args.pattern)

for port_path in ports_paths:
    # Check validity:
    # if directory exists and if it's a port (check if there's mpconfigport.h inside)
    if not path.isfile(port_path + '/mpconfigport.h'):
        print(f'{port_path} is not a port\'s directory')
        continue
    
    p = Port(path = path.normpath(port_path))
    ports.append(p)

print(f'Analyzing {len(ports)} ports...')

for port in ports:
    generate_file_list(port, additional_dirs)
    generate_tags(port)
    analyze(port, pattern)

exit(0)
