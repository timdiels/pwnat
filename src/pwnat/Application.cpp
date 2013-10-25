/*
 * Copyright (C) 2013 by Tim Diels
 *
 * This file is part of pwnat.
 *
 * pwnat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * pwnat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with pwnat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Application.h"
#include <udt/udt.h>
#include <csignal>
#include <pwnat/SocketException.h>
#include <pwnat/util.h>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>

#include <pwnat/namespaces.h>

Application* Application::m_instance = nullptr;

Application::Application(const ProgramArgs& args) :
    m_udt_service(m_io_service),
    m_args(args)
{
    assert(!m_instance); // singleton
    m_instance = this;

    boost::log::trivial::severity_level min_severity;
    switch (args.verbosity()) {
        case 1:
            min_severity = boost::log::trivial::error;
            break;

        case 2:
            min_severity = boost::log::trivial::warning;
            break;

        case 3:
            min_severity = boost::log::trivial::info;
            break;

        case 4:
            min_severity = boost::log::trivial::debug;
            break;

        case 5:
            min_severity = boost::log::trivial::trace;
            break;
    }
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= min_severity);

    signal(SIGINT, Application::signal_handler);

    if (UDT::startup() == UDT::ERROR) {
        throw runtime_error(format_udt_error("UDT startup failed"));
    }
}

void Application::run() {
    BOOST_LOG_TRIVIAL(debug) << "Running" << endl;
    while (!m_io_service.stopped()) {
        try {
            m_io_service.run();
        }
        catch (const SocketException& e) {
            BOOST_LOG_TRIVIAL(error) << e.what() << endl;
        }
    }

    m_udt_service.stop();
}

void Application::signal_handler(int sig)
{
    switch(sig)
    {
        case SIGINT:
            m_instance->m_io_service.stop();
    }
}

Application& Application::instance() {
    return *m_instance;
}

const ProgramArgs& Application::args() {
    return m_args;
}

