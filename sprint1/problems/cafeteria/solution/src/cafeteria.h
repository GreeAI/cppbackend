#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <memory>
#include <chrono>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

using namespace std::chrono;
using namespace std::literals;

using Timer = boost::asio::steady_timer;

using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Logger {
   public:
    explicit Logger(std::string id) : id_(std::move(id)) {}
 
    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv
           << duration<double>(steady_clock::now() - start_time_).count()
           << "s] "sv << message << std::endl;
    }
 
   private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

class Order : public std::enable_shared_from_this<Order> {
public:

    Order(net::io_context& io, int id, HotDogHandler handler, std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread)
    : io_(io)
    , id_(id)
    , handler_(std::move(handler))
    , sausage_(sausage)
    , bread_(bread)
    {}

    void Execute(std::shared_ptr<GasCooker> gas_cooker) {
        net::post(io_, [this, self = shared_from_this(), gas_cooker] 
        {
                sausage_->StartFry(*gas_cooker, [self = shared_from_this()]
                {
                    self->FryingSausage();
                });
        });

        net::post(io_, [this, self = shared_from_this(), gas_cooker] 
        {
                bread_->StartBake(*gas_cooker, [self = shared_from_this()]
                { 
                    self->BakingBread();
                });
        });

    }

private:

    void FryingSausage() {
        auto timer = std::make_shared<net::steady_timer>(io_, std::chrono::milliseconds(1500));

        timer->async_wait(net::bind_executor(strand_, [timer, self = shared_from_this()](sys::error_code ec) 
        {
                self->OnSausageFrying(ec);
        }));
    }

    void OnSausageFrying(sys::error_code ec) {
        if(ec) {
            logger_.LogMessage("Ec :" + ec.what());
        } else {
            logger_.LogMessage("OnSausageFrying.");
        }
        sausage_->StopFry();
        MakeHotDog();
    }

    void BakingBread() {
        auto timer = std::make_shared<net::steady_timer>(io_, std::chrono::seconds(1));
 
        timer->async_wait(net::bind_executor(strand_,[timer, self = shared_from_this()](sys::error_code ec)
        {
            self->OnBreadBaking(ec);
        }));
    }

    void OnBreadBaking(sys::error_code ec) {
        if(ec) {
            logger_.LogMessage("Ec :" + ec.what());
        } else {
            logger_.LogMessage("OnBreadBaking.");
        }
        bread_->StopBaking();
        MakeHotDog();
    }

    void MakeHotDog() {
        if(deliver_) {
            return;
        }
        if(SausageBreadReady()) {
            std::cout  << "ID sausage: "<< sausage_->GetId() << std::endl;
            std::cout  << "ID bread: "<< bread_->GetId() << std::endl;
            handler_(Result<HotDog>{HotDog{id_, sausage_, bread_}});
            deliver_ = true;
        }
    }

    bool SausageBreadReady() {
        return sausage_->IsCooked() && bread_->IsCooked();
    }



    net::io_context& io_;
    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
    int id_;
    HotDogHandler handler_;
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;
    Logger logger_{std::to_string(id_)};
    //bool sausage_roasted_ = false;
    //bool bread_roasted_ = false;
    bool deliver_ = false;
}; 

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
    const int order_id = ++next_order_id_;
    
    net::dispatch(strand_, [this, order_id, handler] {
        auto sausage = store_.GetSausage();
        auto bread = store_.GetBread();
        
        std::make_shared<Order>(io_, order_id, std::move(handler), sausage, bread)->Execute(gas_cooker_);
    });
}

private:
    net::io_context& io_;
    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    int next_order_id_ = 0;
};
