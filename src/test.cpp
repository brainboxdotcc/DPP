#include <dpp.h>
#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	WSClient client(0, 1, configdocument["token"], "gateway.discord.gg");
	client.ReadLoop();
	client.close();
	return 0;
}

