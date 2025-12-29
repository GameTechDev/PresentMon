#include <iostream>
#include "../Interprocess/source/ExperimentalInterprocess.h"
#include "../Interprocess/source/Interprocess.h"
#include "Options.h"
#include <thread>
#include "../PresentMonMiddleware/MockCommon.h"

int main(int argc, char** argv)
{
	using namespace pmon::ipc;
	using namespace pmon::ipc::mock;

	// parse command line options
	if (auto ecode = opt::Options::Init(argc, argv)) {
		return *ecode;
	}

	// shortcut for command line options
	try
	{
		const auto& opts = opt::Options::Get();

		if (opts.testF) {
			std::cout << experimental::f() << std::endl;
		}
		else if (opts.basicMessage) {
			std::string buffer;
			std::cin >> buffer;

			auto pServer = experimental::IServer::Make(buffer);
			pServer->MakeUptrToMessage(buffer);

			// send goahead signal to client
			std::cout << "go" << std::endl;

			// wait until client has finished receiving
			std::cin >> buffer;
		}
		else if (opts.sharedRootBasic) {
			std::string buffer;
			std::cin >> buffer;

			auto pServer = experimental::IServer::Make(buffer);
			pServer->MakeUptrToMessage(buffer);

			// send result of allocator aware test to client
			std::cout << pServer->RoundtripRootInShared() << std::endl;
			// wait until client has finished receiving
			std::cin >> buffer;
		}
		else if (opts.destroyUptr) {
			std::string buffer;

			// waiting for client that contains input code
			std::cin >> buffer;

			auto pServer = experimental::IServer::Make(buffer);
			pServer->MakeUptrToMessage(buffer);

			// send goahead signal to client, checks the free memory
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;

			pServer->FreeUptrToMessage();

			// send goahead signal to client to check free memory again
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;
		}
		else if (opts.makeDestroyUptr) {
			std::string buffer;

			auto pServer = experimental::IServer::Make("dummy");

			// signal to client that shm has been created
			std::cout << "go" << std::endl;

			// wait for client signal that free memory has been checked, contains code string
			std::cin >> buffer;

			// create the uptr and string in memory
			pServer->MakeUptrToMessage(buffer);

			// send goahead signal to client, checks the free memory
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;

			pServer->FreeUptrToMessage();

			// send goahead signal to client to check free memory again
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;
		}
		else if (opts.sharedRootRetained) {
			std::string buffer;

			auto pServer = experimental::IServer::Make("dummy");

			// signal to client that shm has been created
			std::cout << "go" << std::endl;

			// wait for client signal that free memory has been checked, contains code string
			std::cin >> buffer;

			// create the uptr and string in memory
			pServer->MakeRoot(std::stoi(buffer));

			// send goahead signal to client, checks the free memory
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;

			pServer->FreeRoot();

			// send goahead signal to client to check free memory again
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;
		}
		else if (opts.clientFree) {
			std::string buffer;

			auto pServer = experimental::IServer::Make("dummy");

			// signal to client that shm has been created
			std::cout << "go" << std::endl;

			// wait for client signal that free memory has been checked, contains code string
			std::cin >> buffer;

			// create object in shm
			pServer->CreateForClientFree(777, buffer);

			// send goahead signal to client, checks memory and values and frees
			std::cout << "go" << std::endl;

			// wait until client has finished work
			std::cin >> buffer;
		}
		else if (opts.deep) {
			std::string buffer;

			auto pServer = experimental::IServer::Make("dummy");

			// signal to client that shm has been created
			std::cout << "go" << std::endl;

			// wait for client signal that free memory has been checked, client sends 2 ints
			int n1, n2;
			std::cin >> n1;
			std::cin >> n2;

			// create object in shm
			pServer->MakeDeep(n1, n2);

			// send goahead signal to client, checks memory and values
			std::cout << "go" << std::endl;

			// wait for client ack
			std::cin >> buffer;

			// free data
			pServer->FreeDeep();

			// send goahead signal to client, checks memory
			std::cout << "go" << std::endl;

			// wait until client has finished work
			std::cin >> buffer;
		}
		else if (opts.cloneHeap) {
			std::string buffer;

			auto pServer = experimental::IServer::Make("dummy");

			// signal to client that shm has been created
			std::cout << "go" << std::endl;

			// wait for client signal that free memory has been checked, contains code string
			std::cin >> buffer;

			// create the uptr and string in memory
			pServer->MakeRootCloneHeap(std::stoi(buffer));

			// send goahead signal to client, checks the free memory
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;

			pServer->FreeRoot();

			// send goahead signal to client to check free memory again
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;
		}
		else if (opts.cloneHeapDeep) {
			std::string buffer;

			auto pServer = experimental::IServer::Make("dummy");

			// signal to client that shm has been created
			std::cout << "go" << std::endl;

			// wait for client signal that free memory has been checked, 2 numbers
			int n1, n2;
			std::cin >> n1;
			std::cin >> n2;

			// create the uptr and string in memory
			pServer->MakeDeepCloneHeap(n1, n2);

			// send goahead signal to client, checks the free memory, makes a clone
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory / cloning
			std::cin >> buffer;

			pServer->FreeDeep();

			// send goahead signal to client to check free memory again
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;
		}
		else if (opts.cloneHeapDeep2) {
			std::string buffer;

			auto pServer = experimental::IServer::Make("dummy");

			// signal to client that shm has been created
			std::cout << "go" << std::endl;

			// wait for client signal that free memory has been checked, 2 numbers
			int n1, n2;
			std::cin >> n1;
			std::cin >> n2;

			// create the uptr and string in memory
			pServer->MakeDeepCloneHeap2(n1, n2);

			// send goahead signal to client, checks the free memory, makes a clone
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory / cloning
			std::cin >> buffer;

			pServer->FreeDeep2();

			// send goahead signal to client to check free memory again
			std::cout << "go" << std::endl;

			// wait until client has finishing checking free memory
			std::cin >> buffer;
		}
		else if (opts.basicIntro) {
			std::string buffer;

			auto pServiceComms = pmon::ipc::MakeServiceComms(*opts.introNsm,
				1000,
				1500);
			pmon::ipc::intro::RegisterMockIntrospectionDevices(*pServiceComms);

			// signal to client that shm has been created
			std::cout << "ready" << std::endl;

			// wait for client signal that work is done
			std::cin >> buffer;
		}
		else {
			std::cout << "default-output" << std::endl;
		}
	}
	catch (const CLI::CallForHelp& e)
	{
		return e.get_exit_code();
	}
	catch (const boost::interprocess::interprocess_exception& e)
	{
		std::cout << "Interprocess Exception Occurred: " << e.what();
		return e.get_error_code();
	}
	catch (const std::ios_base::failure& e)
	{
		std::cout << "Input/Output library Exception: " << e.what();
		return -1;
	}
	catch (const std::bad_cast& e)
	{
		std::cout << "Invalid Dynamic Cast Exception: " << e.what();
		return -1;
	}
	catch (const CLI::BadNameString& e)
	{
		return e.get_exit_code();
	}

	return 0;
}
