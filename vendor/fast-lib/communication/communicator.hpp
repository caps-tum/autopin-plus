/*
 * This file is part of fast-lib.
 * Copyright (C) 2015 RWTH Aachen University - ACS
 *
 * This file is licensed under the GNU Lesser General Public License Version 3
 * Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
 */

#ifndef FAST_LIB_COMMUNICATOR_HPP
#define FAST_LIB_COMMUNICATOR_HPP

#include <string>

namespace fast {

/**
 * \brief An abstract class to provide an interface for communication.
 *
 * This interface provides a method to send messages and to get incoming messages.
 */
class Communicator
{
public:
	/**
	 * \brief Default virtual destructor.
	 */
	virtual ~Communicator() = default;
	/**
	 * \brief Method to send a message.
	 *
	 * A pure virtual method to provide an interface to send a message.
	 */
	virtual void send_message(const std::string &message) = 0;
	/**
	 * \brief Method to get a message.
	 *
	 * A pure virtual method to provide an interface to receive a message.
	 * This is a blocking method which waits for a message.
	 */
	virtual std::string get_message() = 0;
};

} // namespace fast

#endif
