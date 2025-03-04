#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <optional>
#include <thread>

#include "boost_logger.h"
#include "json_loader.h"
#include "request_handler.h"
#include "request_struct.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace {

[[nodiscard]] std::optional<strct::Args>
ParseCommandLine(int argc, const char *const argv[]) {
  using namespace std;
  namespace po = boost::program_options;

  po::options_description desc{"Allowed options"s};

  strct::Args args;
  double tick_per;
  desc.add_options()("help,h", "produce help message")(
      "tick-period,t", po::value(&tick_per)->value_name("milliseconds"s),
      "Set tick period")("config-file,c",
                         po::value(&args.config)->value_name("file"s),
                         "Set config file path")(
      "www-root,r", po::value(&args.root)->value_name("dir"s),
      "Set static files root")("random", "Put the dog in a random position");

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
template <typename Fn> void RunWorkers(unsigned n, const Fn &fn) {
  n = std::max(1u, n);
  std::vector<std::jthread> workers;
  workers.reserve(n - 1);
  // Запускаем n-1 рабочих потоков, выполняющих функцию fn
  while (--n) {
    workers.emplace_back(fn);
  }
  fn();
}

} // namespace

int main(int argc, const char *argv[]) {
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
    // 1. Загружаем карту из файла и построить модель игры
    model::Game game = json_loader::LoadGame((*args).config);
    // 2. Инициализируем io_context
    const unsigned num_threads = std::thread::hardware_concurrency();
    net::io_context ioc(num_threads);

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
                                                       net::make_strand(ioc));

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
    RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });
  } catch (const std::exception &ex) {
    logger::LogServerExit(EXIT_FAILURE, ex.what());
    return EXIT_FAILURE;
  }
  logger::LogServerExit(0);
}