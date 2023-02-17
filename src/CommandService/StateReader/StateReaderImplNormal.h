// jeremie



#ifndef OWLACCESSTERMINAL_STATEREADER_ImplNormal_H
#define OWLACCESSTERMINAL_STATEREADER_ImplNormal_H

#include "StateReader.h"
#include <utility>
#include <string_view>
#include <memory>
#include <deque>
#include <boost/asio/read_until.hpp>
#include <boost/bind/bind.hpp>
#include <boost/array.hpp>
#include <boost/assert.hpp>
#include "../SerialController.h"
#include "../AirplaneState.h"
#include "./LoadDataLittleEndian.h"

namespace OwlSerialController {


    class StateReaderImplNormal : std::enable_shared_from_this<StateReaderImplNormal> {
    public:
        StateReaderImplNormal(
                std::weak_ptr<StateReader> parentRef,
                std::shared_ptr<boost::asio::serial_port> serialPort
        ) : parentRef_(std::move(parentRef)), serialPort_(std::move(serialPort)) {}

    private:
        std::weak_ptr<StateReader> parentRef_;

        boost::asio::streambuf readBuffer_;
        std::shared_ptr<boost::asio::serial_port> serialPort_;

        std::shared_ptr<AirplaneState> airplaneState_;
        uint8_t dataSize_ = 0;

        boost::system::error_code ec_{};
        std::size_t bytes_transferred_ = 0;

        size_t strange = 0;
    public:

        void start() {
            read_start();
        }

    private:

        const std::string delimStart{
                static_cast<char>(0xAA),
                static_cast<char>(0xAA),
                static_cast<char>(0xAA),
                static_cast<char>(0xAA),
        };
        const std::string delimEnd{
                static_cast<char>(0xBB),
                static_cast<char>(0xBB),
                static_cast<char>(0xBB),
                static_cast<char>(0xBB),
        };
    private:

        void read_start() {
            {
                std::string s{
                        (std::istreambuf_iterator<char>(&readBuffer_)),
                        std::istreambuf_iterator<char>()
                };
                auto p = s.find(delimStart);
                if (p == std::string::npos) {
                    // ignore
                    // BOOST_LOG_TRIVIAL(warning) << "StateReaderImplCo"
                    //                            << " cannot find start delim, next loop";
                } else {
                    BOOST_LOG_TRIVIAL(trace) << "StateReaderImplNormal"
                                             << " we find the start delim, next step";
                    // we find the start delim
                    // trim the other data before start delim
                    readBuffer_.consume(p);
                    // goto next step
                    findAfterStart();
                    return;
                }
            }
            BOOST_ASSERT(serialPort_);
            boost::asio::async_read(
                    *serialPort_,
                    readBuffer_,
                    [
                            this, self = shared_from_this()
                    ](
                            const boost::system::error_code &ec,
                            std::size_t bytes_transferred
                    ) {
                        ec_ = ec;
                        bytes_transferred_ = bytes_transferred;
                        if (ec_) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                                     << " async_read find start error: "
                                                     << ec_.what();
                            return;
                        }
                        if (bytes_transferred_ == 0) {
                            ++strange;
                            if (strange > 10) {
                                BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                                         << " async_read strange";
                                return;
                            }
                            // retry
                            read_start();
                            return;
                        }
                        strange = 0;
                        {
                            std::string s{
                                    (std::istreambuf_iterator<char>(&readBuffer_)),
                                    std::istreambuf_iterator<char>()
                            };
                            auto p = s.find(delimStart);
                            if (p == std::string::npos) {
                                read_start();
                                return;
                            }
                            // we find the start delim
                            // trim the other data before start delim
                            readBuffer_.consume(p);
                            // goto next step
                        }
                        findAfterStart();

                    });
        }

        void findAfterStart() {
            // ======================== find next
            if (!std::string{
                    (std::istreambuf_iterator<char>(&readBuffer_)),
                    std::istreambuf_iterator<char>()
            }.starts_with(delimStart)) {
                BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                         << " check next start tag error, never go there !!!";
                BOOST_ASSERT_MSG(false,
                                 "StateReaderImplNormal check next start tag error, never go there !!!");
                return;
            }
            // remove start tag
            readBuffer_.consume(delimStart.size());

            if (readBuffer_.size() < sizeof(typeof(dataSize_))) {
                ec_.clear();
                bytes_transferred_ = 0;

                loadSize();
                readDataLength();
                return;
            }

            loadSize();
            processData();
        }

        void loadSize() {

            // now we have the data length tag and more data
            // load size
            dataSize_ = 0;
            {
                static_assert(sizeof(typeof(dataSize_)) == 1);
                // dataSize_ = uint8_t
                std::string d{
                        (std::istreambuf_iterator<char>(&readBuffer_)),
                        std::istreambuf_iterator<char>()
                };
                // dataSize <- d
                dataSize_ = static_cast<uint8_t>(d[0]);
            }
        }

        void readDataLength() {

            BOOST_ASSERT(serialPort_);
            boost::asio::async_read(
                    *serialPort_,
                    readBuffer_,
                    boost::asio::transfer_exactly(
                            (dataSize_ + sizeof(uint32_t) + delimEnd.size()) - readBuffer_.size()),
                    [
                            this, self = shared_from_this()
                    ](
                            const boost::system::error_code &ec,
                            std::size_t bytes_transferred
                    ) {
                        ec_ = ec;
                        bytes_transferred_ = bytes_transferred;
                        boost::ignore_unused(bytes_transferred_);
                        if (ec_) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                                     << " async_read_until data error: "
                                                     << ec_.what();
                            return;
                        }
                        if (readBuffer_.size() < (dataSize_ + sizeof(uint32_t) + delimEnd.size())) {
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                                     << " async_read_until data bad";
                            return;
                        }

                        processData();
                    });
        }

        void processData() {

            if (readBuffer_.size() < (dataSize_ + sizeof(uint32_t) + delimEnd.size())) {
                BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                         << " async_read_until data bad";
                return;
            }
            // ======================================= process data
            airplaneState_ = std::make_shared<AirplaneState>();
            BOOST_ASSERT(airplaneState_);
            {
                if (dataSize_ != AirplaneStateDataSize) {
                    BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                             << " (dataSize_ != AirplaneStateDataSize ) , ignore!!!";
                    // ignore this package
                } else {
                    loadData();
                    // send
                    {
                        auto ptr_sr = parentRef_.lock();
                        if (!ptr_sr) {
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                                     << " parentRef_.lock() ptr_sr failed.";
                            return;
                        }
                        // do a ptr copy to make sure ptr not release by next loop too early
                        ptr_sr->sendAirplaneState(airplaneState_->shared_from_this());
                    }
                }

            }

            makeClean();

        }

        void makeClean() {

            // ======================================= make clean
            // clean all used data
            {
                std::string s{
                        (std::istreambuf_iterator<char>(&readBuffer_)),
                        std::istreambuf_iterator<char>()
                };
                auto p = s.find(delimEnd);
                if (p == std::string::npos) {
                    // error, never go there
                    BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                             << " make clean check error. never gone.";
                    BOOST_ASSERT(p != std::string::npos);
                    return;
                } else {
                    // we find the end delim
                    // trim the other data include end delim
                    readBuffer_.consume(p + delimEnd.size());
                    // goto next loop
                    read_start();
                    return;
                }
            }

        }

        void loadData() {

            // https://stackoverflow.com/questions/41220792/how-copy-or-reuse-boostasiostreambuf
            // std::vector<uint8_t> data(readBuffer_.size());
            // boost::asio::buffer_copy(boost::asio::buffer(data), readBuffer_.data());
            std::string data{
                    (std::istreambuf_iterator<char>(&readBuffer_)),
                    std::istreambuf_iterator<char>()
            };
            if (data.size() < dataSize_) {
                BOOST_LOG_TRIVIAL(error) << "StateReaderImplNormal"
                                         << " loadData (data.size() < dataSize_), never go there !!!!!";
                BOOST_ASSERT(!(data.size() < dataSize_));
            }

            // https://www.ruanyifeng.com/blog/2016/11/byte-order.html
            static_assert(sizeof(typeof(AirplaneState::stateFly)) == sizeof(uint8_t));
            BOOST_ASSERT(data.size() >= sizeof(uint8_t));
            airplaneState_->stateFly = loadDataLittleEndian<uint8_t>({data.begin(), data.end()});
            data.erase(data.begin(), data.end() + sizeof(uint8_t) * 1);


            static_assert(sizeof(typeof(AirplaneState::pitch)) == sizeof(int32_t));
            BOOST_ASSERT(data.size() >= sizeof(int32_t));
            airplaneState_->pitch = loadDataLittleEndian<int32_t>({data.begin(), data.end()});
            // airplaneState_->pitch = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
            static_assert(sizeof(typeof(AirplaneState::roll)) == sizeof(int32_t));
            BOOST_ASSERT(data.size() >= sizeof(int32_t) * 2);
            airplaneState_->roll = loadDataLittleEndian<int32_t>(
                    {data.begin() + sizeof(int32_t) * 1, data.end()});
            // airplaneState_->roll = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
            static_assert(sizeof(typeof(AirplaneState::yaw)) == sizeof(int32_t));
            BOOST_ASSERT(data.size() >= sizeof(int32_t) * 3);
            airplaneState_->yaw = loadDataLittleEndian<int32_t>(
                    {data.begin() + sizeof(int32_t) * 2, data.end()});

            BOOST_ASSERT(data.size() >= sizeof(int32_t) * 3);
            data.erase(data.begin(), data.end() + sizeof(int32_t) * 3);


            static_assert(sizeof(typeof(AirplaneState::vx)) == sizeof(int32_t));
            BOOST_ASSERT(data.size() >= sizeof(int32_t) * 1);
            airplaneState_->vx = loadDataLittleEndian<int32_t>(
                    {data.begin(), data.end()});
            static_assert(sizeof(typeof(AirplaneState::vy)) == sizeof(int32_t));
            BOOST_ASSERT(data.size() >= sizeof(int32_t) * 2);
            airplaneState_->vy = loadDataLittleEndian<int32_t>(
                    {data.begin() + sizeof(int32_t) * 1, data.end()});
            static_assert(sizeof(typeof(AirplaneState::vz)) == sizeof(int32_t));
            BOOST_ASSERT(data.size() >= sizeof(int32_t) * 3);
            airplaneState_->vz = loadDataLittleEndian<int32_t>(
                    {data.begin() + sizeof(int32_t) * 2, data.end()});

            BOOST_ASSERT(data.size() >= sizeof(int32_t) * 3);
            data.erase(data.begin(), data.end() + sizeof(int32_t) * 3);

            static_assert(sizeof(typeof(AirplaneState::high)) == sizeof(uint16_t));
            BOOST_ASSERT(data.size() >= sizeof(uint16_t));
            airplaneState_->high = loadDataLittleEndian<uint16_t>(
                    {data.begin(), data.end()});
            data.erase(data.begin(), data.end() + sizeof(uint16_t) * 1);

            static_assert(sizeof(typeof(AirplaneState::voltage)) == sizeof(uint16_t));
            BOOST_ASSERT(data.size() >= sizeof(uint16_t));
            airplaneState_->voltage = loadDataLittleEndian<uint16_t>(
                    {data.begin(), data.end()});
            data.erase(data.begin(), data.end() + sizeof(uint16_t) * 1);

        }


    };


}

#endif //OWLACCESSTERMINAL_STATEREADER_ImplNormal_H

