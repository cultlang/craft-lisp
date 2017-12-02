
#include "lisp/common.h"
#include "lisp/lisp.h"

#include <stack>
#include <queue>

#include <spdlog/spdlog.h>
#include <bandit/bandit.h>



using namespace bandit;
using namespace craft;
using namespace craft::lisp;

// Tell bandit there are tests here.
go_bandit([](){
  describe("base:", [](){
    it("adds things", [&](){
        /*int four = craft::cmake::base::add(2, 2);
        AssertThat(four, Equals(4));*/
      });
  });
});

int main(int argc, char const *argv[]) {
	auto logger = spdlog::stdout_color_mt("console");
	return bandit::run(argc, (char**)argv);
}

