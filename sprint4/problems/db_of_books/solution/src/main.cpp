#include "request.h"

int main(int argc, const char* argv[]){ 
    using namespace request_handler;

    if (argc == 1) {
            std::cout << "Usage: book_manager <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }

    RequestManager manager(argv[1]);

    while(true){
        std::string query;
        std::getline(std::cin, query);

        if(manager.ManageQuery(query) == ResultCode::EXIT){
            break;
        }
    }
}