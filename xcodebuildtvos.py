#!/usr/bin/env python

import logging
import os
import re
import shutil
import subprocess
import tempfile

"""Python wrapper around the xcodebuild command line program."""

__author__ = "Martijn The"
__email__ = "martijn@getpebble.com"

DEFAULT_SDK_VERSION = "7.0"


class PebbleXcodeBuildException (Exception):
    pass


class XcodeBuild(object):
    sdk_version = ""
    conf = None
    archs = None
    project = None
    scheme = None
    derived_data_path = None
    is_built = False

    def __init__(self, project, derived_data_path=None):
        self.project = project
        self.derived_data_path = derived_data_path or tempfile.mkdtemp()
        self.build_settings = {}

    def _pre_build_sanity_check(self):
        if self.scheme is None:
            raise PebbleXcodeBuildException("No scheme set!")
        if self.is_built:
            raise PebbleXcodeBuildException("Already built!")

    def _get_sdk_string(self):
        is_device = len([arch for arch in self.archs
                         if arch.startswith("arm")]) > 0
        is_simulator = len([arch for arch in self.archs
                            if arch.startswith("i386") or
                            arch.startswith("x86_64")]) > 0
        if is_device and is_simulator:
            raise PebbleXcodeBuildException("Can't build for Device and"
                                            "Simulator in one go! (archs=%s)" %
                                            self.archs)
        platform = ("appletvos" if is_device else "appletvsimulator")
        return platform + self.sdk_version

    def _get_params(self):
        params = []
        if self.project:
            params.extend(("-project", self.project))
        if self.scheme:
            params.extend(("-scheme", self.scheme))
            params.extend(("-derivedDataPath", self.derived_data_path))
        if self.conf:
            params.extend(("-configuration", self.conf))
        if self.archs:
            concat_archs = " ".join(self.archs)
            params.append("ARCHS=%s" % concat_archs)
            # Auto-select SDK if archs is set:
            sdk = self._get_sdk_string()
            params.extend(("-sdk", sdk))
        
        is_device = len([arch for arch in self.archs
                         if arch.startswith("arm")]) > 0
        if is_device:
            params.append("OTHER_CFLAGS=-Qunused-arguments -fembed-bitcode")
            
        return params

    def _xcodebuild(self, *actions):
        self._pre_build_sanity_check()
        params = self._get_params()
        base_cmd = ["xcodebuild"] + params
        actions_cmd = base_cmd + list(actions)
        logging.debug("Executing: %s" % " ".join(actions_cmd))
        if subprocess.call(actions_cmd):
            raise PebbleXcodeBuildException("Build failed. xcodebuild exited"
                                            "with non-zero return code (%s)" %
                                            self.project)
        # Collect the build settings that were used:
        process = subprocess.Popen(base_cmd + ["-showBuildSettings"],
                                   stdout=subprocess.PIPE)
        if process.returncode:
            raise PebbleXcodeBuildException("Gettings settings failed."
                                            " xcodebuild exited with non-zero"
                                            " return code (%s)" % self.project)
        out = process.stdout.read()
        settings_list = re.findall('\s*([A-Z_]+)\s*=\s*(.*)',
                                   out, re.MULTILINE)
        self.build_settings = {pair[0]: pair[1] for pair in settings_list}

        self.is_built = True

    def build(self):
        self._xcodebuild("build")

    def _check_is_built(self):
        if not self.is_built:
            raise PebbleXcodeBuildException("Cannot be accessed before build()"
                                            "has been finished.")

    def _path_from_build_settings_components(self, *setting_names):
        self._check_is_built()
        components = [self.build_settings[n] for n in setting_names]
        # We want to treat absolute components as relative paths
        concat_path = os.sep.join(components)
        return os.path.normpath(concat_path)

    def built_product_path(self):
        return self._path_from_build_settings_components("BUILT_PRODUCTS_DIR",
                                                         "FULL_PRODUCT_NAME")

    def public_headers_path(self):
        args = ("BUILT_PRODUCTS_DIR", "PUBLIC_HEADERS_FOLDER_PATH")
        return self._path_from_build_settings_components(*args)


class FrameworkBuild(object):
    def __init__(self, project=None, workspace=None, scheme=None,
                 conf="Release", outdir=None, name=None,
                 derived_data_path=None):
        self.scheme = scheme
        self.name = name
        self.devicebuildarm64 = XcodeBuild(project, derived_data_path=None if derived_data_path is None else os.path.join(derived_data_path, "arm64"))
        self.simulatorbuild64 = XcodeBuild(project, derived_data_path=None if derived_data_path is None else os.path.join(derived_data_path, "x86_64"))
        self.outdir = outdir
        for (bld, archs) in [self.devicebuildarm64, ["arm64"]], \
                            [self.simulatorbuild64, ["x86_64"]]:
            bld.archs = archs
            bld.scheme = scheme
            bld.conf = conf

    def build(self):
        name = self.name or self.scheme
        if " " in name:
            name = name.replace(" ", "-")

        # Run the builds of the libraries:
        self.devicebuildarm64.build()
        self.simulatorbuild64.build()

        # Create the framework directory structure:
        temp_dir = tempfile.mkdtemp()
        framework_name = name + ".framework"
        framework_dir = os.path.join(temp_dir, framework_name)
        versions_dir = os.path.join(framework_dir, "Versions")
        a_dir = os.path.join(versions_dir, "A")
        lib_path = os.path.join(a_dir, name)
        headers_dir = os.path.join(a_dir, "Headers")
        os.makedirs(headers_dir)
        os.symlink("A", os.path.join(versions_dir, "Current"))
        os.symlink(os.path.join("Versions", "Current", "Headers"),
                   os.path.join(framework_dir, "Headers"))
        os.symlink(os.path.join("Versions", "Current", name),
                   os.path.join(framework_dir, name))

        # Move public headers:
        for filename in os.listdir(self.devicebuildarm64.public_headers_path()):
            shutil.move(os.path.join(self.devicebuildarm64.public_headers_path(), filename), headers_dir)
        #shutil.move(self.devicebuildarm64.public_headers_path(), headers_dir)

        # Use lipo to create one fat static library:
        lipo_cmd = ["lipo", "-create",
                    self.devicebuildarm64.built_product_path(),
                    self.simulatorbuild64.built_product_path(),
                    "-output", lib_path]
        logging.debug("Executing: %s" % " ".join(lipo_cmd))
        if subprocess.call(lipo_cmd):
            raise PebbleXcodeBuildException("lipo failed")

        # Move to outdir:
        if self.outdir:
            if not os.path.exists(self.outdir):
                os.makedirs(self.outdir)
            self._built_product_path = os.path.join(self.outdir,
                                                    framework_name)
            self._public_headers_path = os.path.join(self.outdir,
                                                     framework_name,
                                                     "Versions",
                                                     "A", "Headers")
            if os.path.exists(self._built_product_path):
                shutil.rmtree(self._built_product_path)
            shutil.move(framework_dir, self._built_product_path)
        else:
            self._built_product_path = framework_dir
            self._public_headers_path = headers_dir

    def built_product_path(self):
        return self._built_product_path

    def public_headers_path(self):
        return self._public_headers_path
