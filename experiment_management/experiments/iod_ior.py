import sys,os,time
sys.path += [ './lib', '../lib' ]
import expr_mgmt

# just edit these parameters, the rest of the script
# adjusts accordingly to produce N-N, N-1 strided and N-1 segmented
data_per_proc = 2 * 1024 * 1024 * 1024
io_sizes = []
for pow in range(19,28):
 io_sizes.append(2**pow)

mpi_program = "/scratch/iod/bin/ior" 
mpirun="purgeall.sh ; /scratch/iod/scripts/ior_wrapper.sh mpirun" # take out the randomize for now

program_arguments = [
    [ "-np 8 -hosts %s" % os.getenv("EFF_MPI_IONS") ]
]

mpi_options = {
    'np' : [ 8 ],
    'hosts' : [ os.getenv("EFF_MPI_IONS") ],
}

program_options = {
  "i" : [ 1 ],
  "o" : [ "/mnt/daos/%s.ior.out.%d" % ( os.getenv('USER'), int(time.time()) )],
  "a" : [ 'DAOS', 'IOD' ],
  'L' : [ '' ],
  'O' : [ 'daosaios=128' ],
}

fname="%s.ior.out.%d" % ( os.getenv('USER'), int(time.time()) )
api_targets = {
  'DAOS'  : "/mnt/daos/%s" % fname,
  'IOD'   : "/mnt/daos/%s" % fname,
  'POSIX' : "/mnt/lustre12/%s/%s" % (os.getenv('USER'), fname),
  'MPIIO' : "/mnt/lustre12/%s/%s" % (os.getenv('USER'), fname),
}

#############################################################################
# typical use of this framework won't require modification beyond this point
#############################################################################

def get_commands( expr_mgmt_options ):

    # helper utility
  def make_commands():
    return expr_mgmt.get_commands(
      mpi_options=mpi_options,
      mpi_program=mpi_program,
      mpirun=mpirun,
      program_arguments=None,
      program_options=program_options,
      expr_mgmt_options=expr_mgmt_options )

  commands = []

  # N-1 strided
  for api in api_targets:
    program_options['a'] = [ api ]
    program_options['o'] = [ api_targets[api] ]
    for size in io_sizes:
      program_options['s'] = [ data_per_proc/size ]
      program_options['b'] = [ size ]
      program_options['t'] = [ size ]
      commands += make_commands()

  # N-1 segmented
  program_options['s'] = [ 1 ]
  program_options['b'] = [ data_per_proc ]
  program_options['t'] = io_sizes 
  #commands += make_commands() 

  # N-N 
  program_options['F'] = [ '' ]
  #commands += make_commands() 

  return commands
