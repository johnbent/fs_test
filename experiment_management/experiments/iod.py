import os,sys,time,getpass
sys.path += [ './lib', '../lib' ]
import fs_test,expr_mgmt

mpi_program = "/scratch/iod/fs_test/fs_test/fs_test.x" 
mpi_options = {
  #"np" : [ 1,2,3,4,5,6,7,8,9,10,11,12,13,14,16,18,20,22,24,26,28,42,56,70,84 ],
  "np" : [ 56 ],
  "hosts" : [ os.getenv("EFF_MPI_IONS") ]
}

program_options = {
  "size"     : [ 1024, 2048, 4096, 8192, "16K", "32K", 65536, "128K", "256K", "512K", "1M", "2M", "4M", "8M"],
  #"nobj"    : [ 4096 ],
  "time"  : [ 10 ],
  "shift"    : [ '' ],
  "deletefile" : [ '' ],
  #"experiment" : [ 'iod.' + str(int(time.time())) ],
  "experiment" : [ 'iod2' ],
  "barriers"   : [ 'aopen,bclose' ],
  "noextra" : [ '' ],
}

def get_commands( expr_mgmt_options ):
  global mpi_options, mpi_program, program_options 
  commands = []

  # plfs commands
  program_options["target"] = [ 'plfs:/tmp/iod_plfs/%s.%%s' % getpass.getuser() ]
  program_options["io"] = [ 'plfs' ]
  commands = fs_test.get_commands( n1_segmented=False, n1_strided=True, nn=True, 
        mpi_options=mpi_options, mpi_program=mpi_program, 
        program_options=program_options, expr_mgmt_options=expr_mgmt_options )

  # lustre commands
  program_options["target"] = [ '/mnt/lustre1/%s.%%s' % getpass.getuser() ]
  program_options["io"] = [ 'posix' ]
  commands += fs_test.get_commands( n1_segmented=False, n1_strided=True, nn=True, 
        mpi_options=mpi_options, mpi_program=mpi_program, 
        program_options=program_options, expr_mgmt_options=expr_mgmt_options )

  # iod commands
  program_options["target"] = [ '%s.%%s' % getpass.getuser() ]
  program_options["io"] = [ 'iod' ]
  del program_options["deletefile"]
  commands += fs_test.get_commands( n1_segmented=False, n1_strided=True, nn=False, 
        mpi_options=mpi_options, mpi_program=mpi_program, 
        program_options=program_options, expr_mgmt_options=expr_mgmt_options )

  return commands
