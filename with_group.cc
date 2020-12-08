#include <thread>
#include <tbb/task_group.h>
#include <tbb/global_control.h>
#include <atomic>
#include <iostream>
#include <functional>

namespace {
  void workInLane(std::atomic<int>& nEventsProcessed, unsigned int nChains, int nEvents, tbb::task_group& iGroup, int index) {
    int count = 0;
    if(index == 0) {
      count = ++nEventsProcessed;
      if(count >= nEvents) {
	return;
      }
    } 
    if(++index > nChains) {
      index = 0;
    }
    iGroup.run(
	       [&nEventsProcessed, nChains, nEvents, &iGroup,index](){
		 workInLane(nEventsProcessed, nChains, nEvents, iGroup, index);
	       });
  }
}


int main(int argc, char* argv[]) {
  if(argc != 4) {
    return 1;
  }

  //size_t parallelism = tbb::this_task_arena::max_concurrency();
  int parallelism = atoi(argv[1]);
  tbb::global_control c(tbb::global_control::max_allowed_parallelism, parallelism);

  unsigned int nLanes = parallelism;

  unsigned int nChains = atoi(argv[2]);

  auto nEvents = atoi(argv[3]);
  
  std::atomic<int> nEventsProcessed{0};

  tbb::task_group group;
  auto start = std::chrono::high_resolution_clock::now();
  for(int i=0; i< nLanes; ++i) {
    group.run(
	      [&nEventsProcessed, nEvents, nChains, &group]() {
		workInLane(nEventsProcessed, nChains, nEvents, group,0);
	      });
  }

  group.wait();
  std::chrono::microseconds eventTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-start);

  std::cout <<"group> nThreads: "<<parallelism<<" nLanes: "<<nLanes<<" nChains: "<<nChains<<" nEvents: "<<nEvents<<" nTried: "<<nEventsProcessed.load()<<" time: "<<eventTime.count()<<"us" << std::endl;
  return 0;
}
