#!/bin/bash

###### SETUP STUFF - NO NEED TO EDIT ######
# STACK is top directory where stack is installed
STACK="/usr/local/ff/stack"

# EXAMPLES is path to where vpicio_uni and ff_server are installed
EXAMPLES="${STACK}/bin"

#
# IONs and CNs: Defined here rather than using /scratch/iod version so more control
# xtprocadmin (used to find operational CNs) only runs on buffy 
#
EFF_MPI_IONS="ion01,ion02,ion03,ion04,ion05,ion06,ion07,ion08,ion09,ion10,ion11,ion12,ion13,ion14,"
EFF_MPI_CNS="`ssh buffy xtprocadmin | grep compute | grep up | awk '{printf "%s,", $3;}'`"
EFF_IONS=`echo $EFF_MPI_IONS | sed 's/,/ /g'`
EFF_CNS=`echo $EFF_MPI_CNS | sed 's/,/ /g'`

# Functions
message () {
	echo "#### $1"
}

# What we're going to run
SERVER=$EXAMPLES/h5ff_server
DEMO=$EXAMPLES/vpicio_uni

############ END OF FIXED SETUP

############ CONTROLS FOR THE RUN - MAY EDIT
num_ions=8
compute_ranks=64
DEMO_OPTS="-p 1 -s 6 -r 1 -f $USER.eff_vpic.h5"

########## PROBABLY NO EDITS BELOW HERE
sleep_time=2
slog="ff_output/server_vpicio"
clog="ff_output/client_vpicio"

vcommand="mpiexec -np $compute_ranks -hosts $EFF_MPI_CNS $DEMO $DEMO_OPTS"
scommand="mpiexec -np $num_ions -hosts $EFF_MPI_IONS $SERVER"

# do stuff!
if [ ! -d "ff_output" ]; then
    mkdir ff_output
fi

killall mpiexec

message "Launching servers $scommand"
$scommand 2> $slog & srv_pid=$!
message "Srv pid: $srv_pid.  Sleeping for $sleep_time before launching client"
sleep $sleep_time

message "Launching Clients $vcommand"
$vcommand > $clog
message "If errors were seen, consult $slog and $clog"
