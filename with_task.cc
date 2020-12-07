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
  void workInLane(std::atomic<int>& nEventsProcessed, int nEvents, tbb::task* iEndTask, int index) {
    int count = 0;
    if(index == 0) {
      count = ++nEventsProcessed;
      if(count >= nEvents) {
	iEndTask->decrement_ref_count();
	return;
      }
    } 
    if(++index > 4) {
      index = 0;
    }
    tbb::task::spawn(* make_functor_task(tbb::task::allocate_root(),
					 [&nEventsProcessed, nEvents, iEndTask,index](){
					   workInLane(nEventsProcessed, nEvents, iEndTask, index);
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

  unsigned int nLanes = atoi(argv[2]);

  auto nEvents = atoi(argv[3]);

  
  auto waitTask = new(tbb::task::allocate_root()) tbb::empty_task{};
  waitTask->set_ref_count(1+nLanes);

  std::atomic<int> nEventsProcessed{0};


  auto start = std::chrono::high_resolution_clock::now();
  for(int i=0; i< nLanes; ++i) {
    auto task = make_functor_task(tbb::task::allocate_root(),
				  [&nEventsProcessed, nEvents, waitTask]() {
				    workInLane(nEventsProcessed, nEvents, waitTask,0);
				  }
				  );
    tbb::task::spawn(*task);
  }

  waitTask->wait_for_all();
  std::chrono::microseconds eventTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-start);

  std::cout <<"nThreads: "<<parallelism<<" nLanes: "<<nLanes<<" nEvents: "<<nEvents<<" time: "<<eventTime.count()<<"us" << std::endl;

  tbb::task::destroy(*waitTask);

  return 0;
}
