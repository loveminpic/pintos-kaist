make clean
make 
cd build
# make tests/userprog/args-none.result
# make tests/userprog/args-single.result
# make tests/userprog/args-multiple.result
# make tests/userprog/args-many.result
# make tests/userprog/args-dbl-space.result
# pintos --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
# pintos --fs-disk=10 -p tests/userprog/no-vm/multi-oom -- -q -f run 'multi-oom'
make tests/userprog/no-vm/multi-oom.output
make tests/userprog/no-vm/multi-oom.result
cd ..
