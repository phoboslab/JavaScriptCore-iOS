#!/usr/bin/env python

import argparse
import logging
import os
import tempfile
import shutil
import subprocess
from xcodebuild import FrameworkBuild

"""Script to build JavaScriptCore.framework."""

__author__ = "Martijn The"
__email__ = "martijn@getpebble.com"


class PebbleKitiOSException (Exception):
    pass


def build(out, derived_data_path):
    outdir = out if out else tempfile.mkdtemp()

    jsc = FrameworkBuild(workspace="JavaScriptCore-iOS.xcworkspace",
                         scheme="JavaScriptCore iOS",
                         name="JavaScriptCore",
                         conf="Production",
                         outdir=outdir,
                         derived_data_path=derived_data_path)
    jsc.build()

    # FIXME: wtf headers are copied... remove them:
    shutil.rmtree(os.path.join(outdir, 'JavaScriptCore.framework',
                               'Headers', 'wtf'))

    return outdir

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Build'
                                     ' JavaScriptCore.framework')
    parser.add_argument('--out', default=None, type=str,
                        help="Output directory. Defaults to a random' \
                        ' temporary folder.")
    parser.add_argument('--derived-data', default=None, type=str,
                        help="Derived data directory. Defaults to a random' \
                        ' temporary folder.")
    parser.add_argument('--verbose', action='store_const', default=False,
                        const=True, help="Enable verbose logging.")
    parser.add_argument('--no-open', action='store_const', default=True,
                        const=False, help="Use `--no-open` to skip' \
                        ' revealing the built product in Finder.")
    args = parser.parse_args()

    log_level = logging.INFO
    if args.verbose:
        log_level = logging.DEBUG
    logging.basicConfig(format='[%(levelname)-8s] %(message)s',
                        level=log_level)

    outdir = build(args.out, args.derived_data)

    if args.no_open:
        os.system("open %s" % outdir)
