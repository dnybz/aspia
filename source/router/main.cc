//
// Aspia Project
// Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "base/command_line.h"
#include "base/logging.h"
#include "base/crypto/key_pair.h"
#include "base/files/base_paths.h"
#include "router/settings.h"

#if defined(OS_WIN)
#include "base/win/service_controller.h"
#include "router/win/service.h"
#include "router/win/service_constants.h"
#else
#include "base/crypto/scoped_crypto_initializer.h"
#include "base/message_loop/message_loop.h"
#include "router/server.h"
#endif

#include <iostream>

namespace {

void initLogging()
{
    router::Settings settings;

    base::LoggingSettings logging_settings;
    logging_settings.min_log_level = settings.minLogLevel();

    base::initLogging(logging_settings);
}

void shutdownLogging()
{
    base::shutdownLogging();
}

void generateAndPrintKeys()
{
    base::KeyPair key_pair = base::KeyPair::create(base::KeyPair::Type::X25519);
    if (!key_pair.isValid())
    {
        std::cout << "Failed to generate keys" << std::endl;
        return;
    }

    std::cout << "Private key: " << base::toHex(key_pair.privateKey()) << std::endl;
    std::cout << "Public key: " << base::toHex(key_pair.publicKey()) << std::endl;
}

#if defined(OS_WIN)

void startService()
{
    base::win::ServiceController controller =
        base::win::ServiceController::open(router::kServiceName);
    if (!controller.isValid())
    {
        std::cout << "Failed to access the service. Not enough rights or service not installed."
            << std::endl;
    }
    else
    {
        if (!controller.start())
        {
            std::cout << "Failed to start the service." << std::endl;
        }
        else
        {
            std::cout << "The service started successfully." << std::endl;
        }
    }
}

void stopService()
{
    base::win::ServiceController controller =
        base::win::ServiceController::open(router::kServiceName);
    if (!controller.isValid())
    {
        std::cout << "Failed to access the service. Not enough rights or service not installed."
            << std::endl;
    }
    else
    {
        if (!controller.stop())
        {
            std::cout << "Failed to stop the service." << std::endl;
        }
        else
        {
            std::cout << "The service has stopped successfully." << std::endl;
        }
    }
}

void installService()
{
    std::filesystem::path file_path;

    if (!base::BasePaths::currentExecFile(&file_path))
    {
        std::cout << "Failed to get the path to the executable." << std::endl;
    }
    else
    {
        base::win::ServiceController controller = base::win::ServiceController::install(
            router::kServiceName, router::kServiceDisplayName, file_path);
        if (!controller.isValid())
        {
            std::cout << "Failed to install the service." << std::endl;
        }
        else
        {
            controller.setDescription(router::kServiceDescription);
            std::cout << "The service has been successfully installed." << std::endl;
        }
    }
}

void removeService()
{
    if (base::win::ServiceController::isRunning(router::kServiceName))
    {
        stopService();
    }

    if (!base::win::ServiceController::remove(router::kServiceName))
    {
        std::cout << "Failed to remove the service." << std::endl;
    }
    else
    {
        std::cout << "The service was successfully deleted." << std::endl;
    }
}

void showHelp()
{
    std::cout << "aspia_router [switch]" << std::endl
        << "Available switches:" << std::endl
        << '\t' << "--install" << '\t' << "Install service" << std::endl
        << '\t' << "--remove" << '\t' << "Remove service" << std::endl
        << '\t' << "--start" << '\t' << "Start service" << std::endl
        << '\t' << "--stop" << '\t' << "Stop service" << std::endl
        << '\t' << "--keygen" << '\t' << "Generating public and private keys" << std::endl
        << '\t' << "--help" << '\t' << "Show help" << std::endl;
}

#endif // defined(OS_WIN)

} // namespace

#if defined(OS_WIN)
int wmain()
{
    initLogging();

    base::CommandLine::init(0, nullptr); // On Windows ignores arguments.
    base::CommandLine* command_line = base::CommandLine::forCurrentProcess();

    if (command_line->hasSwitch(u"install"))
    {
        installService();
    }
    else if (command_line->hasSwitch(u"remove"))
    {
        removeService();
    }
    else if (command_line->hasSwitch(u"start"))
    {
        startService();
    }
    else if (command_line->hasSwitch(u"stop"))
    {
        stopService();
    }
    else if (command_line->hasSwitch(u"keygen"))
    {
        generateAndPrintKeys();
    }
    else if (command_line->hasSwitch(u"help"))
    {
        showHelp();
    }
    else
    {
        router::Service().exec();
    }

    shutdownLogging();
    return 0;
}
#else
int main(int argc, const char* const* argv)
{
    initLogging();

    base::CommandLine::init(argc, argv);
    base::CommandLine* command_line = base::CommandLine::forCurrentProcess();

    std::unique_ptr<base::ScopedCryptoInitializer> crypto_initializer =
        std::make_unique<base::ScopedCryptoInitializer>();

    if (command_line->hasSwitch(u"keygen"))
    {
        generateAndPrintKeys();
    }
    else if (command_line->hasSwitch(u"help"))
    {
        std::cout << "aspia_router [switch]" << std::endl
            << "Available switches:" << std::endl
            << '\t' << "--keygen" << '\t' << "Generating public and private keys" << std::endl
            << '\t' << "--help" << '\t' << "Show help" << std::endl;
    }
    else
    {
        std::unique_ptr<base::MessageLoop> message_loop =
            std::make_unique<base::MessageLoop>(base::MessageLoop::Type::ASIO);

        std::unique_ptr<router::Server> server =
            std::make_unique<router::Server>(message_loop->taskRunner());

        server->start();
        message_loop->run();

        server.reset();
        message_loop.reset();
    }

    crypto_initializer.reset();
    shutdownLogging();
}
#endif
