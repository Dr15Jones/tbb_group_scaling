#! /bin/bash
export TBB_INCLUDE=/cvmfs/cms.cern.ch/slc7_amd64_gcc820/external/tbb/2020_U2-ghbfee/include
export TBB_DIR=/cvmfs/cms.cern.ch/slc7_amd64_gcc820/external/tbb/2020_U2-ghbfee/lib

g++ -std=c++17 -I${TBB_INCLUDE} -L${TBB_DIR} -ltbb -pthread -o with_task with_task.cc
g++ -std=c++17 -I${TBB_INCLUDE} -L${TBB_DIR} -ltbb -pthread -o with_group with_group.cc
g++ -std=c++17 -I${TBB_INCLUDE} -L${TBB_DIR} -ltbb -pthread -o with_multiple_groups with_multiple_groups.cc

