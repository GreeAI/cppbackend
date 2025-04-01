#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <optional>
#include <thread>

#include "boost_logger.h"
#include "http_server.h"
#include "json_loader.h"
#include "request_handler.h"
#include "request_struct.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

    [[nodiscard]] std::optional<strct::Args>
    ParseCommandLine(int argc, const char *const argv[])
    {
        using namespace std;
        namespace po = boost::program_options;

        po::options_description desc{"Allowed options"s};

        strct::Args args;
        double tick_per;
        std::string state_file;
        double save_tick_state;

        desc.add_options()("help,h", "produce help message")("tick-period,t", po::value(&tick_per)->value_name("milliseconds"s), "Set tick period")("config-file,c", po::value(&args.config)->value_name("file"s), "Set config file path")("www-root,r", po::value(&args.root)->value_name("dir"s), "Set static files root")("randomize-spawn-points", "Put the dog in a random position")("state-file", po::value(&state_file)->value_name("state-file"s), "set file path, which saves a game state in procces, and restore it at startup")("save-state-period", po::value(&save_tick_state)->value_name("milliseconds"s), "set period for automatic saving of game state.");

        // variables_map хранит значения опций после разбора
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.contains("help"s)) {
            // Если был указан параметр --help, то выводим справку и возвращаем nullopt
            std::cout << desc;
            return std::nullopt;
        }

        if (vm.contains("tick-period"s)) {
            args.tick = tick_per;
        } else {
            args.tick = std::nullopt;
        }

        if (vm.contains("state-file"s)) {
            args.state_file = state_file;
        }

        if (vm.contains("save-state-period"s)) {
            args.save_tick = save_tick_state;
        }

        if (!vm.contains("config-file"s)) {
            throw std::runtime_error("Config file have not been specified"s);
        }

        if (!vm.contains("www-root"s)) {
            throw std::runtime_error("Static files path is not specified"s);
        }
        if (vm.contains("randomize-spawn-points"s)) {
            args.random_spawn = true;
        }

        // С опциями программы всё в порядке, возвращаем структуру args
        return args;
    }

    // Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
    void RunWorkers(int n, const Fn &fn)
    {
        n = std::max(1, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--n) {
            workers.emplace_back(fn);
        }
        fn();
    }

} // namespace

int main(int argc, const char *argv[])
{
    logger::InitBoostLog();
    std::optional<strct::Args> args;
    try {
        args = ParseCommandLine(argc, argv);
        if (!args.has_value()) {
            logger::LogServerExit(0);
            return EXIT_SUCCESS;
        }
    } catch (const std::exception &ex) {
        logger::LogServerExit(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
    try {
        const static int NUM_THREADS = std::thread::hardware_concurrency();
        const char *DB_URL = std::getenv("GAME_DB_URL");
        if (!DB_URL) {
            throw std::runtime_error("GAME_DB_URL is not specified");
        }
        db_connection::CreateTable(DB_URL);
        auto db_manager = std::make_unique<db_connection::DatabaseManager>(NUM_THREADS, DB_URL);

        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame((*args).config);
        // 2. Инициализируем io_context
        net::io_context ioc(NUM_THREADS);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait(
            [&ioc](const sys::error_code &ec, [[maybe_unused]] int signal_number) {
                if (!ec) {
                    ioc.stop();
                }
            });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры и
        // файловым каталогом static
        std::shared_ptr<http_handler::RequestHandler> handler =
            std::make_shared<http_handler::RequestHandler>(game, (*args),
                                                           net::make_strand(ioc), std::move(db_manager));

        handler->LoadState();

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port},
                               [&handler](auto &&req, auto &&send) {
                                   (*handler)(std::forward<decltype(req)>(req),
                                              std::forward<decltype(send)>(send));
                               });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов
        // обрабатывать запросы
        logger::LogServerStart(port, address.to_string());
        std::cout << "Server has started..."sv << std::endl;

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1, NUM_THREADS), [&ioc] { ioc.run(); });

        handler->SaveState();

    } catch (const std::exception &ex) {
        logger::LogServerExit(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
    logger::LogServerExit(0);
}