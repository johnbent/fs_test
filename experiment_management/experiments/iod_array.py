import os,sys,time,getpass
sys.path += [ './lib', '../lib' ]
import fs_test,expr_mgmt

mpi_program = ("/scratch/iod/code/fs_test/fs_test/fs_test.%s.x" 
			% os.getenv( "MY_MPI_HOST" ))
randomize="randomize_config.pl" 
randomize_args="/etc/iodrc iod_max_persist_memory_mbs=1,2,4,8,16,32,64,128,256,512,1024 iod_shards_per_target=1,2,4"
mpirun="purgeall.sh ; %s %s ; mpirun" % ( randomize, randomize_args ) 
mpirun="purgeall.sh ; mpirun" # take out the randomize for now

mpi_options = {
  #"np" : [ 1,2,3,4,5,6,7,8,9,10,11,12,13,14,16,18,20,22,24,26,28,42,56,70,84 ],
  #"hostfile" : [ "/scratch/iod/tests/machinefile" ],
  "np" : [ 8 ],
  "hosts" : [ os.getenv("EFF_MPI_IONS") ]
}

program_options = {
  #"size"     : [ 1024, 2048, 4096, 8192, "16K", "32K", 65536, "128K", "256K", "512K", "1M", "2M", "4M", "8M"],
  #"size" : [ "1M" ],
  #"nobj"    : [ 6144 ],
  #"time"  : [ 10 ],
  "check" : [ 2 ],
  "iodcksum" : [ 0, 1 ],
  "iodtype" : [ 'blob', 'array' ],
  "touch" : [ 2 ],
  "shift"    : [ '' ],
  "experiment" : [ 'iod_types.' + str(int(time.time())) ],
  "barriers"   : [ 'aopen,bclose,aclose' ],
  #"noextra" : [ '' ],
  "target" : [ '%s.%%s' % getpass.getuser() ],
  "io" : [ 'iod' ],
  "flatten" : [ 1 ]
}

def get_commands( expr_mgmt_options ):
  global mpi_options, mpi_program, program_options 
  commands = []

  # iod commands, do 4GB each rank
  mbs_per_rank = 4 * 1024
  for i in range( 0, 11 ):
    mbs_per_io = 2**i 
    ios_per_rank = mbs_per_rank / mbs_per_io
    program_options["size"] = [ "%dM" % mbs_per_io ]
    program_options["nobj"] = [ ios_per_rank ] 
    icomms = fs_test.get_commands( n1_segmented=True, n1_strided=False, nn=False, 
        mpi_options=mpi_options, mpi_program=mpi_program, mpirun=mpirun,
        program_options=program_options, expr_mgmt_options=expr_mgmt_options )
    commands += icomms

  return commands
