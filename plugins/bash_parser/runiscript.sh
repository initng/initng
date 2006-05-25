#!/bin/bash
#
# Initng, a next generation sysvinit replacement.
# Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# Pars here are are ideas from gentoo runscript.sh

stop() {
	# Return success so the symlink gets removed
	return 0
}

start() {
	echo "ERROR: $SERVICE does not have a start function."
	return 1
}

daemon() {
	echo "ERROR: $SERVICE does not have a daemon function."
	return 1
}

setup() {
	echo "ERROR: $SERVICE are missing a setup function."
	return 1
}

reg_start() {
	iset $SERVICE exec start = "$SERVICE_FILE start"
	return 0
}

reg_stop() {
	iset $SERVICE exec stop = "$SERVICE_FILE stop"
	return 0
}

#echo "SERVICE_FILE: $SERVICE_FILE"
#echo "SERVICE: $SERVICE"
#echo "COMMAND: $COMMAND"

#echo "Sourcing $SERVICE_FILE"
source $SERVICE_FILE

#Make sure the ibins are in path
export PATH=/lib/ibin:$PATH

case "${COMMAND}" in
	stop)
			stop
			exit $?
		;;
	start)
			start
			exit $?
		;;
	daemon)
			daemon
			exit $?
		;;
	setup)
			setup
			exit $?
		;;
	*)
			echo "Bad Usage"
		;;
esac
