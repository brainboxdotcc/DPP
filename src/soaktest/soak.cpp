#include <dpp/dpp.h>

int main() {
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	if (t) {
		dpp::cluster soak_test(t);
		soak_test.on_log(dpp::utility::cout_logger());
		soak_test.start(dpp::st_wait);
	}
}
