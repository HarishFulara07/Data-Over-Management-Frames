import os

__desc__ = \
	"""
	A handy script to make NDK standalone toolchains for a given api level and stl
		- for all abi
	"""

import argparse

_ARCH = [
	"arm",
	"arm64",
	"x86",
	"x86_64"
]


def argument_parser():
	_parser = argparse.ArgumentParser(__desc__)
	_parser.add_argument(
		'--dir',
		dest = 'output_dir',
		action = 'store',
		required = True,
		help = "the output directory"
	)
	_parser.add_argument(
		'--api',
		dest = 'api',
		action = 'store',
		type = int,
		required = True,
		help = "the api level"
	)
	return _parser


def make(args):
	output_dir = os.path.abspath(args.output_dir)
	if os.path.exists(output_dir):
		print("Output directory already exists!")
		exit(1)
	else:
		os.mkdir(output_dir)
		pass

	# make
	pass


if __name__ == '__main__':
	parser = argument_parser()
	arguments = parser.parse_args()
	make(arguments)
	pass
