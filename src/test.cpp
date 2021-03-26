#include <dpp/dpp.h>
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
	DiscordClient client(0, 1, configdocument["token"]);
	client.ReadLoop();
	client.close();
	return 0;
}

