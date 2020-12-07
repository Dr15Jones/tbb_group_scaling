#include <thread>
#include <tbb/task_group.h>
#include <tbb/global_control.h>
#include <atomic>
#include <iostream>
#include <functional>

namespace {
  void workInLane(std::atomic<int>& nEventsProcessed, int nEvents, tbb::task_group& iGroup, std::atomic<unsigned int>& iNGroupsDone, int index) {
    int count = 0;
    if(index == 0) {
      count = ++nEventsProcessed;
      if(count >= nEvents) {
	--iNGroupsDone;
	return;
      }
    } 
    if(++index > 4) {
      index = 0;
    }
    iGroup.run(
	       [&nEventsProcessed, nEvents, &iGroup,&iNGroupsDone, index](){
		 workInLane(nEventsProcessed, nEvents, iGroup, iNGroupsDone, index);
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

  unsigned int nLanes = atoi(argv[2]);

  auto nEvents = atoi(argv[3]);
  
  std::atomic<int> nEventsProcessed{0};

  std::atomic<unsigned int> nGroupsDone{nLanes};
  std::vector<tbb::task_group> groups(nLanes);
  auto start = std::chrono::high_resolution_clock::now();
  for(int i=0; i< nLanes; ++i) {
    auto& group = groups[i];
    group.run(
	      [&nEventsProcessed, nEvents, &group, &nGroupsDone]() {
		workInLane(nEventsProcessed, nEvents, group,nGroupsDone, 0);
	      });
  }

  do {
    for(auto& g: groups) {
      g.wait();
    }
  }while(nGroupsDone != 0);

  std::chrono::microseconds eventTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-start);

  std::cout <<"nThreads: "<<parallelism<<" nLanes: "<<nLanes<<" nEvents: "<<nEvents<<" time: "<<eventTime.count()<<"us" << std::endl;
  return 0;
}
