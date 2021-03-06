#include <thread>
#include <tbb/task.h>
#include <tbb/global_control.h>
#include <atomic>
#include <iostream>
#include <functional>

template <typename F>
class FunctorTask : public tbb::task {
public:
  explicit FunctorTask(F f) : func_(std::move(f)) {}

  task* execute() override {
    func_();
    return nullptr;
  };

private:
  F func_;
};

template <typename ALLOC, typename F>
FunctorTask<F>* make_functor_task(ALLOC&& iAlloc, F f) {
  return new (iAlloc) FunctorTask<F>(std::move(f));
}

namespace {
  void workInLane(std::atomic<int>& nEventsProcessed, unsigned int nChains, int nEvents, tbb::task* iEndTask, int index) {
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
    tbb::task::spawn(* make_functor_task(tbb::task::allocate_additional_child_of(*iEndTask),
					 [&nEventsProcessed, nChains, nEvents, iEndTask,index](){
					   workInLane(nEventsProcessed, nChains, nEvents, iEndTask, index);
					 }));
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

  
  auto waitTask = new(tbb::task::allocate_root()) tbb::empty_task{};
  waitTask->set_ref_count(1+nLanes);

  std::atomic<int> nEventsProcessed{0};


  auto start = std::chrono::high_resolution_clock::now();
  for(int i=0; i< nLanes; ++i) {
    auto laneTask = make_functor_task(tbb::task::allocate_root(), [waitTask]() {
	waitTask->decrement_ref_count();
      });

    auto task = make_functor_task(tbb::task::allocate_additional_child_of(*laneTask),
				  [&nEventsProcessed, nEvents, nChains, laneTask]() {
				    workInLane(nEventsProcessed, nChains, nEvents, laneTask,0);
				  }
				  );
    tbb::task::spawn(*task);
  }

  waitTask->wait_for_all();
  std::chrono::microseconds eventTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-start);

  std::cout <<"task> nThreads: "<<parallelism<<" nLanes: "<<nLanes<<" nChains: "<<nChains<<" nEvents: "<<nEvents<<" nTried: "<<nEventsProcessed.load()<<" time: "<<eventTime.count()<<"us" << std::endl;

  tbb::task::destroy(*waitTask);

  return 0;
}
