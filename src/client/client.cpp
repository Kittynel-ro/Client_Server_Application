#include "client/connection_manager.h"

int main(int argc, char* argv[])
{

	char *username, *password;

	try
	{

		if(argc != 3)
		{
			std::cerr << "Usage: client <username> <password>" << "\n";
			return 1;
		}

		username = argv[1];
		password = argv[2];

		boost::asio::io_context io_context;
		auto client_connection = std::make_shared<connection_manager>(io_context, username, password);
		client_connection->start();
		io_context.run();
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}