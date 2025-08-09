#include "server/session_manager.h"

int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_context io_context;

		session_manager s(io_context);

		io_context.run();
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}