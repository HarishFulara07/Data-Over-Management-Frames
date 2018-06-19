import os
import subprocess

__desc__ = \
	"""
	A handy script to make NDK standalone toolchains.
	Three arguments possible: arch, api and stl
	Any unspecified argument is iterated over all possible values
	"""

import argparse

_ARCH = [
	"arm",
	"arm64",
	"x86",
	"x86_64"
]

_API = [
	14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25
]

_STL = [
	"gnustl",
	"libc++",
	"stlport"
]


def argument_parser():
	_parser = argparse.ArgumentParser(__desc__)
	_parser.add_argument(
		'--ndk-home',
		dest = 'ndk_home',
		action = 'store',
		type = str,
		required = True,
		help = "the path to ndk installation dir"
	)
	_parser.add_argument(
		'--dir',
		dest = 'output_dir',
		action = 'store',
		type = str,
		required = True,
		help = "the output directory"
	)
	_parser.add_argument(
		'--api',
		dest = 'api',
		action = 'store',
		type = int,
		help = "the api level to build for"
	)
	_parser.add_argument(
		'--stl',
		dest = 'stl',
		action = 'store',
		type = str,
		help = "the standard template library to use"
	)
	_parser.add_argument(
		'--arch',
		dest = 'arch',
		action = 'store',
		type = str,
		help = "the architecture to build for"
	)
	return _parser


def make(ndk_home, output_dir, _api_list, _stl_list, _arch_list):
	for _api in _api_list:
		_api_dir = os.path.join(output_dir, "android-{:d}".format(_api))
		os.mkdir(_api_dir)

		for _stl in _stl_list:
			_stl_dir = os.path.join(_api_dir, "{:s}".format(_stl))
			os.mkdir(_stl_dir)

			for _arch in _arch_list:
				_arch_dir = os.path.join(_stl_dir, "{:s}".format(_arch))
				os.mkdir(_arch_dir)

				command = "python {:s}/build/tools/make-standalone-toolchains.py --api {:d} --stl {:s} --arch {:s} " \
				          "--install-dir {:s}".format(str(ndk_home), _api, _stl, _arch, str(_arch_dir))

				print("Running `{:s}`".format(command))
				rc = subprocess.call(
					args = command,
					stdin = subprocess.PIPE,
					stderr = subprocess.PIPE,
					shell = True
				)
				print("Return-Code: {:d}".format(rc))
				print()

				if rc != 0:
					os.rmdir(_arch_dir)

	pass


if __name__ == '__main__':
	parser = argument_parser()
	arguments = parser.parse_args()

	# Sanitize
	# ndk-home
	arguments.ndk_home = os.path.abspath(arguments.ndk_home)
	# output-dir
	arguments.output_dir = os.path.abspath(arguments.output_dir)
	# api
	api_list = list()
	if arguments.api is not None:
		api_list.append(arguments.api)
	else:
		api_list.extend(_API)
	# stl
	stl_list = list()
	if arguments.stl is not None:
		stl_list.append(arguments.stl)
	else:
		stl_list.extend(_STL)
	# stl
	arch_list = list()
	if arguments.arch is not None:
		arch_list.append(arguments.arch)
	else:
		arch_list.extend(_ARCH)

	make(arguments.ndk_home, arguments.output_dir, api_list, stl_list, arch_list)
	pass
